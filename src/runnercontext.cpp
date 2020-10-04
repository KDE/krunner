/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnercontext.h"

#include <cmath>

#include <QReadWriteLock>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSharedData>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QUrl>

#include <KConfigGroup>
#include <KShell>
#include <KProtocolInfo>

#include "krunner_debug.h"
#include "abstractrunner.h"
#include "querymatch.h"

#define LOCK_FOR_READ(d) d->lock.lockForRead();
#define LOCK_FOR_WRITE(d) d->lock.lockForWrite();
#define UNLOCK(d) d->lock.unlock();

namespace Plasma
{

/*
Corrects the case of the last component in a path (e.g. /usr/liB -> /usr/lib)
path: The path to be processed.
correctCasePath: The corrected-case path
mustBeDir: Tells whether the last component is a folder or doesn't matter
Returns true on success and false on error, in case of error, correctCasePath is not modified
*/
bool correctLastComponentCase(const QString &path, QString &correctCasePath, const bool mustBeDir)
{
    // If the file already exists then no need to search for it.
    if (QFile::exists(path)) {
        correctCasePath = path;
        return true;
    }

    const QFileInfo pathInfo(path);

    const QDir fileDir = pathInfo.dir();

    const QString filename = pathInfo.fileName();

    const QStringList matchingFilenames = fileDir.entryList(QStringList(filename),
                                          mustBeDir ? QDir::Dirs : QDir::NoFilter);

    if (matchingFilenames.empty()) {
        return false;
    } else {
        if (fileDir.path().endsWith(QDir::separator())) {
            correctCasePath = fileDir.path() + matchingFilenames[0];
        } else {
            correctCasePath = fileDir.path() + QDir::separator() + matchingFilenames[0];
        }

        return true;
    }
}

/*
Corrects the case of a path (e.g. /uSr/loCAL/bIN -> /usr/local/bin)
path: The path to be processed.
corrected: The corrected-case path
Returns true on success and false on error, in case of error, corrected is not modified
*/
bool correctPathCase(const QString& path, QString &corrected)
{
    // early exit check
    if (QFile::exists(path)) {
        corrected = path;
        return true;
    }

    // path components
    QStringList components = QString(path).split(QDir::separator());

    if (components.size() < 1) {
        return false;
    }

    const bool mustBeDir = components.back().isEmpty();

    if (mustBeDir) {
        components.pop_back();
    }

    if (components.isEmpty()) {
        return true;
    }

    QString correctPath;
    const int initialComponents = components.size();
    for (int i = 0; i < initialComponents - 1; i++) {
        const QString tmp = components[0] + QDir::separator() + components[1];

        if (!correctLastComponentCase(tmp, correctPath, components.size() > 2 || mustBeDir)) {
            return false;
        }

        components.removeFirst();
        components[0] = correctPath;
    }

    corrected = correctPath;
    return true;
}

class RunnerContextPrivate : public QSharedData
{
    public:
        RunnerContextPrivate(RunnerContext *context)
            : QSharedData(),
              type(RunnerContext::UnknownType),
              q(context),
              singleRunnerQueryMode(false)
        {
        }

        RunnerContextPrivate(const RunnerContextPrivate &p)
            : QSharedData(),
              launchCounts(p.launchCounts),
              type(RunnerContext::None),
              q(p.q),
              singleRunnerQueryMode(false)
        {
        }

        ~RunnerContextPrivate()
        {
        }

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
                type = (space > 0) ? RunnerContext::ShellCommand :
                                     RunnerContext::Executable;
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
                if ((hasProtocol &&
                    ((!isLocalProtocol && !url.host().isEmpty()) ||
                     (isLocalProtocol && url.scheme() != QLatin1String("file"))))
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
                        QString correctCasePath;
                        if (correctPathCase(path, correctCasePath)) {
                            path = correctCasePath;
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
        }

        void invalidate()
        {
            q = &s_dummyContext;
        }

        QReadWriteLock lock;
        QList<QueryMatch> matches;
        QMap<QString, const QueryMatch*> matchesById;
        QHash<QString, int> launchCounts;
        QString term;
        QString mimeType;
        QStringList enabledCategories;
        RunnerContext::Type type;
        RunnerContext * q;
        static RunnerContext s_dummyContext;
        bool singleRunnerQueryMode;
};

RunnerContext RunnerContextPrivate::s_dummyContext;

RunnerContext::RunnerContext(QObject *parent)
    : QObject(parent),
      d(new RunnerContextPrivate(this))
{
}

//copy ctor
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
        d->matchesById.clear();
        d->matches.clear();
        Q_EMIT matchesChanged();
    }

    d->term.clear();
    d->mimeType.clear();
    d->type = UnknownType;
    d->singleRunnerQueryMode = false;
}

void RunnerContext::setQuery(const QString &term)
{
    reset();

    if (term.isEmpty()) {
        return;
    }

    d->term = term;
    d->determineType();
}

QString RunnerContext::query() const
{
    // the query term should never be set after
    // a search starts. in fact, reset() ensures this
    // and setQuery(QString) calls reset()
    return d->term;
}

void RunnerContext::setEnabledCategories(const QStringList& categories)
{
    d->enabledCategories = categories;
}

QStringList RunnerContext::enabledCategories() const
{
    return d->enabledCategories;
}

RunnerContext::Type RunnerContext::type() const
{
    return d->type;
}

QString RunnerContext::mimeType() const
{
    return d->mimeType;
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
       //Bail out if the query is empty or the qptr is dirty
        return false;
    }

    LOCK_FOR_WRITE(d)
    for (QueryMatch match : matches) {
        // Give previously launched matches a slight boost in relevance
        // The boost smoothly saturates to 0.5;
        if (int count = d->launchCounts.value(match.id())) {
            match.setRelevance(match.relevance() + 0.5 * (1-exp(-count*0.3)));
        }

        d->matches.append(match);
        d->matchesById.insert(match.id(), &d->matches.at(d->matches.size() - 1));
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

    d->matches.append(m);
    d->matchesById.insert(m.id(), &d->matches.at(d->matches.size() - 1));
    UNLOCK(d);
    Q_EMIT d->q->matchesChanged();

    return true;
}

bool RunnerContext::removeMatches(const QStringList matchIdList)
{
    if (!isValid()) {
        return false;
    }

    QStringList presentMatchIdList;
    QList<const QueryMatch*> presentMatchList;

    LOCK_FOR_READ(d)
    for (const QString &matchId : matchIdList) {
        const QueryMatch* match = d->matchesById.value(matchId, nullptr);
        if (match) {
            presentMatchList << match;
            presentMatchIdList << matchId;
        }
    }
    UNLOCK(d)

    if (presentMatchIdList.isEmpty()) {
        return false;
    }

    LOCK_FOR_WRITE(d)
    for (const QueryMatch *match : qAsConst(presentMatchList)) {
        d->matches.removeAll(*match);
    }
    for (const QString &matchId : qAsConst(presentMatchIdList)) {
        d->matchesById.remove(matchId);
    }
    UNLOCK(d)

    Q_EMIT d->q->matchesChanged();

    return true;
}

bool RunnerContext::removeMatch(const QString matchId)
{
    if (!isValid()) {
        return false;
    }
    LOCK_FOR_READ(d)
    const QueryMatch* match = d->matchesById.value(matchId, nullptr);
    UNLOCK(d)
    if (!match) {
        return false;
    }
    LOCK_FOR_WRITE(d)
    d->matches.removeAll(*match);
    d->matchesById.remove(matchId);
    UNLOCK(d)
    Q_EMIT d->q->matchesChanged();

    return true;
}

bool RunnerContext::removeMatches(Plasma::AbstractRunner *runner)
{
    if (!isValid()) {
        return false;
    }

    QList<QueryMatch> presentMatchList;

    LOCK_FOR_READ(d)
    for(const QueryMatch &match : qAsConst(d->matches)) {
        if (match.runner() == runner) {
            presentMatchList << match;
        }
    }
    UNLOCK(d)

    if (presentMatchList.isEmpty()) {
        return false;
    }

    LOCK_FOR_WRITE(d)
    for (const QueryMatch &match : qAsConst(presentMatchList)) {
        d->matchesById.remove(match.id());
        d->matches.removeAll(match);
    }
    UNLOCK(d)

    Q_EMIT d->q->matchesChanged();
    return true;
}

QList<QueryMatch> RunnerContext::matches() const
{
    LOCK_FOR_READ(d)
    QList<QueryMatch> matches = d->matches;
    UNLOCK(d);
    return matches;
}

QueryMatch RunnerContext::match(const QString &id) const
{
    LOCK_FOR_READ(d)
    const QueryMatch *match = d->matchesById.value(id, nullptr);
    UNLOCK(d)

    if (match) {
        return *match;
    }

    return QueryMatch(nullptr);
}

void RunnerContext::setSingleRunnerQueryMode(bool enabled)
{
    d->singleRunnerQueryMode = enabled;
}

bool RunnerContext::singleRunnerQueryMode() const
{
    return d->singleRunnerQueryMode;
}

void RunnerContext::restore(const KConfigGroup &config)
{
    const QStringList cfgList = config.readEntry("LaunchCounts", QStringList());

    const QRegularExpression re(QStringLiteral("(\\d*) (.+)"));
    for (const QString& entry : cfgList) {
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

} // Plasma namespace


#include "moc_runnercontext.cpp"
