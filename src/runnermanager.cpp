/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2007, 2009 Ryan P. Bitanga <ryan.bitanga@gmail.com>
    SPDX-FileCopyrightText: 2008 Jordi Polo <mumismo@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnermanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

#include <KConfigWatcher>
#include <KFileUtils>
#include <KPluginMetaData>
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72)
#include <KServiceTypeTrader>
#else
#define KSERVICE_BUILD_DEPRECATED_SINCE(a, b) 0
#endif
#include <KSharedConfig>

#include "config.h"
#if HAVE_KACTIVITIES
#include <KActivities/Consumer>
#endif

#include <ThreadWeaver/DebuggingAids>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Thread>

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 65)
#include <plasma/version.h>
#endif

#include "dbusrunner_p.h"
#include "kpluginmetadata_utils_p.h"
#include "krunner_debug.h"
#include "querymatch.h"
#include "runnerjobs_p.h"

using ThreadWeaver::Queue;

namespace Plasma
{
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72) && KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
void warnAboutDeprecatedMetaData(const KPluginInfo &pluginInfo)
{
    if (!pluginInfo.libraryPath().isEmpty()) {
        qCWarning(KRUNNER).nospace() << "KRunner plugin " << pluginInfo.pluginName() << " still uses a .desktop file (" << pluginInfo.entryPath()
                                     << "). Please port it to JSON metadata.";
    } else {
        qCWarning(KRUNNER).nospace() << "KRunner D-Bus plugin " << pluginInfo.pluginName() << " installs the .desktop file (" << pluginInfo.entryPath()
                                     << ") still in the kservices5 folder. Please install it to ${KDE_INSTALL_DATAROOTDIR}/krunner/dbusplugins.";
    }
}
#endif

class RunnerManagerPrivate
{
public:
    RunnerManagerPrivate(RunnerManager *parent)
        : q(parent)
    {
        matchChangeTimer.setSingleShot(true);
        matchChangeTimer.setTimerType(Qt::TimerType::PreciseTimer); // Without this, autotest will fail due to imprecision of this timer
        delayTimer.setSingleShot(true);

        QObject::connect(&matchChangeTimer, &QTimer::timeout, q, [this]() {
            matchesChanged();
        });
        QObject::connect(&context, &RunnerContext::matchesChanged, q, [this]() {
            scheduleMatchesChanged();
        });
        QObject::connect(&delayTimer, &QTimer::timeout, q, [this]() {
            unblockJobs();
        });

        // Set up tracking of the last time matchesChanged was signalled
        lastMatchChangeSignalled.start();
        QObject::connect(q, &RunnerManager::matchesChanged, q, [&] {
            lastMatchChangeSignalled.restart();
        });
    }

    void scheduleMatchesChanged()
    {
        // We avoid over-refreshing the client. We only refresh every this much milliseconds
        constexpr int refreshPeriod = 250;
        // This will tell us if we are reseting the matches to start a new search. RunnerContext::reset() clears its query string for its emission
        if (context.query().isEmpty()) {
            matchChangeTimer.stop();
            // This actually contains the query string for the new search that we're launching (if any):
            if (!this->untrimmedTerm.trimmed().isEmpty()) {
                // We are starting a new search, we shall stall for some time before deciding to show an empty matches list.
                // This stall should be enough for the engine to provide more meaningful result, so we avoid refreshing with
                // an empty results list if possible.
                matchChangeTimer.start(refreshPeriod);
                // We "pretend" that we have refreshed it so the next call will be forced to wait the timeout:
                lastMatchChangeSignalled.restart();
            } else {
                // We have an empty input string, so it's not a real query. We don't expect any results to come, so no need to stall
                Q_EMIT q->matchesChanged(context.matches());
            }
        } else if (lastMatchChangeSignalled.hasExpired(refreshPeriod)) {
            matchChangeTimer.stop();
            Q_EMIT q->matchesChanged(context.matches());
        } else {
            matchChangeTimer.start(refreshPeriod - lastMatchChangeSignalled.elapsed());
        }
    }

    void matchesChanged()
    {
        Q_EMIT q->matchesChanged(context.matches());
    }

    void loadConfiguration()
    {
        // Limit the number of instances of a single normal speed runner and all of the slow runners
        // to half the number of threads
        DefaultRunnerPolicy::instance().setCap(qMax(2, Queue::instance()->maximumNumberOfThreads() / 2));

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
        enabledCategories = stateData.readEntry("enabledCategories", QStringList());
#endif
#if HAVE_KACTIVITIES
        // Wait for consumer to be ready
        QObject::connect(&activitiesConsumer,
                         &KActivities::Consumer::serviceStatusChanged,
                         &activitiesConsumer,
                         [this](KActivities::Consumer::ServiceStatus status) {
                             if (status == KActivities::Consumer::Running) {
                                 deleteHistoryOfDeletedActivities();
                             }
                         });
#endif
        const KConfigGroup generalConfig = configPrt->group("General");
        const bool _historyEnabled = generalConfig.readEntry("HistoryEnabled", true);
        if (historyEnabled != _historyEnabled) {
            historyEnabled = _historyEnabled;
            Q_EMIT q->historyEnabledChanged();
        }
        activityAware = generalConfig.readEntry("ActivityAware", true);
        retainPriorSearch = generalConfig.readEntry("RetainPriorSearch", true);
        context.restore(stateData);
    }

    void loadSingleRunner()
    {
        // In case we are not in the single runner mode of we do not have an id
        if (!singleMode || singleModeRunnerId.isEmpty()) {
            currentSingleRunner = nullptr;
            return;
        }

        if (currentSingleRunner && currentSingleRunner->id() == singleModeRunnerId) {
            return;
        }
        currentSingleRunner = q->runner(singleModeRunnerId);
        // If there are no runners loaded or the single runner could no be loaded,
        // this is the case if it was disabled but gets queries using the singleRunnerMode, BUG: 435050
        if (runners.isEmpty() || !currentSingleRunner) {
            loadRunners(singleModeRunnerId);
            currentSingleRunner = q->runner(singleModeRunnerId);
        }
    }

    void loadRunners(const QString &singleRunnerId = QString())
    {
        QVector<KPluginMetaData> offers = RunnerManager::runnerMetaDataList();

        const bool loadAll = stateData.readEntry("loadAll", false);
        const bool noWhiteList = whiteList.isEmpty();
        KConfigGroup pluginConf = configPrt->group("Plugins");

        QSet<AbstractRunner *> deadRunners;
        QMutableVectorIterator<KPluginMetaData> it(offers);
        while (it.hasNext()) {
            KPluginMetaData &description = it.next();
            qCDebug(KRUNNER) << "Loading runner: " << description.pluginId();

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 85)
            const QString tryExec = description.value(QStringLiteral("TryExec"));
            if (!tryExec.isEmpty()) {
                qCDebug(KRUNNER) << description.pluginId() << "The TryExec property is deprecated, manually check if the application exists if needed";
                if (QStandardPaths::findExecutable(tryExec).isEmpty()) {
                    // we don't actually have this application!
                    continue;
                }
            }
#endif

            const QString runnerName = description.pluginId();
            const bool isPluginEnabled = description.isEnabled(pluginConf);
            const bool loaded = runners.contains(runnerName);
            bool selected = loadAll || disabledRunnerIds.contains(runnerName) || (isPluginEnabled && (noWhiteList || whiteList.contains(runnerName)));
            if (!selected && runnerName == singleRunnerId) {
                selected = true;
                disabledRunnerIds << runnerName;
            }

            if (selected) {
                AbstractRunner *runner = nullptr;
                if (!loaded) {
                    runner = loadInstalledRunner(description);
                } else {
                    runner = runners.value(runnerName);
                }

                if (runner) {
                    bool allCategoriesDisabled = true;
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
                    const QStringList categories = runner->categories();

                    for (const QString &cat : categories) {
                        if (enabledCategories.contains(cat)) {
                            allCategoriesDisabled = false;
                            break;
                        }
                    }
#endif

                    if (enabledCategories.isEmpty() || !allCategoriesDisabled) {
                        qCDebug(KRUNNER) << "Loaded:" << runnerName;
                        runners.insert(runnerName, runner);
                    } else {
                        runners.remove(runnerName);
                        deadRunners.insert(runner);
                        qCDebug(KRUNNER) << "Categories not enabled. Removing runner: " << runnerName;
                    }
                }
            } else if (loaded) {
                // Remove runner
                deadRunners.insert(runners.take(runnerName));
                qCDebug(KRUNNER) << "Plugin disabled. Removing runner: " << runnerName;
            }
        }

        if (!deadRunners.isEmpty()) {
            QSet<QSharedPointer<FindMatchesJob>> deadJobs;
            auto it = searchJobs.begin();
            while (it != searchJobs.end()) {
                auto &job = (*it);
                if (deadRunners.contains(job->runner())) {
                    QObject::disconnect(job.data(), &FindMatchesJob::done, q, nullptr);
                    it = searchJobs.erase(it);
                    deadJobs.insert(job);
                } else {
                    it++;
                }
            }

            it = oldSearchJobs.begin();
            while (it != oldSearchJobs.end()) {
                auto &job = (*it);
                if (deadRunners.contains(job->runner())) {
                    it = oldSearchJobs.erase(it);
                    deadJobs.insert(job);
                } else {
                    it++;
                }
            }

            if (deadJobs.isEmpty()) {
                qDeleteAll(deadRunners);
            } else {
                new DelayedJobCleaner(deadJobs, deadRunners);
            }
        }

        // in case we deleted it up above, just to be sure we do not have a dangeling pointer
        currentSingleRunner = nullptr;

        qCDebug(KRUNNER) << "All runners loaded, total:" << runners.count();
    }

    AbstractRunner *loadInstalledRunner(const KPluginMetaData &pluginMetaData)
    {
        if (!pluginMetaData.isValid()) {
            return nullptr;
        }

        AbstractRunner *runner = nullptr;

        const QString api = pluginMetaData.value(QStringLiteral("X-Plasma-API"));

        if (api.isEmpty()) {
                const QVariantList args
                {
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 77)
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
                    pluginMetaData.metaDataFileName(),
#endif
                        QVariant::fromValue(pluginMetaData),
#endif
                };
                auto res = KPluginFactory::instantiatePlugin<AbstractRunner>(pluginMetaData, q, args);
                if (res) {
                    runner = res.plugin;
                } else {
                    qCWarning(KRUNNER).nospace() << "Could not load runner " << pluginMetaData.name() << ":" << res.errorString
                                                 << " (library path was:" << pluginMetaData.fileName() << ")";
                }
        } else if (api.startsWith(QLatin1String("DBus"))) {
            runner = new DBusRunner(q, pluginMetaData, {});
        } else {
            qCWarning(KRUNNER) << "Unknown X-Plasma-API requested for runner" << pluginMetaData.fileName();
            return nullptr;
        }

        if (runner) {
            QObject::connect(runner, &AbstractRunner::matchingSuspended, q, [this](bool state) {
                runnerMatchingSuspended(state);
            });
            runner->init();
            if (prepped) {
                Q_EMIT runner->prepare();
            }
        }

        return runner;
    }

    void jobDone(ThreadWeaver::JobPointer job)
    {
        auto runJob = job.dynamicCast<FindMatchesJob>();

        if (!runJob) {
            return;
        }

        searchJobs.remove(runJob);
        oldSearchJobs.remove(runJob);

        if (searchJobs.isEmpty()) {
            // If there are any new matches scheduled to be notified, we should anticipate it and just refresh right now
            if (matchChangeTimer.isActive()) {
                matchChangeTimer.stop();
                Q_EMIT q->matchesChanged(context.matches());
            } else if (context.matches().isEmpty()) {
                // we finished our run, and there are no valid matches, and so no
                // signal will have been sent out. so we need to emit the signal
                // ourselves here
                Q_EMIT q->matchesChanged(context.matches());
            }
            Q_EMIT q->queryFinished();
        }
    }

    void checkTearDown()
    {
        if (!prepped || !teardownRequested) {
            return;
        }

        if (Queue::instance()->isIdle()) {
            searchJobs.clear();
            oldSearchJobs.clear();
        }

        if (searchJobs.isEmpty() && oldSearchJobs.isEmpty()) {
            if (allRunnersPrepped) {
                for (AbstractRunner *runner : std::as_const(runners)) {
                    Q_EMIT runner->teardown();
                }

                allRunnersPrepped = false;
            }

            if (singleRunnerPrepped) {
                if (currentSingleRunner) {
                    Q_EMIT currentSingleRunner->teardown();
                }

                singleRunnerPrepped = false;
            }

            prepped = false;
            teardownRequested = false;
        }
    }

    void unblockJobs()
    {
        if (searchJobs.isEmpty() && Queue::instance()->isIdle()) {
            oldSearchJobs.clear();
            checkTearDown();
            return;
        }

        Queue::instance()->reschedule();
    }

    void runnerMatchingSuspended(bool suspended)
    {
        auto *runner = qobject_cast<AbstractRunner *>(q->sender());
        if (suspended || !prepped || teardownRequested || !runner) {
            return;
        }

        const QString query = context.query();
        if (singleMode || runner->minLetterCount() <= query.size()) {
            if (singleMode || !runner->hasMatchRegex() || runner->matchRegex().match(query).hasMatch()) {
                startJob(runner);
            }
        }
    }

    void startJob(AbstractRunner *runner)
    {
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
        if ((runner->ignoredTypes() & context.type()) != 0) {
            return;
        }
#endif
        QSharedPointer<FindMatchesJob> job(new FindMatchesJob(runner, &context, Queue::instance()));
        QObject::connect(job.data(), &FindMatchesJob::done, q, [this](ThreadWeaver::JobPointer jobPtr) {
            jobDone(jobPtr);
        });

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
        if (runner->speed() == AbstractRunner::SlowSpeed) {
            job->setDelayTimer(&delayTimer);
        }
#endif
        Queue::instance()->enqueue(job);
        searchJobs.insert(job);
    }

    inline QString getActivityKey()
    {
#if HAVE_KACTIVITIES
        if (activityAware) {
            const QString currentActivity = activitiesConsumer.currentActivity();
            return currentActivity.isEmpty() ? nulluuid : currentActivity;
        }
#endif
        return nulluuid;
    }

    void addToHistory()
    {
        const QString term = context.query();
        // We want to imitate the shall behavior
        if (!historyEnabled || term.isEmpty() || untrimmedTerm.startsWith(QLatin1Char(' '))) {
            return;
        }
        QStringList historyEntries = readHistoryForCurrentActivity();
        // Avoid removing the same item from the front and prepending it again
        if (!historyEntries.isEmpty() && historyEntries.constFirst() == term) {
            return;
        }

        historyEntries.removeOne(term);
        historyEntries.prepend(term);

        while (historyEntries.count() > 50) { // we don't want to store more than 50 entries
            historyEntries.removeLast();
        }
        writeActivityHistory(historyEntries);
    }

    void writeActivityHistory(const QStringList &historyEntries)
    {
        stateData.group("History").writeEntry(getActivityKey(), historyEntries, KConfig::Notify);
        stateData.sync();
    }

#if HAVE_KACTIVITIES
    void deleteHistoryOfDeletedActivities()
    {
        KConfigGroup historyGroup = stateData.group("History");
        QStringList historyEntries = historyGroup.keyList();
        historyEntries.removeOne(nulluuid);

        // Check if history still exists
        const QStringList activities = activitiesConsumer.activities();
        for (const auto &a : activities) {
            historyEntries.removeOne(a);
        }

        for (const QString &deletedActivity : std::as_const(historyEntries)) {
            historyGroup.deleteEntry(deletedActivity);
        }
        historyGroup.sync();
    }
#endif

    inline QStringList readHistoryForCurrentActivity()
    {
        return stateData.group("History").readEntry(getActivityKey(), QStringList());
    }

    // Delay in ms before slow runners are allowed to run
    static const int slowRunDelay = 400;

    RunnerManager *const q;
    RunnerContext context;
    QTimer matchChangeTimer;
    QTimer delayTimer; // Timer to control when to run slow runners
    QElapsedTimer lastMatchChangeSignalled;
    QHash<QString, AbstractRunner *> runners;
    AbstractRunner *currentSingleRunner = nullptr;
    QSet<QSharedPointer<FindMatchesJob>> searchJobs;
    QSet<QSharedPointer<FindMatchesJob>> oldSearchJobs;
    QStringList enabledCategories;
    QString singleModeRunnerId;
    bool prepped = false;
    bool allRunnersPrepped = false;
    bool singleRunnerPrepped = false;
    bool teardownRequested = false;
    bool singleMode = false;
    bool activityAware = false;
    bool historyEnabled = false;
    bool retainPriorSearch = false;
    QStringList whiteList;
    KConfigWatcher::Ptr watcher;
    QHash<QString, QString> priorSearch;
    QString untrimmedTerm;
    QString nulluuid = QStringLiteral("00000000-0000-0000-0000-000000000000");
    KSharedConfigPtr configPrt;
    KConfigGroup stateData;
    QSet<QString> disabledRunnerIds; // Runners that are disabled but were loaded as single runners
#if HAVE_KACTIVITIES
    const KActivities::Consumer activitiesConsumer;
#endif
};

RunnerManager::RunnerManager(QObject *parent)
    : RunnerManager(QString(), parent)
{
}

RunnerManager::RunnerManager(const QString &configFile, QObject *parent)
    : QObject(parent)
    , d(new RunnerManagerPrivate(this))
{
    d->configPrt = KSharedConfig::openConfig(configFile);
    // If the old config group still exists the migration script wasn't executed
    // so we keep using this location
    KConfigGroup oldStateDataGroup = d->configPrt->group("PlasmaRunnerManager");
    if (oldStateDataGroup.exists() && !oldStateDataGroup.readEntry("migrated", false)) {
        d->stateData = oldStateDataGroup;
    } else {
        d->stateData =
            KSharedConfig::openConfig(QStringLiteral("krunnerstaterc"), KConfig::NoGlobals, QStandardPaths::GenericDataLocation)->group("PlasmaRunnerManager");
    }
    d->loadConfiguration();
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
RunnerManager::RunnerManager(KConfigGroup &c, QObject *parent)
    : QObject(parent)
    , d(new RunnerManagerPrivate(this))
{
    d->configPrt = KSharedConfig::openConfig();
    d->stateData = KConfigGroup(&c, "PlasmaRunnerManager");
    d->loadConfiguration();
}
#endif

RunnerManager::~RunnerManager()
{
    if (!qApp->closingDown() && (!d->searchJobs.isEmpty() || !d->oldSearchJobs.isEmpty())) {
        const QSet<QSharedPointer<FindMatchesJob>> jobs(d->searchJobs + d->oldSearchJobs);
        QSet<AbstractRunner *> runners;
        for (auto &job : jobs) {
            job->runner()->setParent(nullptr);
            runners << job->runner();
        }
        new DelayedJobCleaner(jobs, runners);
    }
}

void RunnerManager::reloadConfiguration()
{
    d->configPrt->reparseConfiguration();
    d->stateData.config()->reparseConfiguration();
    d->loadConfiguration();
    d->loadRunners();
}

void RunnerManager::setAllowedRunners(const QStringList &runners)
{
    d->whiteList = runners;
    if (!d->runners.isEmpty()) {
        // this has been called with runners already created. so let's do an instant reload
        d->loadRunners();
    }
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
void RunnerManager::setEnabledCategories(const QStringList &categories)
{
    d->stateData.writeEntry("enabledCategories", categories);
    d->enabledCategories = categories;

    if (!d->runners.isEmpty()) {
        d->loadRunners();
    }
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 88)
QStringList RunnerManager::allowedRunners() const
{
    return d->stateData.readEntry("pluginWhiteList", QStringList());
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
QStringList RunnerManager::enabledCategories() const
{
    QStringList list = d->stateData.readEntry("enabledCategories", QStringList());
    if (list.isEmpty()) {
        list.reserve(d->runners.count());
        for (AbstractRunner *runner : std::as_const(d->runners)) {
            list << runner->categories();
        }
    }

    return list;
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72) && KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
void RunnerManager::loadRunner(const KService::Ptr service)
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
    QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
    KPluginInfo description(service);
    QT_WARNING_POP
    loadRunner(description.toMetaData());
}
#endif

void RunnerManager::loadRunner(const KPluginMetaData &pluginMetaData)
{
    const QString runnerName = pluginMetaData.pluginId();
    if (!runnerName.isEmpty() && !d->runners.contains(runnerName)) {
        if (AbstractRunner *runner = d->loadInstalledRunner(pluginMetaData)) {
            d->runners.insert(runnerName, runner);
        }
    }
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 77)
void RunnerManager::loadRunner(const QString &path)
{
    if (!d->runners.contains(path)) {
        AbstractRunner *runner = new AbstractRunner(this, path);
        connect(runner, &AbstractRunner::matchingSuspended, this, [this](bool state) {
            d->runnerMatchingSuspended(state);
        });
        d->runners.insert(path, runner);
    }
}
#endif

AbstractRunner *RunnerManager::runner(const QString &name) const
{
    if (d->runners.isEmpty()) {
        d->loadRunners();
    }

    return d->runners.value(name, nullptr);
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 82)
AbstractRunner *RunnerManager::singleModeRunner() const
{
    return d->currentSingleRunner;
}

void RunnerManager::setSingleModeRunnerId(const QString &id)
{
    d->singleModeRunnerId = id;
    d->loadSingleRunner();
}

QString RunnerManager::singleModeRunnerId() const
{
    return d->singleModeRunnerId;
}

bool RunnerManager::singleMode() const
{
    return d->singleMode;
}

void RunnerManager::setSingleMode(bool singleMode)
{
    if (d->singleMode == singleMode) {
        return;
    }

    Plasma::AbstractRunner *prevSingleRunner = d->currentSingleRunner;
    d->singleMode = singleMode;
    d->loadSingleRunner();
    d->singleMode = d->currentSingleRunner;

    if (prevSingleRunner != d->currentSingleRunner) {
        if (d->prepped) {
            matchSessionComplete();

            if (d->singleMode) {
                setupMatchSession();
            }
        }
    }
}

QStringList RunnerManager::singleModeAdvertisedRunnerIds() const
{
    QStringList advertiseSingleRunnerIds;
    for (auto *runner : std::as_const(d->runners)) {
        if (runner->metadata(RunnerReturnPluginMetaData).rawData().value(QStringLiteral("X-Plasma-AdvertiseSingleRunnerQueryMode")).toVariant().toBool()) {
            advertiseSingleRunnerIds << runner->id();
        }
    }
    return advertiseSingleRunnerIds;
}

QString RunnerManager::runnerName(const QString &id) const
{
    return d->runners.contains(id) ? d->runners.value(id)->name() : QString();
}
#endif

QList<AbstractRunner *> RunnerManager::runners() const
{
    return d->runners.values();
}

RunnerContext *RunnerManager::searchContext() const
{
    return &d->context;
}

// Reordering is here so data is not reordered till strictly needed
QList<QueryMatch> RunnerManager::matches() const
{
    return d->context.matches();
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 79)
void RunnerManager::run(const QString &matchId)
{
    run(d->context.match(matchId));
}
#endif

void RunnerManager::run(const QueryMatch &match)
{
    if (match.isEnabled()) {
        d->context.run(match);
    }
}

bool RunnerManager::runMatch(const QueryMatch &match)
{
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 99)
    if (match.type() == Plasma::QueryMatch::InformationalMatch && !match.selectedAction()) {
        d->addToHistory();
        const QString info = match.data().toString();
        qWarning() << Q_FUNC_INFO << info << match.data();
        if (!info.isEmpty()) {
            Q_EMIT setSearchTerm(info, info.length());
            return false;
        }
    }
#endif
    d->context.run(match);
    if (!d->context.shouldIgnoreCurrentMatchForHistory()) {
        d->addToHistory();
    }
    if (d->context.requestedQueryString().isEmpty()) {
        return true;
    } else {
        Q_EMIT setSearchTerm(d->context.requestedQueryString(), d->context.requestedCursorPosition());
        return false;
    }
}

QList<QAction *> RunnerManager::actionsForMatch(const QueryMatch &match)
{
    if (AbstractRunner *runner = match.runner()) {
        return runner->actionsForMatch(match);
    }

    return QList<QAction *>();
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 79)
QMimeData *RunnerManager::mimeDataForMatch(const QString &id) const
{
    return mimeDataForMatch(d->context.match(id));
}
#endif

QMimeData *RunnerManager::mimeDataForMatch(const QueryMatch &match) const
{
    return match.isValid() ? match.runner()->mimeDataForMatch(match) : nullptr;
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 85)
QVector<KPluginMetaData> RunnerManager::runnerMetaDataList(const QString &parentApp)
{
    // get binary plugins
    // filter rule also covers parentApp.isEmpty()
    auto filterParentApp = [&parentApp](const KPluginMetaData &md) -> bool {
        return md.value(QStringLiteral("X-KDE-ParentApp")) == parentApp;
    };

    QVector<KPluginMetaData> pluginMetaDatas = KPluginMetaData::findPlugins(QStringLiteral("kf5/krunner"), filterParentApp);
    QSet<QString> knownRunnerIds;
    knownRunnerIds.reserve(pluginMetaDatas.size());
    for (const KPluginMetaData &pluginMetaData : std::as_const(pluginMetaDatas)) {
        knownRunnerIds.insert(pluginMetaData.pluginId());
    }

    const QStringList dBusPlugindirs =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("krunner/dbusplugins"), QStandardPaths::LocateDirectory);
    const QStringList dbusRunnerFiles = KFileUtils::findAllUniqueFiles(dBusPlugindirs, QStringList(QStringLiteral("*.desktop")));
    for (const QString &dbusRunnerFile : dbusRunnerFiles) {
        KPluginMetaData pluginMetaData = parseMetaDataFromDesktopFile(dbusRunnerFile);
        if (pluginMetaData.isValid() && !knownRunnerIds.contains(pluginMetaData.pluginId())) {
            pluginMetaDatas.append(pluginMetaData);
            knownRunnerIds.insert(pluginMetaData.pluginId());
        }
    }

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
    // also search for deprecated kservice-based KRunner plugins metadata
    const QString constraint = parentApp.isEmpty() ? QStringLiteral("not exist [X-KDE-ParentApp] or [X-KDE-ParentApp] == ''")
                                                   : QStringLiteral("[X-KDE-ParentApp] == '") + parentApp + QLatin1Char('\'');

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    const KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("Plasma/Runner"), constraint);
    const KPluginInfo::List backwardCompatPluginInfos = KPluginInfo::fromServices(offers);
    QT_WARNING_POP

    for (const KPluginInfo &pluginInfo : backwardCompatPluginInfos) {
        if (!knownRunnerIds.contains(pluginInfo.pluginName())) {
            warnAboutDeprecatedMetaData(pluginInfo);
            pluginMetaDatas.append(pluginInfo.toMetaData());
        }
    }
#endif

    return pluginMetaDatas;
}
#endif

QVector<KPluginMetaData> RunnerManager::runnerMetaDataList()
{
    QVector<KPluginMetaData> pluginMetaDatas = KPluginMetaData::findPlugins(QStringLiteral("kf" QT_STRINGIFY(QT_VERSION_MAJOR) "/krunner"));
    QSet<QString> knownRunnerIds;
    knownRunnerIds.reserve(pluginMetaDatas.size());
    for (const KPluginMetaData &pluginMetaData : std::as_const(pluginMetaDatas)) {
        knownRunnerIds.insert(pluginMetaData.pluginId());
    }

    const QStringList dBusPlugindirs =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("krunner/dbusplugins"), QStandardPaths::LocateDirectory);
    const QStringList dbusRunnerFiles = KFileUtils::findAllUniqueFiles(dBusPlugindirs, QStringList(QStringLiteral("*.desktop")));
    for (const QString &dbusRunnerFile : dbusRunnerFiles) {
        KPluginMetaData pluginMetaData = parseMetaDataFromDesktopFile(dbusRunnerFile);
        if (pluginMetaData.isValid() && !knownRunnerIds.contains(pluginMetaData.pluginId())) {
            pluginMetaDatas.append(pluginMetaData);
            knownRunnerIds.insert(pluginMetaData.pluginId());
        }
    }

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72) && KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
    // also search for deprecated kservice-based KRunner plugins metadata
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    const KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("Plasma/Runner"));
    const KPluginInfo::List backwardCompatPluginInfos = KPluginInfo::fromServices(offers);
    QT_WARNING_POP

    for (const KPluginInfo &pluginInfo : backwardCompatPluginInfos) {
        if (!knownRunnerIds.contains(pluginInfo.pluginName())) {
            warnAboutDeprecatedMetaData(pluginInfo);
            pluginMetaDatas.append(pluginInfo.toMetaData());
        }
    }
#endif

    return pluginMetaDatas;
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72)
KPluginInfo::List RunnerManager::listRunnerInfo(const QString &parentApp)
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    return KPluginInfo::fromMetaData(runnerMetaDataList(parentApp));
    QT_WARNING_POP
}
#endif

void RunnerManager::setupMatchSession()
{
    d->teardownRequested = false;

    if (d->prepped) {
        return;
    }

    d->prepped = true;
    if (d->singleMode) {
        if (d->currentSingleRunner) {
            Q_EMIT d->currentSingleRunner->prepare();
            d->singleRunnerPrepped = true;
        }
    } else {
        for (AbstractRunner *runner : std::as_const(d->runners)) {
            if (!d->disabledRunnerIds.contains(runner->name())) {
                Q_EMIT runner->prepare();
            }
        }

        d->allRunnersPrepped = true;
    }
}

void RunnerManager::matchSessionComplete()
{
    if (!d->prepped) {
        return;
    }

    d->teardownRequested = true;
    d->checkTearDown();
    // We save the context config after each session, just like the history entries
    // BUG: 424505
    d->context.save(d->stateData);
}

void RunnerManager::launchQuery(const QString &term)
{
    launchQuery(term, QString());
}

void RunnerManager::launchQuery(const QString &untrimmedTerm, const QString &runnerName)
{
    setupMatchSession();
    QString term = untrimmedTerm.trimmed();
    const QString prevSingleRunner = d->singleModeRunnerId;
    d->untrimmedTerm = untrimmedTerm;

    // Set the required values and load the runner
    d->singleModeRunnerId = runnerName;
    d->singleMode = !runnerName.isEmpty();
    d->loadSingleRunner();
    // If we could not load the single runner we reset
    if (!runnerName.isEmpty() && !d->currentSingleRunner) {
        reset();
        return;
    }

    if (d->context.query() == term && prevSingleRunner == runnerName) {
        // we already are searching for this!
        return;
    }

    if (!d->singleMode && d->runners.isEmpty()) {
        d->loadRunners();
    }

    reset();
    d->context.setQuery(term);
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
    d->context.setEnabledCategories(d->enabledCategories);
#endif

    QHash<QString, AbstractRunner *> runnable;

    // if the name is not empty we will launch only the specified runner
    if (d->singleMode) {
        runnable.insert(QString(), d->currentSingleRunner);
        d->context.setSingleRunnerQueryMode(true);
    } else {
        runnable = d->runners;
    }

    const int queryLetterCount = term.count();
    for (Plasma::AbstractRunner *r : std::as_const(runnable)) {
        if (r->isMatchingSuspended()) {
            continue;
        }
        // If this runner is loaded but disabled
        if (!d->singleMode && d->disabledRunnerIds.contains(r->id())) {
            continue;
        }
        // The runners can set the min letter count as a property, this way we don't
        // have to spawn threads just for the runner to reject the query, because it is too short
        if (!d->singleMode && queryLetterCount < r->minLetterCount()) {
            continue;
        }
        // If the runner has one ore more trigger words it can set the matchRegex to prevent
        // thread spawning if the pattern does not match
        if (!d->singleMode && r->hasMatchRegex() && !r->matchRegex().match(term).hasMatch()) {
            continue;
        }

        d->startJob(r);
    }
    // In the unlikely case that no runner gets queried we have to emit the signals here
    if (d->searchJobs.isEmpty()) {
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT matchesChanged({});
            Q_EMIT queryFinished();
        });
    }

    // Start timer to unblock slow runners
    d->delayTimer.start(RunnerManagerPrivate::slowRunDelay);
}

QString RunnerManager::query() const
{
    return d->context.query();
}

QStringList RunnerManager::history() const
{
    return d->readHistoryForCurrentActivity();
}

void RunnerManager::removeFromHistory(int index)
{
    QStringList changedHistory = history();
    if (index < changedHistory.length()) {
        changedHistory.removeAt(index);
        d->writeActivityHistory(changedHistory);
    }
}

QString RunnerManager::getHistorySuggestion(const QString &typedQuery) const
{
    const QStringList historyList = history();
    for (const QString &entry : historyList) {
        if (entry.startsWith(typedQuery, Qt::CaseInsensitive)) {
            return entry;
        }
    }
    return QString();
}

void RunnerManager::reset()
{
    // If ThreadWeaver is idle, it is safe to clear previous jobs
    if (Queue::instance()->isIdle()) {
        d->oldSearchJobs.clear();
    } else {
        for (auto it = d->searchJobs.constBegin(); it != d->searchJobs.constEnd(); ++it) {
            Queue::instance()->dequeue((*it));
        }
        d->oldSearchJobs += d->searchJobs;
    }

    d->searchJobs.clear();

    d->context.reset();
    if (!d->oldSearchJobs.empty()) {
        Q_EMIT queryFinished();
    }
}

KPluginMetaData RunnerManager::convertDBusRunnerToJson(const QString &filename) const
{
    return parseMetaDataFromDesktopFile(filename);
}

void RunnerManager::enableKNotifyPluginWatcher()
{
    if (!d->watcher) {
        d->watcher = KConfigWatcher::create(d->configPrt);
        connect(d->watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &changedNames) {
            const QString groupName = group.name();
            if (groupName == QLatin1String("Plugins")) {
                reloadConfiguration();
            } else if (groupName == QLatin1String("Runners")) {
                for (auto *runner : std::as_const(d->runners)) {
                    // Signals from the KCM contain the component name, which is the X-KDE-PluginInfo-Name property
                    if (changedNames.contains(runner->metadata(RunnerReturnPluginMetaData).pluginId().toUtf8())) {
                        runner->reloadConfiguration();
                    }
                }
            } else if (group.parent().isValid() && group.parent().name() == QLatin1String("Runners")) {
                for (auto *runner : std::as_const(d->runners)) {
                    // If the same config group has been modified which gets created in AbstractRunner::config()
                    if (groupName == runner->id()) {
                        runner->reloadConfiguration();
                    }
                }
            }
        });
    }
}

QString RunnerManager::priorSearch() const
{
    return d->priorSearch.value(d->getActivityKey());
}

void RunnerManager::setPriorSearch(const QString &search)
{
    d->priorSearch.insert(d->getActivityKey(), search);
}

bool RunnerManager::historyEnabled()
{
    return d->historyEnabled;
}

bool RunnerManager::retainPriorSearch()
{
    return d->retainPriorSearch;
}

} // Plasma namespace

#include "moc_runnermanager.cpp"
