/*
    SPDX-FileCopyrightText: 2020-2023 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "abstractrunner.h"
#include "runnersyntax.h"
#include <QRegularExpression>

namespace KRunner
{
class AbstractRunnerPrivate
{
public:
    explicit AbstractRunnerPrivate(AbstractRunner *r, const KPluginMetaData &pluginMetaData)
        : runnerDescription(pluginMetaData)
        , runner(r)
    {
        minLetterCount = runnerDescription.value(QStringLiteral("X-Plasma-Runner-Min-Letter-Count"), 0);
        if (runnerDescription.isValid()) {
            const auto rawData = runnerDescription.rawData();
            if (rawData.contains(QStringLiteral("X-Plasma-Runner-Match-Regex"))) {
                matchRegex = QRegularExpression(rawData.value(QStringLiteral("X-Plasma-Runner-Match-Regex")).toString());
                hasMatchRegex = matchRegex.isValid() && !matchRegex.pattern().isEmpty();
            }
            hasUniqueResults = runnerDescription.value(QStringLiteral("X-Plasma-Runner-Unique-Results"), false);
            hasWeakResults = runnerDescription.value(QStringLiteral("X-Plasma-Runner-Weak-Results"), false);
        }
    }

    QObject *initialParent;
    const KPluginMetaData runnerDescription;
    AbstractRunner *runner;
    QList<RunnerSyntax> syntaxes;
    bool suspendMatching = false;
    int minLetterCount = 0;
    QRegularExpression matchRegex;
    bool hasMatchRegex = false;
    bool hasUniqueResults = false;
    bool hasWeakResults = false;
};
}
