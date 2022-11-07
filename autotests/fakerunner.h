/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LicenseRef-KDE-Accepted-GPL
*/

#include "abstractrunner.h"

#include <QEventLoop>
#include <QTimer>

using namespace Plasma;

class FakeRunner : public AbstractRunner
{
public:
    FakeRunner(QObject *parent, const KPluginMetaData &metadata, const QVariantList &args)
        : AbstractRunner(parent, metadata, args)
    {
        setObjectName("FakeRunner");
    }
    FakeRunner(const KPluginMetaData &metadata = KPluginMetaData(QStringLiteral("metadata.desktop")))
        : FakeRunner(nullptr, metadata, {})
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
        if (context.query() == QLatin1String("foo")) {
            context.addMatch(createDummyMatch(QStringLiteral("foo"), 0.1));
            context.addMatch(createDummyMatch(QStringLiteral("bar"), 0.2));
        }
    }

private:
    QueryMatch createDummyMatch(const QString &text, qreal relevance)
    {
        QueryMatch match(this);
        match.setId(text);
        match.setText(text);
        match.setRelevance(relevance);
        return match;
    }
};
