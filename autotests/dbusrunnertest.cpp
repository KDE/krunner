/*
 *   Copyright (C) 2017 David Edmundson <davidedmundson@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QObject>
#include <QAction>
#include <QtTest>

#include "runnermanager.h"
#include <QSignalSpy>
#include <QProcess>

#include <KSycoca>

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
    void cleanupTestCase();
    void testMatch();
    void testMulti();
private:
    QStringList m_filesForCleanup;
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
    // Set up a layer in the bin dir so ksycoca finds the Plasma/Runner service type
    const QByteArray defaultDataDirs = qEnvironmentVariableIsSet("XDG_DATA_DIRS") ? qgetenv("XDG_DATA_DIRS") : QByteArray("/usr/local:/usr");
    const QByteArray modifiedDataDirs = QFile::encodeName(QCoreApplication::applicationDirPath()) + QByteArrayLiteral("/data:") + defaultDataDirs;
    qputenv("XDG_DATA_DIRS", modifiedDataDirs);

    QStandardPaths::setTestModeEnabled(true);
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
}

void DBusRunnerTest::cleanupTestCase()
{
    for(const QString path: m_filesForCleanup) {
        QFile::remove(path);
    }
}

void DBusRunnerTest::testMatch()
{
    QProcess process;
    process.start(QFINDTESTDATA("testremoterunner"), QStringList({QStringLiteral("net.krunnertests.dave")}));
    QVERIFY(process.waitForStarted());

    QTest::qSleep(500);

    RunnerManager m;
    auto s = KService::serviceByDesktopPath(QStringLiteral("dbusrunnertest.desktop"));
    QVERIFY(s);
    m.loadRunner(s);
    m.launchQuery(QStringLiteral("foo"));

    QSignalSpy spy(&m, &RunnerManager::matchesChanged);
    QVERIFY(spy.wait());

    //verify matches
    QCOMPARE(m.matches().count(), 1);
    auto result = m.matches().first();

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

void DBusRunnerTest::testMulti()
{
    QProcess process1;
    process1.start(QFINDTESTDATA("testremoterunner"), QStringList({QStringLiteral("net.krunnertests.multi.a1")}));
    QVERIFY(process1.waitForStarted());

    QProcess process2;
    process2.start(QFINDTESTDATA("testremoterunner"), QStringList({QStringLiteral("net.krunnertests.multi.a2")}));
    QVERIFY(process2.waitForStarted());

    QTest::qSleep(500);

    RunnerManager m;
    auto s = KService::serviceByDesktopPath(QStringLiteral("dbusrunnertestmulti.desktop"));
    QVERIFY(s);
    m.loadRunner(s);
    m.launchQuery(QStringLiteral("foo"));

    QSignalSpy spy(&m, &RunnerManager::matchesChanged);
    QVERIFY(spy.wait());

    //verify matches, must be one from each
    QCOMPARE(m.matches().count(), 2);

    QString first = m.matches().at(0).data().toString();
    QString second = m.matches().at(1).data().toString();
    QVERIFY(first != second);
    QVERIFY(first == QLatin1String("net.krunnertests.multi.a1") || first == QStringLiteral("net.krunnertests.multi.a2"));
    QVERIFY(second == QLatin1String("net.krunnertests.multi.a1") || second == QStringLiteral("net.krunnertests.multi.a2"));

    process1.kill();
    process2.kill();
    process1.waitForFinished();
    process2.waitForFinished();
}



QTEST_MAIN(DBusRunnerTest)


#include "dbusrunnertest.moc"
