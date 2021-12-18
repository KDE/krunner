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
#include <KProtocolInfo>
#include <KShell>

#include "abstractrunner.h"
#include "krunner_debug.h"
#include "querymatch.h"

#define LOCK_FOR_READ(d) d->lock.lockForRead();
#define LOCK_FOR_WRITE(d) d->lock.lockForWrite();
#define UNLOCK(d) d->lock.unlock();

namespace Plasma
{
class RunnerContextPrivate : public QSharedData
{
public:
    RunnerContextPrivate(RunnerContext *context)
        : QSharedData()
        , type(RunnerContext::UnknownType)
        , q(context)
    {
    }

    RunnerContextPrivate(const RunnerContextPrivate &p)
        : QSharedData()
        , launchCounts(p.launchCounts)
        , type(RunnerContext::None)
        , q(p.q)
    {
    }

    ~RunnerContextPrivate()
    {
    }

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
    /**
     * Determines type of query
                &&
     */
    void determineType()
    {
        // NOTE! this method must NEVER be called from
        // code that may be running in multiple threads
        // with the same data.
        type = RunnerContext::UnknownType;
        QString path = QDir::cleanPath(KShell::tildeExpand(term));

        int space = path.indexOf(QLatin1Char(' '));
        if (!QStandardPaths::findExecutable(path.left(space)).isEmpty()) {
            // it's a shell command if there's a space because that implies
            // that it has arguments!
            type = (space > 0) ? RunnerContext::ShellCommand : RunnerContext::Executable;
        } else {
            QUrl url = QUrl::fromUserInput(term);

            // QUrl::fromUserInput assigns http to everything if it cannot match it to
            // anything else. We do not want that.
            if (url.scheme() == QLatin1String("http")) {
                if (!term.startsWith(QLatin1String("http"))) {
                    url.setScheme(QString());
                }
            }

            const bool hasProtocol = !url.scheme().isEmpty();
            const bool isLocalProtocol = !hasProtocol || KProtocolInfo::protocolClass(url.scheme()) == QLatin1String(":local");
            if ((hasProtocol && ((!isLocalProtocol && !url.host().isEmpty()) || (isLocalProtocol && url.scheme() != QLatin1String("file"))))
                || term.startsWith(QLatin1String("\\\\"))) {
                // we either have a network protocol with a host, so we can show matches for it
                // or we have a non-file url that may be local so a host isn't required
                // or we have an UNC path (\\foo\bar)
                type = RunnerContext::NetworkLocation;
            } else if (isLocalProtocol) {
                // at this point in the game, we assume we have a path,
                // but if a path doesn't have any slashes
                // it's too ambiguous to be sure we're in a filesystem context
                path = !url.scheme().isEmpty() ? QDir::cleanPath(url.toLocalFile()) : path;
                if ((path.indexOf(QLatin1Char('/')) != -1 || path.indexOf(QLatin1Char('\\')) != -1)) {
                    QFileInfo info(path);
                    if (info.isSymLink()) {
                        path = info.canonicalFilePath();
                        info = QFileInfo(path);
                    }
                    if (info.isDir()) {
                        type = RunnerContext::Directory;
                        mimeType = QStringLiteral("inode/folder");
                    } else if (info.isFile()) {
                        type = RunnerContext::File;
                        QMimeDatabase db;
                        QMimeType mime = db.mimeTypeForFile(path);
                        if (!mime.isDefault()) {
                            mimeType = mime.name();
                        }
                    }
                }
            }
        }
    }
#endif

    void invalidate()
    {
        q = &s_dummyContext;
    }

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
    QueryMatch *findMatchById(const QString &id)
    {
        QueryMatch *match = nullptr;
        lock.lockForRead();
        for (auto &matchEntry : matches) {
            if (matchEntry.id() == id) {
                match = &matchEntry;
                break;
            }
        }
        lock.unlock();
        return match;
    }
#endif

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
    QString mimeType;
    QStringList enabledCategories;
    RunnerContext::Type type;
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

    QExplicitlySharedDataPointer<Plasma::RunnerContextPrivate> oldD = d;
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
    if (!d->matches.isEmpty()) {
        d->matches.clear();
        Q_EMIT matchesChanged();
    }

    d->term.clear();
    d->mimeType.clear();
    d->uniqueIds.clear();
    d->type = UnknownType;
    d->singleRunnerQueryMode = false;
    d->shouldIgnoreCurrentMatchForHistory = false;
}

void RunnerContext::setQuery(const QString &term)
{
    reset();

    if (term.isEmpty()) {
        return;
    }

    d->requestedText.clear(); // Invalidate this field whenever the query changes
    d->term = term;
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
    d->determineType();
#endif
}

QString RunnerContext::query() const
{
    // the query term should never be set after
    // a search starts. in fact, reset() ensures this
    // and setQuery(QString) calls reset()
    return d->term;
}
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)

void RunnerContext::setEnabledCategories(const QStringList &categories)
{
    d->enabledCategories = categories;
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
QStringList RunnerContext::enabledCategories() const
{
    return d->enabledCategories;
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
RunnerContext::Type RunnerContext::type() const
{
    return d->type;
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
QString RunnerContext::mimeType() const
{
    return d->mimeType;
}
#endif

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
    if (!isValid()) {
        // Bail out if the qptr is dirty
        return false;
    }

    QueryMatch m(match); // match must be non-const to modify relevance

    LOCK_FOR_WRITE(d)

    if (int count = d->launchCounts.value(m.id())) {
        m.setRelevance(m.relevance() + 0.05 * count);
    }
    d->addMatch(match);
    UNLOCK(d);
    Q_EMIT d->q->matchesChanged();

    return true;
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
bool RunnerContext::removeMatches(const QStringList matchIdList)
{
    bool success = false;
    for (const QString &id : matchIdList) {
        if (removeMatch(id)) {
            success = true;
        }
    }

    return success;
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
bool RunnerContext::removeMatch(const QString matchId)
{
    if (!isValid()) {
        return false;
    }
    LOCK_FOR_READ(d)
    const QueryMatch *match = d->findMatchById(matchId);
    UNLOCK(d)
    if (!match) {
        return false;
    }
    LOCK_FOR_WRITE(d)
    d->matches.removeAll(*match);
    UNLOCK(d)
    Q_EMIT d->q->matchesChanged();

    return true;
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 81)
bool RunnerContext::removeMatches(Plasma::AbstractRunner *runner)
{
    if (!isValid()) {
        return false;
    }

    QList<QueryMatch> presentMatchList;

    LOCK_FOR_READ(d)
    for (const QueryMatch &match : std::as_const(d->matches)) {
        if (match.runner() == runner) {
            presentMatchList << match;
        }
    }
    UNLOCK(d)

    if (presentMatchList.isEmpty()) {
        return false;
    }

    LOCK_FOR_WRITE(d)
    for (const QueryMatch &match : std::as_const(presentMatchList)) {
        d->matches.removeAll(match);
    }
    UNLOCK(d)

    Q_EMIT d->q->matchesChanged();
    return true;
}
#endif

QList<QueryMatch> RunnerContext::matches() const
{
    LOCK_FOR_READ(d)
    QList<QueryMatch> matches = d->matches;
    UNLOCK(d);
    return matches;
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 79)
QueryMatch RunnerContext::match(const QString &id) const
{
    LOCK_FOR_READ(d)
    const QueryMatch *match = d->findMatchById(id);
    UNLOCK(d)
    return match ? *match : QueryMatch(nullptr);
}
#endif

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

} // Plasma namespace

#include "moc_runnercontext.cpp"
