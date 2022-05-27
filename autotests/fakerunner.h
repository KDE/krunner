/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LicenseRef-KDE-Accepted-GPL
*/

#include "abstractrunner.h"

using namespace Plasma;

class FakeRunner : public AbstractRunner
{
public:
    FakeRunner(QObject *parent, const KPluginMetaData &metadata, const QVariantList &args)
        : AbstractRunner(parent, metadata, args)
    {
    }
    FakeRunner(const KPluginMetaData &metadata = KPluginMetaData(QStringLiteral("metadata.desktop")))
        : FakeRunner(nullptr, metadata, {})
    {
    }

    void match(RunnerContext &context) override
    {
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
