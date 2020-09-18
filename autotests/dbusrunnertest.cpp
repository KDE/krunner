/*
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnermanager.h"

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72) && KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
#define WITH_KSERVICE 1
#else
#define WITH_KSERVICE 0
#endif

#include <QObject>
#include <QAction>
#include <QTest>
#include <QStandardPaths>
#include <QSignalSpy>
#include <QProcess>
#include <QTime>
#include <QTimer>

#if WITH_KSERVICE
#include <KSycoca>
#endif

using namespace Plasma;

Q_DECLARE_METATYPE(Plasma::QueryMatch)
Q_DECLARE_METATYPE(QList<Plasma::QueryMatch>)

class DBusRunnerTest : public QObject
{
    Q_OBJECT
public:
    DBusRunnerTest();
    ~DBusRunnerTest();

private Q_SLOTS:
    void initTestCase();
#if WITH_KSERVICE
    void cleanupTestCase();
#endif
    void testMatch();
    void testMulti();
    void testRequestActionsOnce();
#if WITH_KSERVICE
    void testMatch_data();
    void testMulti_data();
    void testRequestActionsOnce_data();
private:
    QStringList m_filesForCleanup;
#endif
};

DBusRunnerTest::DBusRunnerTest()
    : QObject()
{
    qRegisterMetaType<QList<Plasma::QueryMatch> >();
}

DBusRunnerTest::~DBusRunnerTest()
{
}

void DBusRunnerTest::initTestCase()
{
    // Set up a layer in the bin dir so ksycoca & KPluginMetaData find the Plasma/Runner service type
    const QByteArray defaultDataDirs = qEnvironmentVariableIsSet("XDG_DATA_DIRS") ? qgetenv("XDG_DATA_DIRS") : QByteArray("/usr/local:/usr");
    const QByteArray modifiedDataDirs = QFile::encodeName(QCoreApplication::applicationDirPath()) + QByteArrayLiteral("/data:") + defaultDataDirs;
    qputenv("XDG_DATA_DIRS", modifiedDataDirs);

    QStandardPaths::setTestModeEnabled(true);
#if WITH_KSERVICE
    QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).mkpath(QStringLiteral("kservices5"));
    {
        const QString fakeServicePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/kservices5/dbusrunnertest.desktop");
        QFile::copy(QFINDTESTDATA("dbusrunnertest.desktop"), fakeServicePath);
        m_filesForCleanup << fakeServicePath;

    }
    {
        const QString fakeServicePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/kservices5/dbusrunnertestmulti.desktop");
        QFile::copy(QFINDTESTDATA("dbusrunnertestmulti.desktop"), fakeServicePath);
        m_filesForCleanup << fakeServicePath;
    }
    KSycoca::self()->ensureCacheValid();
#endif
}

#if WITH_KSERVICE
void DBusRunnerTest::cleanupTestCase()
{
    for(const QString &path: qAsConst(m_filesForCleanup)) {
        QFile::remove(path);
    }
}
#endif

#if WITH_KSERVICE
void DBusRunnerTest::testMatch_data()
{
    QTest::addColumn<bool>("useKService");

    QTest::newRow("deprecated") << true;
    QTest::newRow("non-deprecated") << false;
}
#endif

void DBusRunnerTest::testMatch()
{
#if WITH_KSERVICE
    QFETCH(bool, useKService);
#endif

    QProcess process;
    process.start(QFINDTESTDATA("testremoterunner"), QStringList({QStringLiteral("net.krunnertests.dave")}));
    QVERIFY(process.waitForStarted());

    QTest::qSleep(500);

    RunnerManager m;
#if WITH_KSERVICE
    if (useKService) {
        auto s = KService::serviceByDesktopPath(QStringLiteral("dbusrunnertest.desktop"));
        QVERIFY(s);
        m.loadRunner(s);
    } else {
#endif
        auto md = KPluginMetaData::fromDesktopFile(QFINDTESTDATA("dbusrunnertest.desktop"), {QStringLiteral("plasma-runner.desktop")});
        QVERIFY(md.isValid());
        m.loadRunner(md);
#if WITH_KSERVICE
    }
#endif
    m.launchQuery(QStringLiteral("foo"));

    QSignalSpy spy(&m, &RunnerManager::matchesChanged);
    QVERIFY(spy.wait());

    //verify matches
    const auto matches = m.matches();
    QCOMPARE(matches.count(), 1);
    auto result = matches.first();

    //see testremoterunner.cpp
    QCOMPARE(result.id(), QStringLiteral("dbusrunnertest_id1")); //note the runner name is prepended
    QCOMPARE(result.text(), QStringLiteral("Match 1"));
    QCOMPARE(result.iconName(), QStringLiteral("icon1"));
    QCOMPARE(result.type(), Plasma::QueryMatch::ExactMatch);
    //relevance can't be compared easily because RunnerContext meddles with it

    //verify actions
    auto actions = m.actionsForMatch(result);
    QCOMPARE(actions.count(), 1);
    auto action = actions.first();

    QCOMPARE(action->text(), QStringLiteral("Action 1"));

    QSignalSpy processSpy(&process, &QProcess::readyRead);
    m.run(result);
    processSpy.wait();
    QCOMPARE(process.readAllStandardOutput().trimmed(), QByteArray("Running:id1:"));

    result.setSelectedAction(action);
    m.run(result);
    processSpy.wait();
    QCOMPARE(process.readAllStandardOutput().trimmed(), QByteArray("Running:id1:action1"));

    process.kill();
    process.waitForFinished();
}

#if WITH_KSERVICE
void DBusRunnerTest::testMulti_data()
{
    QTest::addColumn<bool>("useKService");

    QTest::newRow("deprecated") << true;
    QTest::newRow("non-deprecated") << false;
}
#endif

void DBusRunnerTest::testMulti()
{
#if WITH_KSERVICE
    QFETCH(bool, useKService);
#endif

    QProcess process1;
    process1.start(QFINDTESTDATA("testremoterunner"), QStringList({QStringLiteral("net.krunnertests.multi.a1")}));
    QVERIFY(process1.waitForStarted());

    QProcess process2;
    process2.start(QFINDTESTDATA("testremoterunner"), QStringList({QStringLiteral("net.krunnertests.multi.a2")}));
    QVERIFY(process2.waitForStarted());

    QTest::qSleep(500);

    RunnerManager m;
#if WITH_KSERVICE
    if (useKService) {
        auto s = KService::serviceByDesktopPath(QStringLiteral("dbusrunnertestmulti.desktop"));
        QVERIFY(s);
        m.loadRunner(s);
    } else {
#endif
        auto md = KPluginMetaData::fromDesktopFile(QFINDTESTDATA("dbusrunnertestmulti.desktop"), {QStringLiteral("plasma-runner.desktop")});
        QVERIFY(md.isValid());
        m.loadRunner(md);
#if WITH_KSERVICE
    }
#endif
    m.launchQuery(QStringLiteral("foo"));

    QSignalSpy spy(&m, &RunnerManager::matchesChanged);
    QVERIFY(spy.wait());

    //verify matches, must be one from each
    QCOMPARE(m.matches().count(), 2);

    const QString first = m.matches().at(0).data().toList().constFirst().toString();
    const QString second = m.matches().at(1).data().toList().constFirst().toString();
    QVERIFY(first != second);
    QVERIFY(first == QLatin1String("net.krunnertests.multi.a1") || first == QStringLiteral("net.krunnertests.multi.a2"));
    QVERIFY(second == QLatin1String("net.krunnertests.multi.a1") || second == QStringLiteral("net.krunnertests.multi.a2"));

    process1.kill();
    process2.kill();
    process1.waitForFinished();
    process2.waitForFinished();
}

#if WITH_KSERVICE
void DBusRunnerTest::testRequestActionsOnce_data()
{
    QTest::addColumn<bool>("useKService");

    QTest::newRow("deprecated") << true;
    QTest::newRow("non-deprecated") << false;
}
#endif

void DBusRunnerTest::testRequestActionsOnce()
{
#if WITH_KSERVICE
    QFETCH(bool, useKService);
#endif

    QProcess process;
    process.start(QFINDTESTDATA("testremoterunner"), QStringList({QStringLiteral("net.krunnertests.dave")}));
    QVERIFY(process.waitForStarted());
    QTest::qSleep(500);

    RunnerManager m;
#if WITH_KSERVICE
    if (useKService) {
        auto s = KService::serviceByDesktopPath(QStringLiteral("dbusrunnertest.desktop"));
        QVERIFY(s);
        m.loadRunner(s);
    } else {
#endif
        auto md = KPluginMetaData::fromDesktopFile(QFINDTESTDATA("dbusrunnertest.desktop"), {QStringLiteral("plasma-runner.desktop")});
        QVERIFY(md.isValid());
        m.loadRunner(md);
#if WITH_KSERVICE
    }
#endif

    // Wait because dbus signal is async
    QEventLoop loop;
    QTimer t;
    QTimer::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(500);
    loop.exec();

    // Construct a fake match with necesarry data
    QueryMatch fakeMatch(m.runner(QStringLiteral("dbusrunnertest")));
    fakeMatch.setId(QStringLiteral("dbusrunnertest_id1"));
    fakeMatch.setData(QVariantList({
                                       QStringLiteral("net.krunnertests.dave"),
                                       QStringList({QStringLiteral("action1"), QStringLiteral("action2")})
                                   }));

    // We haven't called the prepare slot, if the implementation works
    // the actions should alredy be available
    auto actions = m.actionsForMatch(fakeMatch);
    QCOMPARE(actions.count(), 2);

    process.kill();
    process.waitForFinished();
}

QTEST_MAIN(DBusRunnerTest)


#include "dbusrunnertest.moc"
