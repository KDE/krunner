/*
    SPDX-FileCopyrightText: 2006-2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ABSTRACTRUNNER_P_H
#define ABSTRACTRUNNER_P_H

#include <QRegularExpression>
#include <KPluginMetaData>

namespace KRunner
{
class AbstractRunner;

class AbstractRunnerPrivate
{
public:
    explicit AbstractRunnerPrivate(AbstractRunner *r, const KPluginMetaData &pluginMetaData);
    AbstractRunner::Priority priority = AbstractRunner::NormalPriority;
    const KPluginMetaData runnerDescription;
    AbstractRunner *runner;
    QHash<QString, QAction *> actions;
    QList<RunnerSyntax> syntaxes;
    bool suspendMatching = false;
    int minLetterCount = 0;
    QRegularExpression matchRegex;
    bool hasMatchRegex = false;
    bool hasUniqueResults = false;
    bool hasWeakResults = false;
};

} // namespace KRunner
#endif
