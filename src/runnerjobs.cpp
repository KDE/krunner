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

namespace Plasma
{
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
DelayedRunnerPolicy::DelayedRunnerPolicy()
    : QueuePolicy()
{
}

DelayedRunnerPolicy::~DelayedRunnerPolicy()
{
}

DelayedRunnerPolicy &DelayedRunnerPolicy::instance()
{
    static DelayedRunnerPolicy policy;
    return policy;
}

bool DelayedRunnerPolicy::canRun(ThreadWeaver::JobPointer job)
{
    QSharedPointer<FindMatchesJob> aJob(job.dynamicCast<FindMatchesJob>());
    if (QTimer *t = aJob->delayTimer()) {
        // If the timer is active, the required delay has not been reached
        return !t->isActive(); // DATA RACE!  (with QTimer start/stop from runnermanager.cpp)
    }

    return true;
}

void DelayedRunnerPolicy::free(ThreadWeaver::JobPointer job)
{
    Q_UNUSED(job)
}

void DelayedRunnerPolicy::release(ThreadWeaver::JobPointer job)
{
    free(job);
}

void DelayedRunnerPolicy::destructed(ThreadWeaver::JobInterface *job)
{
    Q_UNUSED(job)
}
#endif

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
    Plasma::AbstractRunner *runner = job.dynamicCast<FindMatchesJob>()->runner();
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
    Plasma::AbstractRunner *runner = job.dynamicCast<FindMatchesJob>()->runner();
    QMutexLocker l(&m_mutex);

    --m_runCounts[runner->name()];
}

void DefaultRunnerPolicy::release(ThreadWeaver::JobPointer job)
{
    free(job);
}

void DefaultRunnerPolicy::destructed(ThreadWeaver::JobInterface *job)
{
    Q_UNUSED(job)
}

////////////////////
// Jobs
////////////////////

FindMatchesJob::FindMatchesJob(Plasma::AbstractRunner *runner, Plasma::RunnerContext *context, QObject *)
    : ThreadWeaver::Job()
    , m_context(*context, nullptr)
    , m_runner(runner)
{
    QMutexLocker l(mutex());
    Q_UNUSED(l);
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
    if (runner->speed() == Plasma::AbstractRunner::SlowSpeed) {
        assignQueuePolicy(&DelayedRunnerPolicy::instance());
    } else {
        assignQueuePolicy(&DefaultRunnerPolicy::instance());
    }
#else
    assignQueuePolicy(&DefaultRunnerPolicy::instance());
#endif
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

Plasma::AbstractRunner *FindMatchesJob::runner() const
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

} // Plasma namespace
