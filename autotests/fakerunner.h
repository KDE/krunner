/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractrunner.h"

#include <QEventLoop>
#include <QTimer>

using namespace KRunner;

class FakeRunner : public AbstractRunner
{
public:
    explicit FakeRunner(QObject *parent, const KPluginMetaData &metadata)
        : AbstractRunner(parent, metadata)
    {
        setObjectName("FakeRunner");
    }
    explicit FakeRunner(const KPluginMetaData &metadata = KPluginMetaData(QStringLiteral("metadata.desktop")))
        : FakeRunner(nullptr, metadata)
    {
        setObjectName("FakeRunner");
    }

    void match(RunnerContext &context) override
    {
        QEventLoop l;
        QTimer::singleShot(50, [&l]() {
            l.quit();
        });
        l.exec();
        if (context.query().startsWith(QLatin1String("foo"))) {
            context.addMatch(createDummyMatch(QStringLiteral("foo"), 0.1));
            context.addMatch(createDummyMatch(QStringLiteral("bar"), 0.2));
        }
    }

private:
    QueryMatch createDummyMatch(const QString &text, qreal relevance)
    {
        QueryMatch queryMatch(this);
        queryMatch.setId(text);
        queryMatch.setText(text);
        queryMatch.setRelevance(relevance);
        return queryMatch;
    }
};
