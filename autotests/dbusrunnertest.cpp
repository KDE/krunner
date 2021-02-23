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

#include <QAction>
#include <QObject>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTime>
#include <QTimer>

#if WITH_KSERVICE
#include <KSycoca>
#endif

#include "abstractrunnertest.h"

using namespace Plasma;

Q_DECLARE_METATYPE(Plasma::QueryMatch)
Q_DECLARE_METATYPE(QList<Plasma::QueryMatch>)

class DBusRunnerTest : public AbstractRunnerTest
{
    Q_OBJECT
public:
    DBusRunnerTest();
    ~DBusRunnerTest();

private Q_SLOTS:
    void initTestCase();
    void cleanup();
    void testMatch();
    void testMulti();
    void testFilterProperties();
    void testFilterProperties_data();
    void testRequestActionsOnce();
    void testDBusRunnerSyntaxIntegration();
    void testIconData();
#if WITH_KSERVICE
    void testMulti_data();
#endif
};

DBusRunnerTest::DBusRunnerTest()
    : AbstractRunnerTest()
{
    qRegisterMetaType<QList<Plasma::QueryMatch>>();
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
}

void DBusRunnerTest::cleanup()
{
    // Make sure kill the running processes after each test
    killRunningDBusProcesses();
}

void DBusRunnerTest::testMatch()
{
    QProcess *process = startDBusRunnerProcess({QStringLiteral("net.krunnertests.dave")});
    initProperties();
    launchQuery(QStringLiteral("foo"));

    // verify matches
    const QList<QueryMatch> matches = manager->matches();
    QCOMPARE(matches.count(), 1);
    auto result = matches.first();

    // see testremoterunner.cpp
    QCOMPARE(result.id(), QStringLiteral("dbusrunnertest_id1")); // note the runner name is prepended
    QCOMPARE(result.text(), QStringLiteral("Match 1"));
    QCOMPARE(result.iconName(), QStringLiteral("icon1"));
    QCOMPARE(result.type(), Plasma::QueryMatch::ExactMatch);
    // relevance can't be compared easily because RunnerContext meddles with it

    // verify actions
    QTRY_COMPARE_WITH_TIMEOUT(manager->actionsForMatch(result).count(), 1, 10000);
    auto action = manager->actionsForMatch(result).constFirst();

    QCOMPARE(action->text(), QStringLiteral("Action 1"));

    QSignalSpy processSpy(process, &QProcess::readyRead);
    manager->run(result);
    processSpy.wait();
    QCOMPARE(process->readAllStandardOutput().trimmed().split('\n').constLast(), QByteArray("Running:id1:"));

    result.setSelectedAction(action);
    manager->run(result);
    processSpy.wait();
    QCOMPARE(process->readAllStandardOutput().trimmed().split('\n').constLast(), QByteArray("Running:id1:action1"));
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
    startDBusRunnerProcess({QStringLiteral("net.krunnertests.multi.a1")}, QStringLiteral("net.krunnertests.multi.a1"));
    startDBusRunnerProcess({QStringLiteral("net.krunnertests.multi.a2")}, QStringLiteral("net.krunnertests.multi.a2"));
    manager.reset(new RunnerManager()); // This case is special, because we want to load the runners manually

#if WITH_KSERVICE
    if (useKService) {
        KService::Ptr s(new KService(QFINDTESTDATA("dbusrunnertestmulti.desktop")));
        QVERIFY(s);
        manager->loadRunner(s);
    } else {
#endif
        auto md = KPluginMetaData::fromDesktopFile(QFINDTESTDATA("dbusrunnertestmulti.desktop"), {QStringLiteral("plasma-runner.desktop")});
        QVERIFY(md.isValid());
        manager->loadRunner(md);
#if WITH_KSERVICE
    }
#endif
    launchQuery(QStringLiteral("foo"));

    // verify matches, must be one from each
    const QList<QueryMatch> matches = manager->matches();
    QCOMPARE(matches.count(), 2);

    const QString first = matches.at(0).data().toList().constFirst().toString();
    const QString second = matches.at(1).data().toList().constFirst().toString();
    QVERIFY(first != second);
    QVERIFY(first == QLatin1String("net.krunnertests.multi.a1") || first == QStringLiteral("net.krunnertests.multi.a2"));
    QVERIFY(second == QLatin1String("net.krunnertests.multi.a1") || second == QStringLiteral("net.krunnertests.multi.a2"));
}

void DBusRunnerTest::testRequestActionsOnce()
{
    startDBusRunnerProcess({QStringLiteral("net.krunnertests.dave")});
    initProperties();

    // Construct a fake match with necessary data
    QueryMatch fakeMatch(runner);
    fakeMatch.setId(QStringLiteral("dbusrunnertest_id1"));
    fakeMatch.setData(QVariantList({QStringLiteral("net.krunnertests.dave"), QStringList({QStringLiteral("action1"), QStringLiteral("action2")})}));

    // We haven't called the prepare slot or launched a query, if the implementation works
    // the actions should already be available
    // We need to retry this, because the DBus call to fetch the actions is async
    QTRY_COMPARE_WITH_TIMEOUT(manager->actionsForMatch(fakeMatch).count(), 2, 2500);
}

void DBusRunnerTest::testFilterProperties_data()
{
    QTest::addColumn<QString>("rejectedQuery");
    QTest::addColumn<QString>("acceptedQuery");

    QTest::newRow("min-letter-count") << "fo"
                                      << "foo";
    QTest::newRow("match-regex") << "barfoo"
                                 << "foobar";
}

void DBusRunnerTest::testFilterProperties()
{
    QFETCH(QString, rejectedQuery);
    QFETCH(QString, acceptedQuery);
    QProcess *process = startDBusRunnerProcess({QStringLiteral("net.krunnertests.dave")});
    initProperties();

    launchQuery(rejectedQuery);
    // Match method was not called, because of the min letter count or match regex property
    QVERIFY(process->readAllStandardOutput().isEmpty());
    // accepted query fits those constraints
    launchQuery(acceptedQuery);
    QCOMPARE(process->readAllStandardOutput().trimmed(), QString(QStringLiteral("Matching:") + acceptedQuery).toLocal8Bit());
}

void DBusRunnerTest::testDBusRunnerSyntaxIntegration()
{
    startDBusRunnerProcess({QStringLiteral("net.krunnertests.dave")});
    initProperties();
    const QList<RunnerSyntax> syntaxes = runner->syntaxes();
    QCOMPARE(syntaxes.size(), 2);

    QCOMPARE(syntaxes.at(0).exampleQueries().size(), 1);
    QCOMPARE(syntaxes.at(0).exampleQueries().constFirst(), QStringLiteral("syntax1"));
    QCOMPARE(syntaxes.at(0).description(), QStringLiteral("description1"));
    QCOMPARE(syntaxes.at(1).exampleQueries().size(), 1);
    QCOMPARE(syntaxes.at(1).exampleQueries().constFirst(), QStringLiteral("syntax2"));
    QCOMPARE(syntaxes.at(1).description(), QStringLiteral("description2"));
}

void DBusRunnerTest::testIconData()
{
    startDBusRunnerProcess({QStringLiteral("net.krunnertests.dave")});
    initProperties();

    launchQuery(QStringLiteral("fooCostomIcon"));

    const auto matches = manager->matches();
    QCOMPARE(matches.count(), 1);
    auto result = matches.first();

    QImage expectedIcon(10, 10, QImage::Format_RGBA8888);
    expectedIcon.fill(Qt::blue);

    QCOMPARE(result.icon().availableSizes().first(), QSize(10, 10));
    QCOMPARE(result.icon().pixmap(QSize(10, 10)), QPixmap::fromImage(expectedIcon));
}

QTEST_MAIN(DBusRunnerTest)

#include "dbusrunnertest.moc"
