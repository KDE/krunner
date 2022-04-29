/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractrunner.h"
#include "fakerunner.h"
#include "runnermanager.h"

#include <QAction>
#include <QObject>
#include <QStandardPaths>
#include <QTest>

#include "kpluginmetadata_utils_p.h"

using namespace Plasma;

namespace
{
inline QueryMatch createMatch(const QString &id, AbstractRunner *r = nullptr)
{
    QueryMatch m(r);
    m.setId(id);
    return m;
}
}

class RunnerContextMatchMethodsTest : public QObject
{
    Q_OBJECT
public:
    RunnerContextMatchMethodsTest();
    ~RunnerContextMatchMethodsTest() override;

    std::unique_ptr<RunnerContext> ctx;
    FakeRunner *runner1;
    FakeRunner *runner2;
private Q_SLOTS:
    void init();
    void testAdd();
    void testAddMulti();
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
    void testRemoveMatch();
    void testRemoveMatchMulti();
    void testRemoveMatchByRunner();
#endif
    void testDuplicateIds();
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 79)
    void testGetMatchById();
    void testNonExistentMatchIds();
#endif
};

RunnerContextMatchMethodsTest::RunnerContextMatchMethodsTest()
    : QObject()
    , runner1(new FakeRunner())
    , runner2(new FakeRunner())
{
    QStandardPaths::setTestModeEnabled(true);
    const QByteArray defaultDataDirs = qEnvironmentVariableIsSet("XDG_DATA_DIRS") ? qgetenv("XDG_DATA_DIRS") : QByteArray("/usr/local:/usr");
    const QByteArray modifiedDataDirs = QFile::encodeName(QCoreApplication::applicationDirPath()) + QByteArrayLiteral("/data:") + defaultDataDirs;
    qputenv("XDG_DATA_DIRS", modifiedDataDirs);
    KPluginMetaData data1 = parseMetaDataFromDesktopFile(QFINDTESTDATA("metadatafile1.desktop"));
    KPluginMetaData data2 = parseMetaDataFromDesktopFile(QFINDTESTDATA("metadatafile2.desktop"));
    QVERIFY(data1.isValid());
    QVERIFY(data2.isValid());
    runner1 = new FakeRunner(data1);
    runner2 = new FakeRunner(data2);
}

RunnerContextMatchMethodsTest::~RunnerContextMatchMethodsTest()
{
    delete runner1;
    delete runner2;
}

void RunnerContextMatchMethodsTest::init()
{
    ctx.reset(new RunnerContext());
}

void RunnerContextMatchMethodsTest::testAdd()
{
    QVERIFY(ctx->matches().isEmpty());
    QVERIFY(ctx->addMatch(createMatch(QStringLiteral("m1"))));
    QVERIFY(ctx->addMatch(createMatch(QStringLiteral("m2"))));
    QCOMPARE(ctx->matches().count(), 2);
    QVERIFY(ctx->addMatch(createMatch(QStringLiteral("m3"))));
    QCOMPARE(ctx->matches().count(), 3);
}

void RunnerContextMatchMethodsTest::testAddMulti()
{
    QVERIFY(ctx->matches().isEmpty());
    QVERIFY(ctx->addMatches({createMatch(QStringLiteral("m1")), createMatch(QStringLiteral("m2"))}));
    QCOMPARE(ctx->matches().count(), 2);
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
void RunnerContextMatchMethodsTest::testRemoveMatch()
{
    QueryMatch m = createMatch(QStringLiteral("m1"));
    ctx->addMatches({m, createMatch(QStringLiteral("m2"))});
    QCOMPARE(ctx->matches().count(), 2);
    QVERIFY(ctx->removeMatch(m.id()));
    QCOMPARE(ctx->matches().count(), 1);
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
void RunnerContextMatchMethodsTest::testRemoveMatchByRunner()
{
    QVERIFY(ctx->matches().isEmpty());
    QueryMatch m1 = createMatch(QStringLiteral("m1"), runner1);
    QueryMatch m2 = createMatch(QStringLiteral("m2"), runner1);
    QueryMatch m3 = createMatch(QStringLiteral("m3"), runner2);
    ctx->addMatches({m1, m2, m3});
    QCOMPARE(ctx->matches().count(), 3);
    QVERIFY(ctx->removeMatches(runner1));
    QCOMPARE(ctx->matches().count(), 1);
    QCOMPARE(ctx->matches().constFirst(), m3);
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
void RunnerContextMatchMethodsTest::testRemoveMatchMulti()
{
    QVERIFY(ctx->matches().isEmpty());
    QueryMatch m1 = createMatch(QStringLiteral("m1"));
    QueryMatch m2 = createMatch(QStringLiteral("m2"));
    QueryMatch m3 = createMatch(QStringLiteral("m3"));
    ctx->addMatches({m1, m2, m3});
    QCOMPARE(ctx->matches().count(), 3);

    // Nothing should happen in case of an empty list
    QVERIFY(!ctx->removeMatches(QStringList()));
    QCOMPARE(ctx->matches().count(), 3);

    QVERIFY(ctx->removeMatches({m1.id(), m2.id()}));
    QCOMPARE(ctx->matches().count(), 1);
    QCOMPARE(ctx->matches().constFirst(), m3);
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 79)
void RunnerContextMatchMethodsTest::testGetMatchById()
{
    QueryMatch m1 = createMatch(QStringLiteral("m1"), runner1);
    QueryMatch m2 = createMatch(QStringLiteral("m2"), runner1);
    QueryMatch m3 = createMatch(QStringLiteral("m3"), runner2);
    ctx->addMatches({m1, m2, m3});
    QCOMPARE(ctx->matches().count(), 3);
    QCOMPARE(ctx->match(m1.id()), m1);
    // ID gets internally concatenated with runner id
    QCOMPARE(ctx->match(QStringLiteral("m1")), m1);
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 79)
void RunnerContextMatchMethodsTest::testNonExistentMatchIds()
{
    QueryMatch m1 = createMatch(QStringLiteral("m1"));
    QueryMatch m2 = createMatch(QStringLiteral("m2"));
    ctx->addMatches({m1, m2});
    QCOMPARE(ctx->matches().count(), 2);
    QVERIFY(!ctx->removeMatch(QStringLiteral("does_not_exist")));
    QCOMPARE(ctx->matches().count(), 2);

    QVERIFY(!ctx->match(QStringLiteral("does_not_exist")).isValid());
    QCOMPARE(ctx->match(QStringLiteral("does_not_exist")).runner(), nullptr);
}
#endif

void RunnerContextMatchMethodsTest::testDuplicateIds()
{
    const QueryMatch match1 = createMatch(QStringLiteral("id1"), runner1);
    QVERIFY(ctx->addMatch(match1));
    const QueryMatch match2 = createMatch(QStringLiteral("id1"), runner2);
    QVERIFY(ctx->addMatch(match2));
    const QueryMatch match3 = createMatch(QStringLiteral("id2"), runner1);
    QVERIFY(ctx->addMatch(match3));
    const QueryMatch match4 = createMatch(QStringLiteral("id3"), runner2);
    QVERIFY(ctx->addMatch(match4));
    const QueryMatch match5 = createMatch(QStringLiteral("id3"), runner2);
    QVERIFY(ctx->addMatch(match5));

    const QList<QueryMatch> matches = ctx->matches();
    QCOMPARE(matches.size(), 3);
    // match2 should have replaced match1
    QCOMPARE(matches.at(0), match2);
    QCOMPARE(matches.at(1), match3);
    // match4 should not have been replaced, the runner does not have the weak property set
    QCOMPARE(matches.at(2), match4);
}


QTEST_MAIN(RunnerContextMatchMethodsTest)

#include "runnermatchmethodstest.moc"
