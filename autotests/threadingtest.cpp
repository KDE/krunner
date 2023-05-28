#include <KRunner/AbstractRunnerTest>
#include <KRunner/RunnerManager>
#include <qtest.h>
#include <qtestcase.h>

using namespace KRunner;

class ThreadingTest : public AbstractRunnerTest
{
    Q_OBJECT

    AbstractRunner *fakeRunner = nullptr;
private Q_SLOTS:
    void initTestCase()
    {
        initProperties();
        startDBusRunnerProcess({QStringLiteral("net.krunnertests.dave")});
        fakeRunner = manager->loadRunner(KPluginMetaData::findPluginById("krunnertest", "fakerunnerplugin"));
        QCOMPARE(manager->runners().size(), 2);
    }
    void cleanupTestCase()
    {
        killRunningDBusProcesses();
    }

    void testParallelQuerying()
    {
        manager->launchQuery("fooDelay300");

        QSignalSpy changedSpy(manager.get(), &RunnerManager::matchesChanged);
        QSignalSpy finishedSpy(manager.get(), &RunnerManager::queryFinished);
        QVERIFY(changedSpy.wait(255)); // Due to throttling, otherwise we'd have this signal after 50 ms
        QCOMPARE(finishedSpy.count(), 0);
        QCOMPARE(manager->matches().size(), 2);

        const auto matches = manager->matches();
        QVERIFY(std::all_of(matches.begin(), matches.end(), [this](QueryMatch m) {
            return m.runner() == fakeRunner;
        }));

        QVERIFY(finishedSpy.wait(105));
        QCOMPARE(changedSpy.count(), 2);

        QCOMPARE(manager->matches().size(), 3);
    }

    void benchmarkQuerying()
    {
        QSKIP("Skipped by default");
        QStandardPaths::setTestModeEnabled(false);
        manager.reset(new RunnerManager());
        QBENCHMARK_ONCE {
            launchQuery("test");
            launchQuery("spell bla");
            launchQuery("define test");
        }
    }
};

QTEST_MAIN(ThreadingTest)

#include "threadingtest.moc"
