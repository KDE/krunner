/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractrunner.h"
#include "fakerunner.h"
#include "runnermanager.h"

#include <QAction>
#include <QObject>
#include <QTest>

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
    ~RunnerContextMatchMethodsTest();

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
    QCOMPARE(ctx->match(QStringLiteral("metadata_m1")), m1);
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

QTEST_MAIN(RunnerContextMatchMethodsTest)

#include "runnermatchmethodstest.moc"
