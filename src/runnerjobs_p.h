/*
    SPDX-FileCopyrightText: 2007, 2009 Ryan P. Bitanga <ryan.bitanga@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_RUNNERJOBS_P_H
#define PLASMA_RUNNERJOBS_P_H

#include <QHash>
#include <QMutex>
#include <QSet>

#include <ThreadWeaver/Job>
#include <ThreadWeaver/Queue>
#include <ThreadWeaver/QueuePolicy>

#include "abstractrunner.h"

using ThreadWeaver::Job;

class QTimer;

namespace Plasma
{
// QueuePolicy that limits the instances of a particular runner
class DefaultRunnerPolicy : public ThreadWeaver::QueuePolicy
{
public:
    ~DefaultRunnerPolicy() override;

    static DefaultRunnerPolicy &instance();

    void setCap(int cap)
    {
        m_cap = cap;
    }
    int cap() const
    {
        return m_cap;
    }

    bool canRun(ThreadWeaver::JobPointer job) override;
    void free(ThreadWeaver::JobPointer job) override;
    void release(ThreadWeaver::JobPointer job) override;
    void destructed(ThreadWeaver::JobInterface *job) override;

private:
    DefaultRunnerPolicy();

    int m_cap;
    QHash<QString, int> m_runCounts;
    QMutex m_mutex;
};

/*
 * FindMatchesJob class
 * Class to run queries in different threads
 */
class FindMatchesJob : public QObject, public Job
{
    Q_OBJECT
public:
    FindMatchesJob(Plasma::AbstractRunner *runner, Plasma::RunnerContext *context, QObject *parent = nullptr);
    ~FindMatchesJob() override;

    int priority() const override;
    Plasma::AbstractRunner *runner() const;

Q_SIGNALS:
    void done(ThreadWeaver::JobPointer self);

protected:
    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override;

private:
    Plasma::RunnerContext m_context;
    Plasma::AbstractRunner *m_runner;
};

class DelayedJobCleaner : public QObject
{
    Q_OBJECT
public:
    explicit DelayedJobCleaner(const QSet<QSharedPointer<FindMatchesJob>> &jobs, const QSet<AbstractRunner *> &runners = QSet<AbstractRunner *>());
    ~DelayedJobCleaner() override;

private Q_SLOTS:
    void jobDone(ThreadWeaver::JobPointer);
    void checkIfFinished();

private:
    ThreadWeaver::Queue *m_weaver;
    QSet<QSharedPointer<FindMatchesJob>> m_jobs;
    QSet<AbstractRunner *> m_runners;
};

}

#endif // PLASMA_RUNNERJOBS_P_H
