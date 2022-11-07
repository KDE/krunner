/*
    SPDX-FileCopyrightText: 2022 Eduardo Cruz <eduardo.cruz@kdemail.net>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "runnermanager.h"

#include <KSharedConfig>
#include <QAction>
#include <QCoreApplication>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTest>

#include "abstractrunnertest.h"
#include "kpluginmetadata_utils_p.h"

Q_DECLARE_METATYPE(Plasma::QueryMatch)
Q_DECLARE_METATYPE(QList<Plasma::QueryMatch>)

using namespace Plasma;

class RunnerManagerTest : public AbstractRunnerTest
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        startDBusRunnerProcess({QStringLiteral("net.krunnertests.dave")});
        qputenv("XDG_DATA_DIRS", QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).toLocal8Bit());
        QCoreApplication::setLibraryPaths(QStringList());
        initProperties();
        auto md = parseMetaDataFromDesktopFile(QFINDTESTDATA("dbusrunnertestmulti.desktop"));
        QVERIFY(md.isValid());
        manager->loadRunner(md);
        qRegisterMetaType<QList<Plasma::QueryMatch>>();
    }

    void cleanupTestCase()
    {
        killRunningDBusProcesses();
    }

    /**
     * This will test the mechanismm that stalls for 250ms before emiting any result in RunnerManager::scheduleMatchesChanged()
     * and the mechanism that anticipates the last results emission in RunnerManager::jobDone().
     */
    void testScheduleMatchesChanged()
    {
        QSignalSpy spyQueryFinished(manager.get(), &Plasma::RunnerManager::queryFinished);
        QSignalSpy spyMatchesChanged(manager.get(), &Plasma::RunnerManager::matchesChanged);

        QVERIFY(spyQueryFinished.isValid());
        QVERIFY(spyMatchesChanged.isValid());

        QCOMPARE(spyQueryFinished.count(), 0);

        // This will track the total execution time
        QElapsedTimer timer;
        timer.start();

        // This special string will simulate a 300ms delay
        manager->launchQuery("fooDelay300");

        // However not yet a matcheschanged, it should be stalled for 250ms
        QCOMPARE(spyMatchesChanged.count(), 0);

        // After 250ms it will emit with empty matches, we wait for that.
        // We can't put a low upper limit on these wait() calls because the CI environment can be slow.
        QVERIFY(spyMatchesChanged.wait()); // This should take just a tad longer than 250ms.

        // This should have taken no less than 250ms. It waits for 250s before "giving up" and emitting an empty matches list.
        QVERIFY(timer.elapsed() >= 250);
        QCOMPARE(spyMatchesChanged.count(), 1);
        QCOMPARE(manager->matches().count(), 0); // This is the empty matches "reset" emission, result is not ready yet
        QCOMPARE(spyQueryFinished.count(), 0); // Still the same, query is not done

        // We programmed it to emit the result after 300ms, so we need to wait 50ms more for the next emission
        QVERIFY(spyQueryFinished.wait());

        // This should have taken at least 300ms total, as we requested via the special query string
        QVERIFY(timer.elapsed() >= 300);

        // At this point RunnerManager::jobDone() should have anticipated the final emission.
        QCOMPARE(spyMatchesChanged.count(), 2); // We had the second matchesChanged emission, now with the query result
        QCOMPARE(manager->matches().count(), 1); // The result is here
        QCOMPARE(spyQueryFinished.count(), 1); // Will have emited queryFinished, job is done

        // Now we will make sure that RunnerManager::scheduleMatchesChanged() emits matchesChanged instantly
        // if we start a query with an empty string. It will never produce results, stalling is meaningless
        manager->launchQuery("");
        QCOMPARE(spyMatchesChanged.count(), 3); // One more, instantly, without stall
        QCOMPARE(manager->matches().count(), 0); // Empty results for empty query string
        QVERIFY(spyQueryFinished.wait());
    }

    /**
     * This will test queryFinished signal from reset() is emitted when the previous runners are
     * still running.
     */
    void testQueryFinishedFromReset()
    {
        QSignalSpy spyQueryFinished(manager.get(), &Plasma::RunnerManager::queryFinished);

        manager->launchQuery("fooDelay1000");
        QCOMPARE(spyQueryFinished.size(), 0);

        manager->launchQuery("fooDelay300");
        QCOMPARE(spyQueryFinished.size(), 1); // From reset()

        QVERIFY(spyQueryFinished.wait());
        QCOMPARE(spyQueryFinished.size(), 2);
    }

    /**
     * When we delete the RunnerManager while a job is still running, we should not crash
     */
    void testNotCrashWhenDeletingRunnerManager()
    {
        RunnerManager manager;
        manager.setAllowedRunners({QStringLiteral("fakerunnerplugin")});
        manager.loadRunner(KPluginMetaData::findPluginById(QStringLiteral("krunnertest"), QStringLiteral("fakerunnerplugin")));

        QCOMPARE(manager.runners().size(), 1);

        manager.launchQuery("somequery");
    }
};

QTEST_MAIN(RunnerManagerTest)

#include "runnermanagertest.moc"
