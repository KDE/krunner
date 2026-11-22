/*
    SPDX-FileCopyrightText: 2026 Farhad Hedayatifard <hf.farhad@gmail.com>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "model/resultsmodelconstants_p.h"
#include "runnermanager.h"
#include <KRunner/ResultsModel>

#include <KPluginMetaData>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

using namespace KRunner;

class ResultsModelTest : public QObject
{
    Q_OBJECT

private:
    RunnerManager *m_manager = nullptr;
    ResultsModel *m_model = nullptr;

    QModelIndex indexForId(const QString &id) const
    {
        for (int i = 0; i < m_model->rowCount(); ++i) {
            const QModelIndex idx = m_model->index(i, 0);
            if (idx.data(ResultsModel::IdRole).toString().endsWith(id)) {
                return idx;
            }
        }
        return {};
    }

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        m_manager = new RunnerManager(this);
        m_manager->loadRunner(KPluginMetaData::findPluginById(QStringLiteral("krunnertest"), QStringLiteral("fakerunnerplugin")));
        QCOMPARE(m_manager->runners().count(), 1);

        m_model = new ResultsModel(this);
        m_model->setRunnerManager(m_manager);

        QSignalSpy spy(m_manager, &RunnerManager::queryFinished);
        m_model->setQueryString(QStringLiteral("hugetext"));
        QVERIFY(spy.wait());
        QCOMPARE(m_model->rowCount(), 2);
    }

    void testHugeSubtextIsShortened()
    {
        const QString subtext = indexForId(QStringLiteral("singleline")).data(ResultsModel::SubtextRole).toString();
        QCOMPARE(subtext.length(), maxDisplayTextLength);
    }

    void testHugeSingleLineTextIsShortened()
    {
        const QString text = indexForId(QStringLiteral("singleline")).data(Qt::DisplayRole).toString();
        QCOMPARE(text.length(), maxDisplayTextLength);
    }

    void testShortenedTextEndsWithEllipsis()
    {
        const QString text = indexForId(QStringLiteral("singleline")).data(Qt::DisplayRole).toString();
        QVERIFY(text.endsWith(QChar(u'…')));
    }

    void testMultiLineTextIsKept()
    {
        const QString text = indexForId(QStringLiteral("multiline")).data(Qt::DisplayRole).toString();
        QCOMPARE(text.length(), 2'000'000);
    }
};

QTEST_MAIN(ResultsModelTest)

#include "resultsmodeltest.moc"
