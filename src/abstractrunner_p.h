/*
    SPDX-FileCopyrightText: 2020-2023 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "abstractrunner.h"
#include "runnersyntax.h"
#include <QReadWriteLock>
#include <QRegularExpression>
#include <optional>

namespace KRunner
{
class AbstractRunnerPrivate
{
public:
    explicit AbstractRunnerPrivate(AbstractRunner *r, const KPluginMetaData &data)
        : runnerDescription(data)
        , runner(r)
        , minLetterCount(data.value(QStringLiteral("X-Plasma-Runner-Min-Letter-Count"), 0))
        , hasUniqueResults(data.value(QStringLiteral("X-Plasma-Runner-Unique-Results"), false))
        , hasWeakResults(data.value(QStringLiteral("X-Plasma-Runner-Weak-Results"), false))
    {
        if (const QString regexStr = data.value(QStringLiteral("X-Plasma-Runner-Match-Regex")); !regexStr.isEmpty()) {
            matchRegex = QRegularExpression(regexStr);
            hasMatchRegex = matchRegex.isValid() && !matchRegex.pattern().isEmpty();
        }
    }

    QReadWriteLock lock;
    const KPluginMetaData runnerDescription;
    const AbstractRunner *runner;
    QList<RunnerSyntax> syntaxes;
    std::optional<bool> suspendMatching;
    int minLetterCount = 0;
    QRegularExpression matchRegex;
    bool hasMatchRegex = false;
    const bool hasUniqueResults = false;
    const bool hasWeakResults = false;
};
}
