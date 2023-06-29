/*
    SPDX-FileCopyrightText: 2007, 2009 Ryan P. Bitanga <ryan.bitanga@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnerjobs_p.h"

#include <QMutexLocker>
#include <QTimer>

#include "krunner_debug.h"
#include "querymatch.h"
#include "runnermanager.h"

using ThreadWeaver::Job;
using ThreadWeaver::Queue;

namespace KRunner
{
DefaultRunnerPolicy::DefaultRunnerPolicy()
    : QueuePolicy()
    , m_cap(2)
{
}

DefaultRunnerPolicy::~DefaultRunnerPolicy()
{
}

DefaultRunnerPolicy &DefaultRunnerPolicy::instance()
{
    static DefaultRunnerPolicy policy;
    return policy;
}

bool DefaultRunnerPolicy::canRun(ThreadWeaver::JobPointer job)
{
    KRunner::AbstractRunner *runner = job.dynamicCast<FindMatchesJob>()->runner();
    QMutexLocker l(&m_mutex);

    if (m_runCounts[runner->name()] > m_cap) {
        return false;
    } else {
        ++m_runCounts[runner->name()];
        return true;
    }
}

void DefaultRunnerPolicy::free(ThreadWeaver::JobPointer job)
{
    KRunner::AbstractRunner *runner = job.dynamicCast<FindMatchesJob>()->runner();
    QMutexLocker l(&m_mutex);

    --m_runCounts[runner->name()];
}

void DefaultRunnerPolicy::release(ThreadWeaver::JobPointer job)
{
    free(job);
}

void DefaultRunnerPolicy::destructed(ThreadWeaver::JobInterface * /*job*/)
{
}

FindMatchesJob::FindMatchesJob(KRunner::AbstractRunner *runner, KRunner::RunnerContext *context, QObject *)
    : ThreadWeaver::Job()
    , m_context(*context, nullptr)
    , m_runner(runner)
{
    QMutexLocker l(mutex());
    Q_UNUSED(l);
    assignQueuePolicy(&DefaultRunnerPolicy::instance());
}

FindMatchesJob::~FindMatchesJob()
{
}

void FindMatchesJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *)
{
    if (m_context.isValid()) {
        m_runner->match(m_context);
    }
    Q_EMIT done(self);
}

int FindMatchesJob::priority() const
{
    return m_runner->priority();
}

KRunner::AbstractRunner *FindMatchesJob::runner() const
{
    return m_runner;
}

DelayedJobCleaner::DelayedJobCleaner(const QSet<QSharedPointer<FindMatchesJob>> &jobs, const QSet<AbstractRunner *> &runners)
    : QObject(Queue::instance())
    , m_weaver(Queue::instance())
    , m_jobs(jobs)
    , m_runners(runners)
{
    connect(m_weaver, &ThreadWeaver::QueueSignals::finished, this, &DelayedJobCleaner::checkIfFinished);

    for (auto it = m_jobs.constBegin(); it != m_jobs.constEnd(); ++it) {
        connect(it->data(), &FindMatchesJob::done, this, &DelayedJobCleaner::jobDone);
    }
}

DelayedJobCleaner::~DelayedJobCleaner()
{
    qDeleteAll(m_runners);
}

void DelayedJobCleaner::jobDone(ThreadWeaver::JobPointer job)
{
    auto runJob = job.dynamicCast<FindMatchesJob>();

    if (!runJob) {
        return;
    }

    m_jobs.remove(runJob);

    if (m_jobs.isEmpty()) {
        deleteLater();
    }
}

void DelayedJobCleaner::checkIfFinished()
{
    if (m_weaver->isIdle()) {
        m_jobs.clear();
        deleteLater();
    }
}

} // KRunner namespace

#include "moc_runnerjobs_p.cpp"
