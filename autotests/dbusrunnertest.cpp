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
#include <QDebug>
#include <QProcess>

#include <KSycoca>

using namespace Plasma;

Q_DECLARE_METATYPE(Plasma::QueryMatch);
Q_DECLARE_METATYPE(QList<Plasma::QueryMatch>);

class DBusRunnerTest : public QObject
{
    Q_OBJECT
public:
    DBusRunnerTest();
    ~DBusRunnerTest();

private Q_SLOTS:
    void initTestCase();
    void testMatch();
private:
    QProcess *m_process;
};

DBusRunnerTest::DBusRunnerTest()
    : QObject(),
    m_process(new QProcess(this))
{
    m_process->start(QFINDTESTDATA("testremoterunner"));
    QVERIFY(m_process->waitForStarted());
    qRegisterMetaType<QList<Plasma::QueryMatch> >();
}

DBusRunnerTest::~DBusRunnerTest()
{
    m_process->kill();
    m_process->waitForFinished();
}

void DBusRunnerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).mkpath(QStringLiteral("kservices5"));
    const QString fakeServicePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/kservices5/dbusrunnertest.desktop");
    QFile::copy(QFINDTESTDATA("dbusrunnertest.desktop"), fakeServicePath);
    KSycoca::self()->ensureCacheValid();
}

void DBusRunnerTest::testMatch()
{
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
    //relevance can't be compared easily becuase RunnerContext meddles with it

    //verify actions
    auto actions = m.actionsForMatch(result);
    QCOMPARE(actions.count(), 1);
    auto action = actions.first();

    QCOMPARE(action->text(), QStringLiteral("Action 1"));

    QSignalSpy processSpy(m_process, &QProcess::readyRead);
    m.run(result);
    processSpy.wait();
    QCOMPARE(m_process->readAllStandardOutput().trimmed(), QByteArray("Running:id1:"));

    result.setSelectedAction(action);
    m.run(result);
    processSpy.wait();
    QCOMPARE(m_process->readAllStandardOutput().trimmed(), QByteArray("Running:id1:action1"));
}



QTEST_MAIN(DBusRunnerTest)


#include "dbusrunnertest.moc"
