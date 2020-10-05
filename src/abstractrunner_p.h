/*
    SPDX-FileCopyrightText: 2006-2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ABSTRACTRUNNER_P_H
#define ABSTRACTRUNNER_P_H

#include <QReadWriteLock>
#include <QRegularExpression>

#include <KPluginMetaData>

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 73)
#include <Plasma/DataEngineConsumer>
#endif

namespace Plasma
{

class AbstractRunner;

class AbstractRunnerPrivate
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 73)
    : public DataEngineConsumer
#endif
{
public:
    AbstractRunnerPrivate(AbstractRunner *r);
    ~AbstractRunnerPrivate();
    void init();
    void init(const KPluginMetaData &pluginMetaData);
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72) && KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
    void init(const KService::Ptr service);
#endif
    void init(const QString &path);

    AbstractRunner::Priority priority;
    AbstractRunner::Speed speed;
    RunnerContext::Types blackListed;
    KPluginMetaData runnerDescription;
    AbstractRunner *runner;
    int fastRuns;
    QReadWriteLock speedLock;
    QHash<QString, QAction*> actions;
    QHash<QString, QueryMatch> matches;
    QList<RunnerSyntax> syntaxes;
    RunnerSyntax *defaultSyntax;
    bool hasRunOptions : 1;
    bool suspendMatching : 1;
    int minLetterCount = 0;
    QRegularExpression matchRegex;
    bool hasMatchRegex = false;
};

} // namespace Plasma
#endif


