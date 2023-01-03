/*
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_ABSTRACTRUNNERTEST_H
#define PLASMA_ABSTRACTRUNNERTEST_H

#include <KPluginMetaData>
#include <KRunner/AbstractRunner>
#include <KRunner/RunnerManager>
#include <QStandardPaths>

#include <QSignalSpy>
#include <QTest>
#if KRUNNER_DBUS_RUNNER_TESTING
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#endif

namespace
{
/**
 * This class provides a basic structure for a runner test.
 * The compile definitions should be configured using the `add_krunner_test` cmake macro
 * @since 5.80
 */
class AbstractRunnerTest : public QObject
{
public:
    using QObject::QObject;
    std::unique_ptr<Plasma::RunnerManager> manager = nullptr;
    Plasma::AbstractRunner *runner = nullptr;

    /**
     * Load the runner and set the manager and runner properties.
     */
    void initProperties()
    {
        qputenv("LC_ALL", "C.utf-8");
        manager.reset(new Plasma::RunnerManager());

#if KRUNNER_DBUS_RUNNER_TESTING
        auto md = manager->convertDBusRunnerToJson(QStringLiteral(KRUNNER_TEST_DESKTOP_FILE));
        QVERIFY(md.isValid());
        manager->loadRunner(md);
#else
        const QString pluginId = QFileInfo(QStringLiteral(KRUNNER_TEST_RUNNER_PLUGIN_NAME)).completeBaseName();
        auto metaData = KPluginMetaData::findPluginById(QStringLiteral(KRUNNER_TEST_RUNNER_PLUGIN_DIR), pluginId);
        QVERIFY2(metaData.isValid(), qPrintable("Could not find plugin " + pluginId + " in folder " + KRUNNER_TEST_RUNNER_PLUGIN_DIR));

        // Set internal variables
        manager->loadRunner(metaData);
#endif
        QCOMPARE(manager->runners().count(), 1);
        runner = manager->runners().constFirst();

        // Just make sure all went well
        QVERIFY(runner);
    }

    /**
     * Launch a query and wait for the RunnerManager to finish
     * @param query
     * @param runnerName
     */
    void launchQuery(const QString &query, const QString &runnerName = QString())
    {
        QSignalSpy spy(manager.get(), &Plasma::RunnerManager::queryFinished);
        manager->launchQuery(query, runnerName);
        QVERIFY2(spy.wait(), "RunnerManager did not emit the queryFinished signal");
    }
#if KRUNNER_DBUS_RUNNER_TESTING
    /**
     * Launch the configured DBus executable with the given arguments and wait for the process to be started.
     * @param args
     * @param waitForService Wait for this service to be registered, this will default to the service from the metadata
     * @return Process that was successfully started
     */
    QProcess *startDBusRunnerProcess(const QStringList &args = {}, const QString waitForService = QString())
    {
        qputenv("LC_ALL", "C.utf-8");
        QProcess *process = new QProcess();
        auto md = manager->convertDBusRunnerToJson(QStringLiteral(KRUNNER_TEST_DESKTOP_FILE));
        QString serviceToWatch = waitForService;
        if (serviceToWatch.isEmpty()) {
            serviceToWatch = md.value(QStringLiteral("X-Plasma-DBusRunner-Service"));
        }
        QDBusServiceWatcher watcher(serviceToWatch, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration);

        QEventLoop loop;
        connect(&watcher, &QDBusServiceWatcher::serviceRegistered, &loop, &QEventLoop::quit);
        process->start(QStringLiteral(KRUNNER_TEST_DBUS_EXECUTABLE), args);
        loop.exec();

        Q_ASSERT(process->state() == QProcess::ProcessState::Running);
        m_runningProcesses << process;
        return process;
    }

    /**
     * Kill all processes that got started with the startDBusRunnerProcess
     */
    void killRunningDBusProcesses()
    {
        for (auto &process : std::as_const(m_runningProcesses)) {
            process->kill();
            QVERIFY(process->waitForFinished());
        }
        qDeleteAll(m_runningProcesses);
        m_runningProcesses.clear();
    }

private:
    QList<QProcess *> m_runningProcesses;
#endif
};
}

#endif
