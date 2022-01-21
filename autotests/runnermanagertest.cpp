/*
    SPDX-FileCopyrightText: 2022 Eduardo Cruz <eduardo.cruz@kdemail.net>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "runnermanager.h"
#include "fakerunner.h"

#include <KSharedConfig>
#include <QAction>
#include <QCoreApplication>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTest>

#include "abstractrunnertest.h"

Q_DECLARE_METATYPE(Plasma::QueryMatch)
Q_DECLARE_METATYPE(QList<Plasma::QueryMatch>)

using namespace Plasma;

class RunnerManagerTest : public AbstractRunnerTest
{
    Q_OBJECT
private Q_SLOTS:
    void loadRunner()
    {
        startDBusRunnerProcess({QStringLiteral("net.krunnertests.dave")});
        qputenv("XDG_DATA_DIRS", QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).toLocal8Bit());
        QCoreApplication::setLibraryPaths(QStringList());
        initProperties();
        auto md = KPluginMetaData::fromDesktopFile(QFINDTESTDATA("dbusrunnertestmulti.desktop"), {QStringLiteral("plasma-runner.desktop")});
        QVERIFY(md.isValid());
        manager->loadRunner(md);
    }

    void cleanup()
    {
        killRunningDBusProcesses();
    }

    void init()
    {
        qRegisterMetaType<QList<Plasma::QueryMatch>>();
    }

    /**
     * This will test the mechanismm that stalls for 250ms before emiting any result in RunnerManager::scheduleMatchesChanged()
     * and the mechanism that anticipates the last results emission in RunnerManager::jobDone().
     */
    void testScheduleMatchesChanged()
    {
        loadRunner();
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

        // We will have one queryFinished emission immediately signaled by RunnerManager::reset()
        QCOMPARE(spyQueryFinished.count(), 1);

        // However not yet a matcheschanged, it should be stalled for 250ms
        QCOMPARE(spyMatchesChanged.count(), 0);

        // After 250ms it will emit with empty matches, we wait for that
        QVERIFY(spyMatchesChanged.wait(265)); // 265ms as a margin of safety for 250ms

        // This should have taken no less than 250ms. It waits for 250s before "giving up" and emitting an empty matches list.
        QVERIFY(timer.elapsed() >= 250);
        QCOMPARE(spyMatchesChanged.count(), 1);
        QCOMPARE(manager->matches().count(), 0); // This is the empty matches "reset" emission, result is not ready yet
        QCOMPARE(spyQueryFinished.count(), 1); // Still the same, query is not done

        // We programmed it to emit the result after 300ms, so we need to wait 50ms more for the next emission
        QVERIFY(spyQueryFinished.wait(65)); // 65ms as a margin of safety for 50ms

        // This should have taken at least 300ms total, as we requested via the special query string
        QVERIFY(timer.elapsed() >= 300);

        // RunnerManager::jobDone() should have anticipated the final emission, so it should not have waited the full 250+250 ms.
        QVERIFY(timer.elapsed() <= 330); // This total should be just a tad bigger than 300ms, we put a 10% margin of safety

        QCOMPARE(spyMatchesChanged.count(), 2); // We had the second matchesChanged emission, now with the query result
        QCOMPARE(manager->matches().count(), 1); // The result is here
        QCOMPARE(spyQueryFinished.count(), 2); // Will have emited queryFinished, job is done

        // Now we will make sure that RunnerManager::scheduleMatchesChanged() emits matchesChanged instantly
        // if we start a query with an empty string. It will never produce results, stalling is meaninless
        manager->launchQuery("");
        QCOMPARE(spyMatchesChanged.count(), 3); // One more, instantly, without stall
        QCOMPARE(manager->matches().count(), 0); // Empty results for empty query string
    }
};

QTEST_MAIN(RunnerManagerTest)

#include "runnermanagertest.moc"
