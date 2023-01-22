/*
    SPDX-FileCopyrightText: 2006-2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ABSTRACTRUNNER_P_H
#define ABSTRACTRUNNER_P_H

#include <QReadWriteLock>
#include <QRegularExpression>

#include <KPluginMetaData>

namespace Plasma
{
class AbstractRunner;

class AbstractRunnerPrivate
{
public:
    AbstractRunnerPrivate(AbstractRunner *r);
    ~AbstractRunnerPrivate();
    void init();
    void init(const KPluginMetaData &pluginMetaData);
    AbstractRunner::Priority priority;
    RunnerContext::Types blackListed;
    KPluginMetaData runnerDescription;
    AbstractRunner *runner;
    int fastRuns;
    QReadWriteLock speedLock;
    QHash<QString, QAction *> actions;
    QList<RunnerSyntax> syntaxes;
    RunnerSyntax *defaultSyntax;
    bool hasRunOptions : 1;
    bool suspendMatching : 1;
    int minLetterCount = 0;
    QRegularExpression matchRegex;
    bool hasMatchRegex = false;
    bool hasUniqueResults = false;
    bool hasWeakResults = false;
};

} // namespace Plasma
#endif
