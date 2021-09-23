/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "fakerunner.h"
#include "runnermanager.h"

#include <KSharedConfig>
#include <QAction>
#include <QCoreApplication>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTest>

#include "abstractrunnertest.h"

using namespace Plasma;

class RunnerManagerSingleRunnerModeTest : public AbstractRunnerTest
{
    Q_OBJECT
private Q_SLOTS:
    void loadTwoRunners()
    {
        startDBusRunnerProcess({QStringLiteral("net.krunnertests.dave")});
        startDBusRunnerProcess({QStringLiteral("net.krunnertests.multi.a1")}, QStringLiteral("net.krunnertests.multi.a1"));
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
        const QString configFile = QStandardPaths::locate(QStandardPaths::ConfigLocation, "runnermanagersinglerunnermodetestrcm");
        if (!configFile.isEmpty()) {
            QFile::remove(configFile);
        }
    }

    void testAllRunnerResults();
    void testSingleRunnerResults();
    void testNonExistentRunnerId();
    void testLoadingDisabledRunner();
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
    void testAdvertisedSingleRunners();
#endif
};

void RunnerManagerSingleRunnerModeTest::testAllRunnerResults()
{
    loadTwoRunners();
    launchQuery("foo");
    const auto matches = manager->matches();
    QCOMPARE(matches.count(), 2);
    const QStringList ids = {matches.at(0).runner()->id(), matches.at(1).runner()->id()};
    // Both runners should have produced one result
    QVERIFY(ids.contains("dbusrunnertest"));
    QVERIFY(ids.contains("dbusrunnertestmulti"));
}

void RunnerManagerSingleRunnerModeTest::testSingleRunnerResults()
{
    loadTwoRunners();
    launchQuery("foo", "dbusrunnertest");
    QCOMPARE(manager->matches().count(), 1);
    QCOMPARE(manager->matches().constFirst().runner()->id(), "dbusrunnertest");
    launchQuery("foo", "dbusrunnertestmulti");
    QCOMPARE(manager->matches().count(), 1);
    QCOMPARE(manager->matches().constFirst().runner()->id(), "dbusrunnertestmulti");
}

void RunnerManagerSingleRunnerModeTest::testNonExistentRunnerId()
{
    loadTwoRunners();
    manager->launchQuery("foo", "bla"); // This internally calls reset, we just wait a bit and make sure there are no matches
    QTest::qSleep(250);
    QVERIFY(manager->matches().isEmpty());
}

void RunnerManagerSingleRunnerModeTest::testLoadingDisabledRunner()
{
    loadTwoRunners();
    // Make sure the runner is disabled in the config
    auto config = KSharedConfig::openConfig();
    config->group("Plugins").writeEntry("dbusrunnertestEnabled", false);
    // reset our manager to start clean
    manager.reset(new RunnerManager());
    // Copy the service files to the appropriate location and only load runners from there
    QString location = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/krunner/dbusplugins/";
    QDir().mkpath(location);
    QFile::copy(QFINDTESTDATA("dbusrunnertest.desktop"), location + "dbusrunnertest.desktop");
    QFile::copy(QFINDTESTDATA("dbusrunnertestmulti.desktop"), location + "dbusrunnertestmulti.desktop");

    // Only enabled runner should be loaded and have results
    launchQuery("foo");
    QCOMPARE(manager->runners().count(), 1);
    QCOMPARE(manager->runners().constFirst()->id(), "dbusrunnertestmulti");
    QCOMPARE(manager->matches().count(), 1);
    QCOMPARE(manager->matches().constFirst().runner()->id(), "dbusrunnertestmulti");

    // Only enabled runner should be loaded and have results
    launchQuery("foo", "dbusrunnertest");
    QCOMPARE(manager->runners().count(), 2);
    const QStringList ids{manager->runners().at(0)->id(), manager->runners().at(1)->id()};
    QVERIFY(ids.contains("dbusrunnertestmulti"));
    QVERIFY(ids.contains("dbusrunnertest"));
    QCOMPARE(manager->matches().count(), 1);
    QCOMPARE(manager->matches().constFirst().runner()->id(), "dbusrunnertest");

    // Only enabled runner should used for querying
    launchQuery("foo");
    QCOMPARE(manager->runners().count(), 2);
    QCOMPARE(manager->matches().count(), 1);
    QCOMPARE(manager->matches().constFirst().runner()->id(), "dbusrunnertestmulti");
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 82)
void RunnerManagerSingleRunnerModeTest::testAdvertisedSingleRunners()
{
    loadTwoRunners();
    QCOMPARE(manager->singleModeAdvertisedRunnerIds(), QStringList{"dbusrunnertest"});
}
#endif

QTEST_MAIN(RunnerManagerSingleRunnerModeTest)

#include "runnermanagersinglerunnermodetest.moc"
