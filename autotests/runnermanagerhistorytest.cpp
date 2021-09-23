/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "fakerunner.h"
#include "runnermanager.h"

#include <QAction>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTest>

using namespace Plasma;

class RunnerManagerHistoryTest : public QObject
{
    Q_OBJECT

private:
    void addToHistory(const QStringList &queries, RunnerManager &manager)
    {
        FakeRunner runner;
        for (const QString &query : queries) {
            QueryMatch match(&runner);
            // Make sure internally the term and untrimmedTerm are set
            manager.launchQuery(query, "thisrunnerdoesnotexist");
            manager.searchContext()->setQuery(query);
            manager.runMatch(match);
        }
    }

private Q_SLOTS:
    void init()
    {
        QStandardPaths::setTestModeEnabled(true);
        QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + "krunnerstaterc";
        if (QFileInfo::exists(path)) {
            QFile::remove(path);
        }
    }
    void testRunnerHistory();
    void testRunnerHistory_data();
    void testHistorySuggestionsAndRemoving();
};

void RunnerManagerHistoryTest::testRunnerHistory()
{
    QFETCH(const QStringList, queries);
    QFETCH(const QStringList, expectedEntries);

    RunnerManager manager;
    addToHistory(queries, manager);
    QCOMPARE(manager.history(), expectedEntries);
}

void RunnerManagerHistoryTest::testRunnerHistory_data()
{
    QTest::addColumn<QStringList>("queries");
    QTest::addColumn<QStringList>("expectedEntries");

    QTest::newRow("should add simple entry to history") << QStringList{"test"} << QStringList{"test"};
    QTest::newRow("should not add entry that starts with space") << QStringList{" test"} << QStringList{};
    QTest::newRow("should not add duplicate entries") << QStringList{"test", "test"} << QStringList{"test"};
    QTest::newRow("should not add duplicate entries but put last run at beginning") << QStringList{"test", "test2", "test"} << QStringList{"test", "test2"};
}

void RunnerManagerHistoryTest::testHistorySuggestionsAndRemoving()
{
    RunnerManager manager;
    const QStringList queries = {"test1", "test2", "test3"};
    addToHistory(queries, manager);
    QStringList expectedBeforeRemoval = QStringList{"test3", "test2", "test1"};
    QCOMPARE(manager.history(), expectedBeforeRemoval);
    QCOMPARE(manager.getHistorySuggestion("t"), "test3");
    QCOMPARE(manager.getHistorySuggestion("doesnotexist"), QString());

    manager.removeFromHistory(42);
    QCOMPARE(manager.history(), expectedBeforeRemoval);
    manager.removeFromHistory(0);
    QStringList expectedAfterRemoval = QStringList{"test2", "test1"};
    QCOMPARE(manager.history(), expectedAfterRemoval);
    QCOMPARE(manager.getHistorySuggestion("t"), "test2");
}

QTEST_MAIN(RunnerManagerHistoryTest)

#include "runnermanagerhistorytest.moc"
