/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2007, 2009 Ryan P. Bitanga <ryan.bitanga@gmail.com>
    SPDX-FileCopyrightText: 2008 Jordi Polo <mumismo@gmail.com>
    SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnauŋmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnermanager.h"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QMutableListIterator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

#include <KConfigWatcher>
#include <KFileUtils>
#include <KPluginMetaData>
#include <KSharedConfig>

#include "config.h"
#if HAVE_KACTIVITIES
#include <KActivities/Consumer>
#endif

#include "abstractrunner_p.h"
#include "dbusrunner_p.h"
#include "kpluginmetadata_utils_p.h"
#include "krunner_debug.h"
#include "querymatch.h"

struct MatchConnection {
    QString runnerId;
    QString query;
};
inline size_t qHash(const MatchConnection &con, size_t seed)
{
    return qHash(con.query, qHash(con.runnerId, seed));
}
inline bool operator==(const MatchConnection &lhs, const MatchConnection &rhs)
{
    return lhs.runnerId == rhs.runnerId && lhs.query == rhs.query;
}

QDebug operator<<(QDebug debug, const QSet<MatchConnection> &connections)
{
    for (const auto &connection : connections) {
        debug << QStringList{connection.runnerId, connection.query};
    }
    return debug;
}

namespace KRunner
{
class RunnerManagerPrivate
{
public:
    RunnerManagerPrivate(const KConfigGroup &configurationGroup, KConfigGroup stateConfigGroup, RunnerManager *parent)
        : q(parent)
        , pluginConf(configurationGroup)
        , stateData(stateConfigGroup)
    {
        initializeKNotifyPluginWatcher();
        matchChangeTimer.setSingleShot(true);
        matchChangeTimer.setTimerType(Qt::TimerType::PreciseTimer); // Without this, autotest will fail due to imprecision of this timer

        QObject::connect(&matchChangeTimer, &QTimer::timeout, q, [this]() {
            matchesChanged();
        });
        QObject::connect(&context, &RunnerContext::matchesChanged, q, [this]() {
            scheduleMatchesChanged();
        });

        // Set up tracking of the last time matchesChanged was signalled
        lastMatchChangeSignalled.start();
        QObject::connect(q, &RunnerManager::matchesChanged, q, [&] {
            lastMatchChangeSignalled.restart();
        });
        loadConfiguration();
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
        const KConfigGroup generalConfig = pluginConf.config()->group("General");
        activityAware = generalConfig.readEntry("ActivityAware", true);
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

    void deleteRunners(QList<AbstractRunner *> runners)
    {
        for (const auto runner : runners) {
            runner->setParent(nullptr); // The thread is not stopped yet!
            runner->thread()->quit();
            // Clean up the thread and runner objects
            for (auto action : std::as_const(runner->d->internalActionsList)) {
                action->moveToThread(runner->thread());
                action->setParent(runner);
            }
            QObject::connect(runner->thread(), &QThread::finished, runner->thread(), &QObject::deleteLater);
            QObject::connect(runner->thread(), &QThread::finished, runner, &QObject::deleteLater);
        }
    }

    void loadRunners(const QString &singleRunnerId = QString())
    {
        QList<KPluginMetaData> offers = RunnerManager::runnerMetaDataList();

        const bool loadAll = stateData.readEntry("loadAll", false);
        const bool noWhiteList = whiteList.isEmpty();

        QList<AbstractRunner *> deadRunners;
        QMutableListIterator<KPluginMetaData> it(offers);
        while (it.hasNext()) {
            const KPluginMetaData &description = it.next();
            qCDebug(KRUNNER) << "Loading runner: " << description.pluginId();

            const QString runnerName = description.pluginId();
            const bool isPluginEnabled = description.isEnabled(pluginConf);
            const bool loaded = runners.contains(runnerName);
            bool selected = loadAll || disabledRunnerIds.contains(runnerName) || (isPluginEnabled && (noWhiteList || whiteList.contains(runnerName)));
            if (!selected && runnerName == singleRunnerId) {
                selected = true;
                disabledRunnerIds << runnerName;
            }

            if (selected) {
                if (!loaded) {
                    if (auto runner = loadInstalledRunner(description)) {
                        qCDebug(KRUNNER) << "Loaded:" << runnerName;
                        runners.insert(runnerName, runner);
                    }
                }
            } else if (loaded) {
                // Remove runner
                deadRunners.append(runners.take(runnerName));
                qCDebug(KRUNNER) << "Plugin disabled. Removing runner: " << runnerName;
            }
        }

        deleteRunners(deadRunners);
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
            auto res = KPluginFactory::instantiatePlugin<AbstractRunner>(pluginMetaData, q);
            if (res) {
                runner = res.plugin;
            } else {
                qCWarning(KRUNNER).nospace() << "Could not load runner " << pluginMetaData.name() << ":" << res.errorString
                                             << " (library path was:" << pluginMetaData.fileName() << ")";
            }
        } else if (api.startsWith(QLatin1String("DBus"))) {
            runner = new DBusRunner(q, pluginMetaData);
        } else {
            qCWarning(KRUNNER) << "Unknown X-Plasma-API requested for runner" << pluginMetaData.fileName();
            return nullptr;
        }

        if (runner) {
            QObject::connect(runner, &AbstractRunner::matchingSuspended, q, [this](bool state) {
                runnerMatchingSuspended(state);
            });
            auto thread = new QThread();
            thread->start();
            // By now, runners who do "qobject_cast<Krunner::RunnerManager*>(parent)" should have saved the value
            // By setting the parrent to a nullptr, we are allowed to move the object to another thread
            runner->setParent(nullptr);
            runner->d->internalActionsList = runner->findChildren<QAction *>(Qt::FindDirectChildrenOnly);
            for (auto action : std::as_const(runner->d->internalActionsList)) {
                action->setParent(nullptr);
            }
            runner->moveToThread(thread);

            // The runner might outlive the manager due to us waiting for the thread to exit
            q->connect(runner, &AbstractRunner::matchInternalFinished, q, [this, runner](const QString &query) {
                const MatchConnection matchConnection{runner->metadata().pluginId(), query};
                if (oldConnections.remove(matchConnection)) {
                    return;
                }
                if (currentConnections.remove(matchConnection) && currentConnections.isEmpty()) {
                    // If there are any new matches scheduled to be notified, we should anticipate it and just refresh right now
                    if (matchChangeTimer.isActive()) {
                        matchChangeTimer.stop();
                        Q_EMIT q->matchesChanged(context.matches());
                    } else if (context.matches().isEmpty()) {
                        // we finished our runa, and there are no valid matches, and so no
                        // signal will have been sent out. so we need to emit the signal
                        // ourselves here
                        Q_EMIT q->matchesChanged(context.matches());
                    }
                    Q_EMIT q->queryFinished();
                }
            });

            QMetaObject::invokeMethod(runner, &AbstractRunner::init);
            if (prepped) {
                Q_EMIT runner->prepare();
            }
        }

        return runner;
    }

    void teardown()
    {
        if (!prepped) {
            return;
        }

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
    }

    void runnerMatchingSuspended(bool suspended)
    {
        auto *runner = qobject_cast<AbstractRunner *>(q->sender());
        if (suspended || !prepped || !runner) {
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
        QMetaObject::invokeMethod(runner, "matchInternal", Q_ARG(KRunner::RunnerContext, context));
        currentConnections.insert(MatchConnection{runner->id(), context.query()});
    }

    // Must only be called once
    void initializeKNotifyPluginWatcher()
    {
        Q_ASSERT(!watcher);
        watcher = KConfigWatcher::create(KSharedConfig::openConfig(pluginConf.config()->name()));
        q->connect(watcher.data(), &KConfigWatcher::configChanged, q, [this](const KConfigGroup &group, const QByteArrayList &changedNames) {
            const QString groupName = group.name();
            if (groupName == QLatin1String("Plugins")) {
                q->reloadConfiguration();
            } else if (groupName == QLatin1String("Runners")) {
                for (auto *runner : std::as_const(runners)) {
                    // Signals from the KCM contain the component name, which is the X-KDE-PluginInfo-Name property
                    if (changedNames.contains(runner->metadata().pluginId().toUtf8())) {
                        runner->reloadConfiguration();
                    }
                }
            } else if (group.parent().isValid() && group.parent().name() == QLatin1String("Runners")) {
                for (auto *runner : std::as_const(runners)) {
                    // If the same config group has been modified which gets created in AbstractRunner::config()
                    if (groupName == runner->id()) {
                        runner->reloadConfiguration();
                    }
                }
            }
        });
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

    RunnerManager *const q;
    RunnerContext context;
    QTimer matchChangeTimer;
    QElapsedTimer lastMatchChangeSignalled;
    QHash<QString, AbstractRunner *> runners;
    AbstractRunner *currentSingleRunner = nullptr;
    QSet<MatchConnection> currentConnections;
    QSet<MatchConnection> oldConnections;
    QStringList enabledCategories;
    QString singleModeRunnerId;
    bool prepped = false;
    bool allRunnersPrepped = false;
    bool singleRunnerPrepped = false;
    bool singleMode = false;
    bool activityAware = false;
    bool historyEnabled = true;
    QStringList whiteList;
    KConfigWatcher::Ptr watcher;
    QString untrimmedTerm;
    const QString nulluuid = QStringLiteral("00000000-0000-0000-0000-000000000000");
    KConfigGroup pluginConf;
    KConfigGroup stateData;
    QSet<QString> disabledRunnerIds; // Runners that are disabled but were loaded as single runners
#if HAVE_KACTIVITIES
    const KActivities::Consumer activitiesConsumer;
#endif
};

RunnerManager::RunnerManager(const KConfigGroup &pluginConfigGroup, KConfigGroup stateConfigGroup, QObject *parent)
    : QObject(parent)
    , d(new RunnerManagerPrivate(pluginConfigGroup, stateConfigGroup, this))
{
    Q_ASSERT(pluginConfigGroup.isValid());
    Q_ASSERT(stateConfigGroup.isValid());
}

RunnerManager::RunnerManager(QObject *parent)
    : QObject(parent)
{
    auto defaultStatePtr = KSharedConfig::openConfig(QStringLiteral("krunnerstaterc"), KConfig::NoGlobals, QStandardPaths::GenericDataLocation);
    auto configPtr = KSharedConfig::openConfig(QStringLiteral("krunnerrc"), KConfig::NoGlobals);
    d.reset(new RunnerManagerPrivate(configPtr->group("Plugins"), defaultStatePtr->group("PlasmaRunnerManager"), this));
}

RunnerManager::~RunnerManager()
{
    d->context.reset();
    d->deleteRunners(d->runners.values());
}

void RunnerManager::reloadConfiguration()
{
    d->pluginConf.config()->reparseConfiguration();
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

AbstractRunner *RunnerManager::loadRunner(const KPluginMetaData &pluginMetaData)
{
    const QString runnerId = pluginMetaData.pluginId();
    if (auto loadedRunner = d->runners.value(runnerId)) {
        return loadedRunner;
    }
    if (!runnerId.isEmpty()) {
        if (AbstractRunner *runner = d->loadInstalledRunner(pluginMetaData)) {
            d->runners.insert(runnerId, runner);
            return runner;
        }
    }
    return nullptr;
}

AbstractRunner *RunnerManager::runner(const QString &pluginId) const
{
    if (d->runners.isEmpty()) {
        d->loadRunners();
    }

    return d->runners.value(pluginId, nullptr);
}

QList<AbstractRunner *> RunnerManager::runners() const
{
    return d->runners.values();
}

RunnerContext *RunnerManager::searchContext() const
{
    return &d->context;
}

QList<QueryMatch> RunnerManager::matches() const
{
    return d->context.matches();
}

bool RunnerManager::run(const QueryMatch &match, QAction *selectedAction)
{
    if (!match.isValid() || !match.isEnabled()) { // The model should prevent this
        return false;
    }

    // Modify the match and run it
    QueryMatch m = match;
    m.setSelectedAction(selectedAction);
    m.runner()->run(d->context, m);
    // To allow the RunnerContext to increase the relevance of often launched apps
    d->context.increaseLaunchCount(m);

    if (!d->context.shouldIgnoreCurrentMatchForHistory()) {
        d->addToHistory();
    }
    if (d->context.requestedQueryString().isEmpty()) {
        return true;
    } else {
        Q_EMIT requestUpdateQueryString(d->context.requestedQueryString(), d->context.requestedCursorPosition());
        return false;
    }
}

QList<QAction *> RunnerManager::actionsForMatch(const QueryMatch &match)
{
    return match.isValid() ? match.actions() : QList<QAction *>();
}

QMimeData *RunnerManager::mimeDataForMatch(const QueryMatch &match) const
{
    return match.isValid() ? match.runner()->mimeDataForMatch(match) : nullptr;
}

QList<KPluginMetaData> RunnerManager::runnerMetaDataList()
{
    QList<KPluginMetaData> pluginMetaDatas = KPluginMetaData::findPlugins(QStringLiteral("kf6/krunner"));
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

    return pluginMetaDatas;
}

void RunnerManager::setupMatchSession()
{
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

    d->teardown();
    // We save the context config after each session, just like the history entries
    // BUG: 424505
    d->context.save(d->stateData);
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

    QHash<QString, AbstractRunner *> runnable;

    // if the name is not empty we will launch only the specified runner
    if (d->singleMode) {
        runnable.insert(QString(), d->currentSingleRunner);
        d->context.setSingleRunnerQueryMode(true);
    } else {
        runnable = d->runners;
    }

    const int queryLetterCount = term.length();
    for (KRunner::AbstractRunner *r : std::as_const(runnable)) {
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
    if (d->currentConnections.isEmpty()) {
        QTimer::singleShot(0, this, [this]() {
            Q_EMIT matchesChanged({});
            Q_EMIT queryFinished();
        });
    }
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
    d->oldConnections.unite(d->currentConnections);
    d->currentConnections.clear();

    d->context.reset();
    if (!d->oldConnections.empty()) {
        Q_EMIT queryFinished();
    }
}

KPluginMetaData RunnerManager::convertDBusRunnerToJson(const QString &filename) const
{
    return parseMetaDataFromDesktopFile(filename);
}

bool RunnerManager::historyEnabled()
{
    return d->historyEnabled;
}

void RunnerManager::setHistoryEnabled(bool enabled)
{
    d->historyEnabled = enabled;
    Q_EMIT historyEnabledChanged();
}

} // KRunner namespace

#include "moc_runnermanager.cpp"
