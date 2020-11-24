/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2007, 2009 Ryan P. Bitanga <ryan.bitanga@gmail.com>
    SPDX-FileCopyrightText: 2008 Jordi Polo <mumismo@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnermanager.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>

#include <KSharedConfig>
#include <KServiceTypeTrader>
#include <KPluginMetaData>
#include <KConfigWatcher>

#include <ThreadWeaver/DebuggingAids>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Thread>

#include <plasma/version.h>

#include "dbusrunner_p.h"
#include "runnerjobs_p.h"
#include "querymatch.h"
#include "krunner_debug.h"

using ThreadWeaver::Queue;
using ThreadWeaver::Job;

namespace Plasma
{

void forEachDBusPlugin(std::function<void(const KPluginMetaData &, bool *)> callback)
{
    const QStringList dBusPlugindirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("krunner/dbusplugins"), QStandardPaths::LocateDirectory);
    const QStringList serviceTypeFiles(QStringLiteral("plasma-runner.desktop"));
    for (const QString &dir : dBusPlugindirs) {
        const QStringList desktopFiles = QDir(dir).entryList(QStringList(QStringLiteral("*.desktop")));
        for (const QString &file : desktopFiles) {
            const QString desktopFilePath = dir + QLatin1Char('/') + file;
            KPluginMetaData pluginMetaData = KPluginMetaData::fromDesktopFile(desktopFilePath, serviceTypeFiles);
            if (pluginMetaData.isValid()) {
                bool cancel = false;
                callback(pluginMetaData, &cancel);
                if (cancel) {
                    return;
                }
            }
        }
    }
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
void warnAboutDeprecatedMetaData(const KPluginInfo &pluginInfo)
{
    if (!pluginInfo.libraryPath().isEmpty()) {
        qCWarning(KRUNNER).nospace() << "KRunner plugin " << pluginInfo.pluginName() << " still uses a .desktop file ("
        << pluginInfo.entryPath() << "). Please port it to JSON metadata.";
    } else {
        qCWarning(KRUNNER).nospace() << "KRunner D-Bus plugin " << pluginInfo.pluginName() << " installs the .desktop file ("
        << pluginInfo.entryPath() << ") still in the kservices5 folder. Please install it to ${KDE_INSTALL_DATAROOTDIR}/krunner/dbusplugins.";
    }
}
#endif

/*****************************************************
*  RunnerManager::Private class
*
*****************************************************/
class RunnerManagerPrivate
{
public:

    RunnerManagerPrivate(RunnerManager *parent)
      : q(parent),
        deferredRun(nullptr),
        currentSingleRunner(nullptr),
        prepped(false),
        allRunnersPrepped(false),
        singleRunnerPrepped(false),
        teardownRequested(false),
        singleMode(false),
        singleRunnerWasLoaded(false)
    {
        matchChangeTimer.setSingleShot(true);
        delayTimer.setSingleShot(true);

        QObject::connect(&matchChangeTimer, SIGNAL(timeout()), q, SLOT(matchesChanged()));
        QObject::connect(&context, SIGNAL(matchesChanged()), q, SLOT(scheduleMatchesChanged()));
        QObject::connect(&delayTimer, SIGNAL(timeout()), q, SLOT(unblockJobs()));

        // Set up tracking of the last time matchesChanged was signalled
        lastMatchChangeSignalled.start();
        QObject::connect(q, &RunnerManager::matchesChanged, q, [&] { lastMatchChangeSignalled.restart(); });
    }

    void scheduleMatchesChanged()
    {
        if(lastMatchChangeSignalled.hasExpired(250)) {
            matchChangeTimer.stop();
            Q_EMIT q->matchesChanged(context.matches());
        } else {
            matchChangeTimer.start(250 - lastMatchChangeSignalled.elapsed());
        }
    }

    void matchesChanged()
    {
        Q_EMIT q->matchesChanged(context.matches());
    }

    void loadConfiguration()
    {
        KConfigGroup config = configGroup();

        // Limit the number of instances of a single normal speed runner and all of the slow runners
        // to half the number of threads
        DefaultRunnerPolicy::instance().setCap(qMax(2, Queue::instance()->maximumNumberOfThreads() / 2));

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
        enabledCategories = config.readEntry("enabledCategories", QStringList());
#endif
        context.restore(config);
    }

    KConfigGroup configGroup()
    {
        return conf.isValid() ? conf : KConfigGroup(KSharedConfig::openConfig(configFile), "PlasmaRunnerManager");
    }

    void clearSingleRunner()
    {
        if (singleRunnerWasLoaded) {
            delete currentSingleRunner;
        }

        currentSingleRunner = nullptr;
    }

    void loadSingleRunner()
    {
        if (!singleMode || singleModeRunnerId.isEmpty()) {
            clearSingleRunner();
            return;
        }

        if (currentSingleRunner) {
            if (currentSingleRunner->id() == singleModeRunnerId) {
                return;
            }

            clearSingleRunner();
        }

        if (AbstractRunner *loadedRunner = q->runner(singleModeRunnerId)) {
            singleRunnerWasLoaded = false;
            currentSingleRunner = loadedRunner;
            return;
        }

        KPluginMetaData pluginMetaData;
        // binary plugins
        const auto plugins = KPluginLoader::findPluginsById(QStringLiteral("kf5/krunner"), singleModeRunnerId);
        if (!plugins.isEmpty()) {
            pluginMetaData = plugins[0];
        } else {
            // get D-Bus plugins
            forEachDBusPlugin([&](const KPluginMetaData &md, bool *cancel) {
                if (md.pluginId() == singleModeRunnerId) {
                    pluginMetaData = md;
                    *cancel = true;
                }
            });
        }

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
        // also search for deprecated kservice-based KRunner plugins metadata
        if (!pluginMetaData.isValid()) {
            const KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("Plasma/Runner"), QStringLiteral("[X-KDE-PluginInfo-Name] == '%1'").arg(singleModeRunnerId));
            if (!offers.isEmpty()) {
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
                const KPluginInfo pluginInfo(offers[0]);
                warnAboutDeprecatedMetaData(pluginInfo);
                pluginMetaData = pluginInfo.toMetaData();
QT_WARNING_POP
            }
        }
#endif
        if (pluginMetaData.isValid()) {
            currentSingleRunner = loadInstalledRunner(pluginMetaData);

            if (currentSingleRunner) {
                Q_EMIT currentSingleRunner->prepare();
                singleRunnerWasLoaded = true;
            }
        }
    }

    void loadRunners()
    {
        KConfigGroup config = configGroup();
        QVector<KPluginMetaData> offers = RunnerManager::runnerMetaDataList();

        const bool loadAll = config.readEntry("loadAll", false);
        const bool noWhiteList = whiteList.isEmpty();
        KConfigGroup pluginConf;
        if (conf.isValid()) {
            pluginConf = KConfigGroup(&conf, "Plugins");
        } else {
            pluginConf = KConfigGroup(KSharedConfig::openConfig(configFile), "Plugins");
        }

        advertiseSingleRunnerIds.clear();

        QStringList allCategories;
        QSet<AbstractRunner *> deadRunners;
        QMutableVectorIterator<KPluginMetaData> it(offers);
        while (it.hasNext()) {
            KPluginMetaData &description = it.next();
            qCDebug(KRUNNER) << "Loading runner: " << description.pluginId();

            const QString tryExec = description.value(QStringLiteral("TryExec"));
            if (!tryExec.isEmpty() && QStandardPaths::findExecutable(tryExec).isEmpty()) {
                // we don't actually have this application!
                continue;
            }

            const QString runnerName = description.pluginId();
            const bool isPluginEnabled = pluginConf.readEntry(runnerName + QLatin1String("Enabled"), description.isEnabledByDefault());
            const bool loaded = runners.contains(runnerName);
            const bool selected = loadAll || (isPluginEnabled && (noWhiteList || whiteList.contains(runnerName)));

            const bool singleQueryModeEnabled = description.rawData().value(QStringLiteral("X-Plasma-AdvertiseSingleRunnerQueryMode")).toVariant().toBool();

            if (singleQueryModeEnabled) {
                advertiseSingleRunnerIds.insert(runnerName, description.name());
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
                    allCategories << categories;

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
                //Remove runner
                deadRunners.insert(runners.take(runnerName));
                qCDebug(KRUNNER) << "Plugin disabled. Removing runner: " << runnerName;
            }
        }

        if (enabledCategories.isEmpty()) {
            enabledCategories = allCategories;
        }

        if (!deadRunners.isEmpty()) {
                QSet<QSharedPointer<FindMatchesJob> > deadJobs;
                auto it = searchJobs.begin();
                while (it != searchJobs.end()) {
                    auto &job = (*it);
                    if (deadRunners.contains(job->runner())) {
                        QObject::disconnect(job.data(), SIGNAL(done(ThreadWeaver::JobPointer)), q, SLOT(jobDone(ThreadWeaver::JobPointer)));
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

        if (!singleRunnerWasLoaded) {
            // in case we deleted it up above
            clearSingleRunner();
        }

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
            KPluginLoader pluginLoader(pluginMetaData.fileName());
            const quint64 pluginVersion = pluginLoader.pluginVersion();
            if (Plasma::isPluginVersionCompatible(pluginVersion)) {
                if (KPluginFactory *factory = pluginLoader.factory()) {
                    const QVariantList args {
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 77)
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
                        pluginMetaData.metaDataFileName(),
#endif
                        QVariant::fromValue(pluginMetaData),
#endif
                    };
                    runner = factory->create<AbstractRunner>(q, args);
                } else {
                    qCWarning(KRUNNER).nospace() << "Could not load runner " << pluginMetaData.name() << ":"
                        << pluginLoader.errorString() << " (library path was:" << pluginMetaData.fileName() << ")";
                }
            } else {
                const QString runnerVersion = QStringLiteral("%1.%2.%3").arg(pluginVersion >> 16).arg((pluginVersion >> 8) & 0x00ff).arg(pluginVersion & 0x0000ff);
                qCWarning(KRUNNER) << "Cannot load runner" << pluginMetaData.name() <<"- versions mismatch: KRunner"
                    << Plasma::versionString()<< "," << pluginMetaData.name() << runnerVersion;
            }
        } else if (api == QLatin1String("DBus")){
            runner = new DBusRunner(pluginMetaData, q);
        } else {
            runner = new AbstractRunner(pluginMetaData, q);
        }

        if (runner) {
            QObject::connect(runner, SIGNAL(matchingSuspended(bool)), q, SLOT(runnerMatchingSuspended(bool)));
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

        if (deferredRun.isEnabled() && runJob->runner() == deferredRun.runner()) {
            QueryMatch tmpRun = deferredRun;
            deferredRun = QueryMatch(nullptr);
            tmpRun.run(context);
        }

        searchJobs.remove(runJob);
        oldSearchJobs.remove(runJob);

        if (searchJobs.isEmpty() && context.matches().isEmpty()) {
            // we finished our run, and there are no valid matches, and so no
            // signal will have been sent out. so we need to emit the signal
            // ourselves here
            Q_EMIT q->matchesChanged(context.matches());
        }
        if (searchJobs.isEmpty()) {
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
                for (AbstractRunner *runner : qAsConst(runners)) {
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
        if (suspended || !prepped || teardownRequested) {
            return;
        }

        if (auto *runner = qobject_cast<AbstractRunner *>(q->sender())) {
            startJob(runner);
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
            QObject::connect(job.data(), SIGNAL(done(ThreadWeaver::JobPointer)), q, SLOT(jobDone(ThreadWeaver::JobPointer)));
            if (runner->speed() == AbstractRunner::SlowSpeed) {
                job->setDelayTimer(&delayTimer);
            }
            Queue::instance()->enqueue(job);
            searchJobs.insert(job);
    }

    // Delay in ms before slow runners are allowed to run
    static const int slowRunDelay = 400;

    RunnerManager *const q;
    QueryMatch deferredRun;
    RunnerContext context;
    QTimer matchChangeTimer;
    QTimer delayTimer; // Timer to control when to run slow runners
    QElapsedTimer lastMatchChangeSignalled;
    QHash<QString, AbstractRunner*> runners;
    QHash<QString, QString> advertiseSingleRunnerIds;
    AbstractRunner* currentSingleRunner;
    QSet<QSharedPointer<FindMatchesJob> > searchJobs;
    QSet<QSharedPointer<FindMatchesJob> > oldSearchJobs;
    KConfigGroup conf;
    QStringList enabledCategories;
    QString singleModeRunnerId;
    bool prepped : 1;
    bool allRunnersPrepped : 1;
    bool singleRunnerPrepped : 1;
    bool teardownRequested : 1;
    bool singleMode : 1;
    bool singleRunnerWasLoaded : 1;
    QStringList whiteList;
    QString configFile;
    KConfigWatcher::Ptr watcher;
};

/*****************************************************
*  RunnerManager::Public class
*
*****************************************************/
RunnerManager::RunnerManager(QObject *parent)
    : RunnerManager(QString(), parent)
{
}

RunnerManager::RunnerManager(const QString &configFile, QObject *parent)
    : QObject(parent),
      d(new RunnerManagerPrivate(this))
{
    d->configFile = configFile;
    d->loadConfiguration();
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
RunnerManager::RunnerManager(KConfigGroup &c, QObject *parent)
    : QObject(parent),
      d(new RunnerManagerPrivate(this))
{
    d->conf = KConfigGroup(&c, "PlasmaRunnerManager");
    d->loadConfiguration();
}
#endif

RunnerManager::~RunnerManager()
{
    if (!qApp->closingDown() && (!d->searchJobs.isEmpty() || !d->oldSearchJobs.isEmpty())) {
        new DelayedJobCleaner(d->searchJobs + d->oldSearchJobs);
    }

    delete d;
}

void RunnerManager::reloadConfiguration()
{
    KSharedConfig::openConfig()->reparseConfiguration();
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
void RunnerManager::setEnabledCategories(const QStringList& categories)
{
    KConfigGroup config = d->configGroup();
    config.writeEntry("enabledCategories", categories);

    d->enabledCategories = categories;

    if (!d->runners.isEmpty()) {
        d->loadRunners();
    }
}
#endif

QStringList RunnerManager::allowedRunners() const
{
    KConfigGroup config = d->configGroup();
    return config.readEntry("pluginWhiteList", QStringList());
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
QStringList RunnerManager::enabledCategories() const
{
    KConfigGroup config = d->configGroup();
    QStringList list = config.readEntry("enabledCategories", QStringList());
    if (list.isEmpty()) {
        list.reserve(d->runners.count());
        for (AbstractRunner* runner : qAsConst(d->runners)) {
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

void RunnerManager::loadRunner(const QString &path)
{
    if (!d->runners.contains(path)) {
        AbstractRunner *runner = new AbstractRunner(this, path);
        connect(runner, SIGNAL(matchingSuspended(bool)), this, SLOT(runnerMatchingSuspended(bool)));
        d->runners.insert(path, runner);
    }
}

AbstractRunner* RunnerManager::runner(const QString &name) const
{
    if (d->runners.isEmpty()) {
        d->loadRunners();
    }

    return d->runners.value(name, nullptr);
}

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

QList<AbstractRunner *> RunnerManager::runners() const
{
    return d->runners.values();
}

QStringList RunnerManager::singleModeAdvertisedRunnerIds() const
{
    return d->advertiseSingleRunnerIds.keys();
}

QString RunnerManager::runnerName(const QString &id) const
{
    if (runner(id)) {
        return runner(id)->name();
    } else {
        return d->advertiseSingleRunnerIds.value(id, QString());
    }
}

RunnerContext* RunnerManager::searchContext() const
{
    return &d->context;
}

//Reordering is here so data is not reordered till strictly needed
QList<QueryMatch> RunnerManager::matches() const
{
    return d->context.matches();
}

void RunnerManager::run(const QString &matchId)
{
    run(d->context.match(matchId));
}

void RunnerManager::run(const QueryMatch &match)
{
    if (!match.isEnabled()) {
        return;
    }

    //TODO: this function is not const as it may be used for learning
    AbstractRunner *runner = match.runner();

    for (auto it = d->searchJobs.constBegin(); it != d->searchJobs.constEnd(); ++it) {
        if ((*it)->runner() == runner && !(*it)->isFinished()) {
            d->deferredRun = match;
            return;
        }
    }

    if (d->deferredRun.isValid()) {
        d->deferredRun = QueryMatch(nullptr);
    }

    d->context.run(match);
}

QList<QAction*> RunnerManager::actionsForMatch(const QueryMatch &match)
{
    if (AbstractRunner *runner = match.runner()) {
        return runner->actionsForMatch(match);
    }

    return QList<QAction*>();
}

QMimeData * RunnerManager::mimeDataForMatch(const QString &id) const
{
    return mimeDataForMatch(d->context.match(id));
}


QMimeData * RunnerManager::mimeDataForMatch(const QueryMatch &match) const
{
    return match.isValid() ? match.runner()->mimeDataForMatch(match) : nullptr;
}

QVector<KPluginMetaData> RunnerManager::runnerMetaDataList(const QString &parentApp)
{
    // get binary plugins
    // filter rule also covers parentApp.isEmpty()
    auto filterParentApp = [&parentApp](const KPluginMetaData &md) -> bool {
        return md.value(QStringLiteral("X-KDE-ParentApp")) == parentApp;
    };

    QVector<KPluginMetaData> pluginMetaDatas = KPluginLoader::findPlugins(QStringLiteral("kf5/krunner"), filterParentApp);
    QSet<QString> knownRunnerIds;
    knownRunnerIds.reserve(pluginMetaDatas.size());
    for (const KPluginMetaData &pluginMetaData : qAsConst(pluginMetaDatas)) {
        knownRunnerIds.insert(pluginMetaData.pluginId());
    }

    // get D-Bus plugins
    forEachDBusPlugin([&](const KPluginMetaData &pluginMetaData, bool *) {
        pluginMetaDatas.append(pluginMetaData);
        knownRunnerIds.insert(pluginMetaData.pluginId());
    });

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
    // also search for deprecated kservice-based KRunner plugins metadata
    const QString constraint = parentApp.isEmpty() ?
        QStringLiteral("not exist [X-KDE-ParentApp] or [X-KDE-ParentApp] == ''") :
        QStringLiteral("[X-KDE-ParentApp] == '") + parentApp + QLatin1Char('\'');

    const KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("Plasma/Runner"), constraint);
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
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
    return KPluginInfo::fromMetaData(runnerMetaDataList(parentApp));
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
        for (AbstractRunner *runner : qAsConst(d->runners)) {
            Q_EMIT runner->prepare();
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
    KConfigGroup config = d->configGroup();
    d->context.save(config);
}

void RunnerManager::launchQuery(const QString &term)
{
    launchQuery(term, QString());
}

void RunnerManager::launchQuery(const QString &untrimmedTerm, const QString &runnerName)
{
    setupMatchSession();
    QString term = untrimmedTerm.trimmed();

    setSingleModeRunnerId(runnerName);
    setSingleMode(d->currentSingleRunner);
    if (!runnerName.isEmpty() && !d->currentSingleRunner) {
        reset();
        return;
    }

    if (d->context.query() == term) {
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

    QHash<QString, AbstractRunner*> runnable;

    //if the name is not empty we will launch only the specified runner
    if (d->singleMode) {
        runnable.insert(QString(), d->currentSingleRunner);
        d->context.setSingleRunnerQueryMode(true);
    } else {
        runnable = d->runners;
    }

    const int queryLetterCount = term.count();
    for (Plasma::AbstractRunner *r : qAsConst(runnable)) {
        if (r->isMatchingSuspended()) {
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
        QTimer::singleShot(0, this,  [this](){
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

    if (d->deferredRun.isEnabled()) {
        QueryMatch tmpRun = d->deferredRun;
        d->deferredRun = QueryMatch(nullptr);
        tmpRun.run(d->context);
    }

    d->context.reset();
    Q_EMIT queryFinished();
}

void RunnerManager::enableKNotifyPluginWatcher()
{
    if (!d->watcher) {
        d->watcher = KConfigWatcher::create(KSharedConfig::openConfig(d->configGroup().config()->name()));
        connect(d->watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &changedNames) {
            const QString groupName = group.name();
            if (groupName == QLatin1String("Plugins")) {
                reloadConfiguration();
            } else if (groupName == QLatin1String("Runners")) {
                for (auto *runner : qAsConst(d->runners)) {
                    // Signals from the KCM contain the component name, which is the X-KDE-PluginInfo-Name property
                    if (changedNames.contains(runner->metadata(RunnerReturnPluginMetaData).pluginId().toUtf8())) {
                        runner->reloadConfiguration();
                    }
                }
            } else if (group.parent().isValid() && group.parent().name() == QLatin1String("Runners")){
                for (auto *runner : qAsConst(d->runners)) {
                    // If the same config group has been modified which gets created in AbstractRunner::config()
                    if (groupName == runner->id()) {
                        runner->reloadConfiguration();
                    }
                }
            }
        });
    }
}

} // Plasma namespace


#include "moc_runnermanager.cpp"
