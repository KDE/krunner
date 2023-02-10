/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnercontext.h"

#include <cmath>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QSharedData>
#include <QStandardPaths>
#include <QUrl>

#include <KConfigGroup>
#include <KShell>

#include "abstractrunner.h"
#include "krunner_debug.h"
#include "querymatch.h"

#define LOCK_FOR_READ(d) d->lock.lockForRead();
#define LOCK_FOR_WRITE(d) d->lock.lockForWrite();
#define UNLOCK(d) d->lock.unlock();

namespace KRunner
{
class RunnerContextPrivate : public QSharedData
{
public:
    explicit RunnerContextPrivate(RunnerContext *context)
        : QSharedData()
        , q(context)
    {
    }

    explicit RunnerContextPrivate(const RunnerContextPrivate &p)
        : QSharedData()
        , launchCounts(p.launchCounts)
        , q(p.q)
    {
    }

    ~RunnerContextPrivate()
    {
    }

    void invalidate()
    {
        q = &s_dummyContext;
    }

    void addMatch(const QueryMatch &match)
    {
        if (match.runner() && match.runner()->hasUniqueResults()) {
            if (uniqueIds.contains(match.id())) {
                const QueryMatch &existentMatch = uniqueIds.value(match.id());
                if (existentMatch.runner() && existentMatch.runner()->hasWeakResults()) {
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

    QReadWriteLock lock;
    QList<QueryMatch> matches;
    QHash<QString, int> launchCounts;
    QString term;
    RunnerContext *q;
    static RunnerContext s_dummyContext;
    bool singleRunnerQueryMode = false;
    bool shouldIgnoreCurrentMatchForHistory = false;
    QMap<QString, QueryMatch> uniqueIds;
    QString requestedText;
    int requestedCursorPosition = 0;
};

RunnerContext RunnerContextPrivate::s_dummyContext;

RunnerContext::RunnerContext(QObject *parent)
    : QObject(parent)
    , d(new RunnerContextPrivate(this))
{
}

// copy ctor
RunnerContext::RunnerContext(RunnerContext &other, QObject *parent)
    : QObject(parent)
{
    LOCK_FOR_READ(other.d)
    d = other.d;
    UNLOCK(other.d)
}

RunnerContext::~RunnerContext()
{
}

RunnerContext &RunnerContext::operator=(const RunnerContext &other)
{
    if (this->d == other.d) {
        return *this;
    }

    QExplicitlySharedDataPointer<KRunner::RunnerContextPrivate> oldD = d;
    LOCK_FOR_WRITE(d)
    LOCK_FOR_READ(other.d)
    d = other.d;
    UNLOCK(other.d)
    UNLOCK(oldD)
    return *this;
}

void RunnerContext::reset()
{
    LOCK_FOR_WRITE(d);
    // We will detach if we are a copy of someone. But we will reset
    // if we are the 'main' context others copied from. Resetting
    // one RunnerContext makes all the copies obsolete.

    // We need to mark the q pointer of the detached RunnerContextPrivate
    // as dirty on detach to avoid receiving results for old queries
    d->invalidate();
    UNLOCK(d);

    d.detach();

    // Now that we detached the d pointer we need to reset its q pointer

    d->q = this;

    // we still have to remove all the matches, since if the
    // ref count was 1 (e.g. only the RunnerContext is using
    // the dptr) then we won't get a copy made
    d->matches.clear();
    d->term.clear();
    Q_EMIT matchesChanged();

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
    // if our qptr is dirty, we aren't useful anymore
    LOCK_FOR_READ(d)
    const bool valid = (d->q != &(d->s_dummyContext));
    UNLOCK(d)
    return valid;
}

bool RunnerContext::addMatches(const QList<QueryMatch> &matches)
{
    if (matches.isEmpty() || !isValid()) {
        // Bail out if the query is empty or the qptr is dirty
        return false;
    }

    LOCK_FOR_WRITE(d)
    for (QueryMatch match : matches) {
        // Give previously launched matches a slight boost in relevance
        // The boost smoothly saturates to 0.5;
        if (int count = d->launchCounts.value(match.id())) {
            match.setRelevance(match.relevance() + 0.5 * (1 - exp(-count * 0.3)));
        }
        d->addMatch(match);
    }
    UNLOCK(d);
    // A copied searchContext may share the d pointer,
    // we always want to sent the signal of the object that created
    // the d pointer
    Q_EMIT d->q->matchesChanged();

    return true;
}

bool RunnerContext::addMatch(const QueryMatch &match)
{
    return addMatches({match});
}

QList<QueryMatch> RunnerContext::matches() const
{
    LOCK_FOR_READ(d)
    QList<QueryMatch> matches = d->matches;
    UNLOCK(d);
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

/**
 * Sets the launch counts for the associated match ids
 *
 * If a runner adds a match to this context, the context will check if the
 * match id has been launched before and increase the matches relevance
 * correspondingly. In this manner, any front end can implement adaptive search
 * by sorting items according to relevance.
 *
 * @param config the config group where launch data was stored
 */
void RunnerContext::restore(const KConfigGroup &config)
{
    const QStringList cfgList = config.readEntry("LaunchCounts", QStringList());

    static const QRegularExpression re(QStringLiteral("(\\d*) (.+)"));
    for (const QString &entry : cfgList) {
        const QRegularExpressionMatch match = re.match(entry);
        if (!match.hasMatch()) {
            continue;
        }
        const int count = match.captured(1).toInt();
        const QString id = match.captured(2);
        d->launchCounts[id] = count;
    }
}

void RunnerContext::save(KConfigGroup &config)
{
    QStringList countList;

    typedef QHash<QString, int>::const_iterator Iterator;
    Iterator end = d->launchCounts.constEnd();
    for (Iterator i = d->launchCounts.constBegin(); i != end; ++i) {
        countList << QStringLiteral("%2 %1").arg(i.key()).arg(i.value());
    }

    config.writeEntry("LaunchCounts", countList);
    config.sync();
}

void RunnerContext::run(const QueryMatch &match)
{
    ++d->launchCounts[match.id()];
    match.run(*this);
}

QString RunnerContext::requestedQueryString() const
{
    return d->requestedText;
}
int RunnerContext::requestedCursorPosition() const
{
    return d->requestedCursorPosition;
}

} // KRunner namespace

#include "moc_runnercontext.cpp"
