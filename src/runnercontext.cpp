/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnercontext.h"

#include <cmath>

#include <QPointer>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QSharedData>
#include <QUrl>

#include <KConfigGroup>
#include <KShell>

#include "abstractrunner.h"
#include "abstractrunner_p.h"
#include "querymatch.h"
#include "runnermanager.h"

namespace KRunner
{
class RunnerContextPrivate : public QSharedData
{
public:
    explicit RunnerContextPrivate(RunnerManager *manager)
        : QSharedData()
        , m_manager(manager)
    {
    }

    RunnerContextPrivate(const RunnerContextPrivate &p)
        : QSharedData(p)
        , m_manager(p.m_manager)
    {
    }

    ~RunnerContextPrivate() = default;

    void invalidate()
    {
        m_isValid = false;
    }

    void addMatch(const QueryMatch &match)
    {
        if (match.runner() && match.runner()->d->hasUniqueResults) {
            if (uniqueIds.contains(match.id())) {
                const QueryMatch &existentMatch = uniqueIds.value(match.id());
                if (existentMatch.runner() && existentMatch.runner()->d->hasWeakResults) {
                    // There is an existing match with the same ID and we are allowed to replace it
                    matches.removeOne(existentMatch);
                    matches.append(match);
                }
            } else {
                // There is no existing match with the same id
                uniqueIds.insert(match.id(), match);
                matches.append(match);
            }
        } else {
            // Runner has the unique results property not set
            matches.append(match);
        }
    }

    void matchesChanged()
    {
        if (m_manager) {
            QMetaObject::invokeMethod(m_manager, "onMatchesChanged");
        }
    }

    QReadWriteLock lock;
    QPointer<RunnerManager> m_manager;
    bool m_isValid = true;
    QList<QueryMatch> matches;
    QString term;
    bool singleRunnerQueryMode = false;
    bool shouldIgnoreCurrentMatchForHistory = false;
    QMap<QString, QueryMatch> uniqueIds;
    QString requestedText;
    int requestedCursorPosition = 0;
    qint64 queryStartTs = 0;
};

RunnerContext::RunnerContext(RunnerManager *manager)
    : d(new RunnerContextPrivate(manager))
{
}

// copy ctor
RunnerContext::RunnerContext(const RunnerContext &other)
{
    QReadLocker locker(&other.d->lock);
    d = other.d;
}

RunnerContext::~RunnerContext() = default;

RunnerContext &RunnerContext::operator=(const RunnerContext &other)
{
    if (this->d == other.d) {
        return *this;
    }

    auto oldD = d; // To avoid the old ptr getting destroyed while the mutex is locked
    QWriteLocker locker(&d->lock);
    QReadLocker otherLocker(&other.d->lock);
    d = other.d;
    return *this;
}

/*!
 * Resets the search term for this object.
 * This removes all current matches in the process and
 * turns off single runner query mode.
 * Copies of this object that are used by runner are invalidated
 * and adding matches will be a noop.
 */
void RunnerContext::reset()
{
    {
        QWriteLocker locker(&d->lock);
        // We will detach if we are a copy of someone. But we will reset
        // if we are the 'main' context others copied from. Resetting
        // one RunnerContext makes all the copies obsolete.

        // We need to mark the q pointer of the detached RunnerContextPrivate
        // as dirty on detach to avoid receiving results for old queries
        d->invalidate();
    }

    d.detach();
    // But out detached version is valid!
    d->m_isValid = true;

    // we still have to remove all the matches, since if the
    // ref count was 1 (e.g. only the RunnerContext is using
    // the dptr) then we won't get a copy made
    d->matches.clear();
    d->term.clear();
    d->matchesChanged();

    d->uniqueIds.clear();
    d->singleRunnerQueryMode = false;
    d->shouldIgnoreCurrentMatchForHistory = false;
}

void RunnerContext::setQuery(const QString &term)
{
    if (!this->query().isEmpty()) {
        reset();
    }

    if (term.isEmpty()) {
        return;
    }

    d->requestedText.clear(); // Invalidate this field whenever the query changes
    d->term = term;
}

QString RunnerContext::query() const
{
    // the query term should never be set after
    // a search starts. in fact, reset() ensures this
    // and setQuery(QString) calls reset()
    return d->term;
}

bool RunnerContext::isValid() const
{
    QReadLocker locker(&d->lock);
    return d->m_isValid;
}

bool RunnerContext::addMatches(const QList<QueryMatch> &matches)
{
    if (matches.isEmpty() || !isValid()) {
        // Bail out if the query is empty or the qptr is dirty
        return false;
    }

    {
        QWriteLocker locker(&d->lock);
        for (const QueryMatch &match : matches) {
            d->addMatch(match);
        }
    }
    d->matchesChanged();

    return true;
}

bool RunnerContext::addMatch(const QueryMatch &match)
{
    return addMatches({match});
}

QList<QueryMatch> RunnerContext::matches() const
{
    QReadLocker locker(&d->lock);
    QList<QueryMatch> matches = d->matches;
    return matches;
}

void RunnerContext::requestQueryStringUpdate(const QString &text, int cursorPosition) const
{
    d->requestedText = text;
    d->requestedCursorPosition = cursorPosition;
}

void RunnerContext::setSingleRunnerQueryMode(bool enabled)
{
    d->singleRunnerQueryMode = enabled;
}

bool RunnerContext::singleRunnerQueryMode() const
{
    return d->singleRunnerQueryMode;
}

void RunnerContext::ignoreCurrentMatchForHistory() const
{
    d->shouldIgnoreCurrentMatchForHistory = true;
}

bool RunnerContext::shouldIgnoreCurrentMatchForHistory() const
{
    return d->shouldIgnoreCurrentMatchForHistory;
}

void RunnerContext::restore([[maybe_unused]] const KConfigGroup &config)
{
    // TODO KF7: Drop
}

void RunnerContext::save([[maybe_unused]] KConfigGroup &config)
{
    // TODO KF7: Drop
}

void RunnerContext::increaseLaunchCount([[maybe_unused]] const QueryMatch &match)
{
    // TODO KF7: Drop
}

QString RunnerContext::requestedQueryString() const
{
    return d->requestedText;
}
int RunnerContext::requestedCursorPosition() const
{
    return d->requestedCursorPosition;
}

void RunnerContext::setJobStartTs(qint64 queryStartTs)
{
    d->queryStartTs = queryStartTs;
}
QString RunnerContext::runnerJobId(AbstractRunner *runner) const
{
    return QLatin1String("%1-%2-%3").arg(runner->id(), query(), QString::number(d->queryStartTs));
}

} // KRunner namespace
