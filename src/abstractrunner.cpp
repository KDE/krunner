/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "abstractrunner.h"
#include "abstractrunner_p.h"

#include <QAction>
#include <QElapsedTimer>
#include <QHash>
#include <QMimeData>
#include <QMutex>

#include "krunner_debug.h"
#include <kplugininfo.h>
#include <ksharedconfig.h>
#include <kservicetypetrader.h>
#include <klocalizedstring.h>
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 65)
#include <Plasma/Package>
#endif
#include <qstandardpaths.h>

#include "querymatch.h"
#include "runnercontext.h"

namespace Plasma
{

AbstractRunner::AbstractRunner(QObject *parent, const QString &path)
    : QObject(parent),
      d(new AbstractRunnerPrivate(this))
{
    d->init(path);
}

AbstractRunner::AbstractRunner(const KService::Ptr service, QObject *parent)
    : QObject(parent),
      d(new AbstractRunnerPrivate(this))
{
    d->init(service);
}

AbstractRunner::AbstractRunner(QObject *parent, const QVariantList &args)
    : QObject(parent),
      d(new AbstractRunnerPrivate(this))
{
    if (!args.isEmpty()) {
        KService::Ptr service = KService::serviceByStorageId(args[0].toString());
        if (service) {
            d->init(service);
        }
    }
}

AbstractRunner::~AbstractRunner()
{
    delete d;
}

KConfigGroup AbstractRunner::config() const
{
    QString group = id();
    if (group.isEmpty()) {
        group = QStringLiteral("UnnamedRunner");
    }

    KConfigGroup runners(KSharedConfig::openConfig(), "Runners");
    return KConfigGroup(&runners, group);
}

void AbstractRunner::reloadConfiguration()
{
}

void AbstractRunner::addSyntax(const RunnerSyntax &syntax)
{
    d->syntaxes.append(syntax);
}

void AbstractRunner::setDefaultSyntax(const RunnerSyntax &syntax)
{
    d->syntaxes.append(syntax);
    d->defaultSyntax = &(d->syntaxes.last());
}

void AbstractRunner::setSyntaxes(const QList<RunnerSyntax> &syntaxes)
{
    d->syntaxes = syntaxes;
}

QList<RunnerSyntax> AbstractRunner::syntaxes() const
{
    return d->syntaxes;
}

RunnerSyntax *AbstractRunner::defaultSyntax() const
{
    return d->defaultSyntax;
}

void AbstractRunner::performMatch(Plasma::RunnerContext &localContext)
{
    static const int reasonableRunTime = 1500;
    static const int fastEnoughTime = 250;

    if (d->suspendMatching) {
        return;
    }

    QElapsedTimer time;
    time.start();

    //The local copy is already obtained in the job
    match(localContext);

    // automatically rate limit runners that become slooow
    const int runtime = time.elapsed();
    bool slowed = speed() == SlowSpeed;

    if (!slowed && runtime > reasonableRunTime) {
        // we punish runners that return too slowly, even if they don't bring
        // back matches
#ifndef NDEBUG
        // qCDebug(KRUNNER) << id() << "runner is too slow, putting it on the back burner.";
#endif
        d->fastRuns = 0;
        setSpeed(SlowSpeed);
    }

    if (slowed && runtime < fastEnoughTime && localContext.query().size() > 2) {
        ++d->fastRuns;

        if (d->fastRuns > 2) {
            // we reward slowed runners who bring back matches fast enough
            // 3 times in a row
#ifndef NDEBUG
            // qCDebug(KRUNNER) << id() << "runner is faster than we thought, kicking it up a notch";
#endif
            setSpeed(NormalSpeed);
        }
    }
}

QList<QAction*> AbstractRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)
    QList<QAction*> ret;
    return ret;
}

QAction* AbstractRunner::addAction(const QString &id, const QIcon &icon, const QString &text)
{
    QAction *a = new QAction(icon, text, this);
    d->actions.insert(id, a);
    return a;
}

void AbstractRunner::addAction(const QString &id, QAction *action)
{
    d->actions.insert(id, action);
}

void AbstractRunner::removeAction(const QString &id)
{
    QAction *a = d->actions.take(id);
    delete a;
}

QAction* AbstractRunner::action(const QString &id) const
{
    return d->actions.value(id);
}

QHash<QString, QAction*> AbstractRunner::actions() const
{
    return d->actions;
}

void AbstractRunner::clearActions()
{
    qDeleteAll(d->actions);
    d->actions.clear();
}

QMimeData *AbstractRunner::mimeDataForMatch(const QueryMatch &match)
{
        if (match.urls().isEmpty()) {
        return nullptr;
    }
    QMimeData *result = new QMimeData();
    result->setUrls(match.urls());
    return result;
}

bool AbstractRunner::hasRunOptions()
{
    return d->hasRunOptions;
}

void AbstractRunner::setHasRunOptions(bool hasRunOptions)
{
    d->hasRunOptions = hasRunOptions;
}

void AbstractRunner::createRunOptions(QWidget *parent)
{
    Q_UNUSED(parent);
}

AbstractRunner::Speed AbstractRunner::speed() const
{
    // the only time the read lock will fail is if we were slow are going to speed up
    // or if we were fast and are going to slow down; so don't wait in this case, just
    // say we're slow. we either will be soon or were just a moment ago and it doesn't
    // hurt to do one more run the slow way
    if (!d->speedLock.tryLockForRead()) {
        return SlowSpeed;
    }
    Speed s = d->speed;
    d->speedLock.unlock();
    return s;
}

void AbstractRunner::setSpeed(Speed speed)
{
    d->speedLock.lockForWrite();
    d->speed = speed;
    d->speedLock.unlock();
}

AbstractRunner::Priority AbstractRunner::priority() const
{
    return d->priority;
}

void AbstractRunner::setPriority(Priority priority)
{
    d->priority = priority;
}

RunnerContext::Types AbstractRunner::ignoredTypes() const
{
    return d->blackListed;
}

void AbstractRunner::setIgnoredTypes(RunnerContext::Types types)
{
    d->blackListed = types;
}

void AbstractRunner::run(const Plasma::RunnerContext &search, const Plasma::QueryMatch &action)
{
    Q_UNUSED(search);
    Q_UNUSED(action);
}

QStringList AbstractRunner::categories() const
{
    return QStringList() << name();
}

QIcon AbstractRunner::categoryIcon(const QString&) const
{
    return icon();
}

void AbstractRunner::match(Plasma::RunnerContext &)
{
}

QString AbstractRunner::name() const
{
    if (d->runnerDescription.isValid()) {
        return d->runnerDescription.name();
    }

    return objectName();
}

QIcon AbstractRunner::icon() const
{
    if (d->runnerDescription.isValid()) {
        return QIcon::fromTheme(d->runnerDescription.icon());
    }

    return QIcon();
}

QString AbstractRunner::id() const
{
    if (d->runnerDescription.isValid()) {
        return d->runnerDescription.pluginName();
    }

    return objectName();
}

QString AbstractRunner::description() const
{
    if (d->runnerDescription.isValid()) {
        return d->runnerDescription.property(QStringLiteral("Comment")).toString();
    }

    return objectName();
}

KPluginInfo AbstractRunner::metadata() const
{
    return d->runnerDescription;
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 65)
Package AbstractRunner::package() const
{
    return Package();
}
#endif

void AbstractRunner::init()
{
    reloadConfiguration();
}

DataEngine *AbstractRunner::dataEngine(const QString &name) const
{
    return d->dataEngine(name);
}

bool AbstractRunner::isMatchingSuspended() const
{
    return d->suspendMatching;
}

void AbstractRunner::suspendMatching(bool suspend)
{
    if (d->suspendMatching == suspend) {
        return;
    }

    d->suspendMatching = suspend;
    emit matchingSuspended(suspend);
}

AbstractRunnerPrivate::AbstractRunnerPrivate(AbstractRunner *r)
    : priority(AbstractRunner::NormalPriority),
      speed(AbstractRunner::NormalSpeed),
      blackListed(RunnerContext::None),
      runner(r),
      fastRuns(0),
      defaultSyntax(nullptr),
      hasRunOptions(false),
      suspendMatching(false)
{
}

AbstractRunnerPrivate::~AbstractRunnerPrivate()
{
}

void AbstractRunnerPrivate::init(const KService::Ptr service)
{
    runnerDescription = KPluginInfo(service);
}

void AbstractRunnerPrivate::init(const QString &path)
{
    runnerDescription = KPluginInfo(path + QStringLiteral("/metadata.desktop"));
    const QString api = runnerDescription.property(QStringLiteral("X-Plasma-API")).toString();
}

} // Plasma namespace


#include "moc_abstractrunner.cpp"
