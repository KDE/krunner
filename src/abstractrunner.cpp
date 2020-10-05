/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractrunner.h"
#include "abstractrunner_p.h"

#include <QAction>
#include <QElapsedTimer>
#include <QHash>
#include <QMimeData>
#include <QMutex>

#include <KSharedConfig>
#include <KLocalizedString>
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 65)
#include <Plasma/Package>
#endif

#include "krunner_debug.h"
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

AbstractRunner::AbstractRunner(const KPluginMetaData &pluginMetaData, QObject *parent)
    : QObject(parent),
      d(new AbstractRunnerPrivate(this))
{
    d->init(pluginMetaData);
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72) && KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
AbstractRunner::AbstractRunner(const KService::Ptr service, QObject *parent)
    : QObject(parent),
      d(new AbstractRunnerPrivate(this))
{
    d->init(service);
}
#endif

AbstractRunner::AbstractRunner(QObject *parent, const QVariantList &args)
    : QObject(parent),
      d(new AbstractRunnerPrivate(this))
{
    if (!args.isEmpty()) {
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72) && KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
        // backward-compatible support has metadata only as second argument
        // which we prefer of course
        if (args.size() > 1) {
            const KPluginMetaData metaData = args[1].value<KPluginMetaData>();
#else
            const KPluginMetaData metaData = args[0].value<KPluginMetaData>();
#endif
            if (metaData.isValid()) {
                d->init(metaData);
                return;
            }
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72) && KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
        }

        KService::Ptr service = KService::serviceByStorageId(args[0].toString());
        if (service) {
            d->init(service);
        }
#endif
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

    KConfigGroup runners(KSharedConfig::openConfig(QStringLiteral("krunnerrc")), "Runners");
    return KConfigGroup(&runners, group);
}

void AbstractRunner::reloadConfiguration()
{
}

void AbstractRunner::addSyntax(const RunnerSyntax &syntax)
{
    d->syntaxes.append(syntax);
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
void AbstractRunner::setDefaultSyntax(const RunnerSyntax &syntax)
{
    d->syntaxes.append(syntax);
    d->defaultSyntax = &(d->syntaxes.last());
}
#endif

void AbstractRunner::setSyntaxes(const QList<RunnerSyntax> &syntaxes)
{
    d->syntaxes = syntaxes;
}

QList<RunnerSyntax> AbstractRunner::syntaxes() const
{
    return d->syntaxes;
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
RunnerSyntax *AbstractRunner::defaultSyntax() const
{
    return d->defaultSyntax;
}
#endif

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
        d->fastRuns = 0;
        setSpeed(SlowSpeed);
    }

    if (slowed && runtime < fastEnoughTime && localContext.query().size() > 2) {
        ++d->fastRuns;

        if (d->fastRuns > 2) {
            // we reward slowed runners who bring back matches fast enough
            // 3 times in a row
            setSpeed(NormalSpeed);
        }
    }
}

QList<QAction*> AbstractRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    return match.isValid() ? match.actions() : QList<QAction *>();
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

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 71)
bool AbstractRunner::hasRunOptions()
{
    return d->hasRunOptions;
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 71)
void AbstractRunner::setHasRunOptions(bool hasRunOptions)
{
    d->hasRunOptions = hasRunOptions;
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 71)
void AbstractRunner::createRunOptions(QWidget *parent)
{
    Q_UNUSED(parent)
}
#endif

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
        return QIcon::fromTheme(d->runnerDescription.iconName());
    }

    return QIcon();
}

QString AbstractRunner::id() const
{
    if (d->runnerDescription.isValid()) {
        return d->runnerDescription.pluginId();
    }

    return objectName();
}

QString AbstractRunner::description() const
{
    if (d->runnerDescription.isValid()) {
        return d->runnerDescription.description();
    }

    return objectName();
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72)
KPluginInfo AbstractRunner::metadata() const
{
    return KPluginInfo::fromMetaData(d->runnerDescription);
}
#endif

KPluginMetaData AbstractRunner::metadata(RunnerReturnPluginMetaDataConstant) const
{
    return d->runnerDescription;
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 65)
Package AbstractRunner::package() const
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
    return Package();
QT_WARNING_POP
}
#endif

void AbstractRunner::init()
{
    reloadConfiguration();
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 73)
DataEngine *AbstractRunner::dataEngine(const QString &name) const
{
    return d->dataEngine(name);
}
#endif

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
    Q_EMIT matchingSuspended(suspend);
}

int AbstractRunner::minLetterCount() const
{
    return d->minLetterCount;
}

void AbstractRunner::setMinLetterCount(int count)
{
    d->minLetterCount = count;
}

QRegularExpression AbstractRunner::matchRegex() const
{
    return d->matchRegex;
}

void AbstractRunner::setMatchRegex(const QRegularExpression &regex)
{
    d->matchRegex = regex;
    d->hasMatchRegex = regex.isValid() && !regex.pattern().isEmpty();
}

void AbstractRunner::setTriggerWords(const QStringList &triggerWords)
{
    int minTriggerWordLetters = 0;
    QString constructedRegex = QStringLiteral("^");
    for (const QString &triggerWord : triggerWords) {
        // We want to link them with an or
        if (constructedRegex.length() > 1) {
            constructedRegex += QLatin1Char('|');
        }
        constructedRegex += QRegularExpression::escape(triggerWord);
        if (minTriggerWordLetters == 0 || triggerWord.length() < minTriggerWordLetters) {
            minTriggerWordLetters = triggerWord.length();
        }
    }
    // If we can reject the query because of the length we don't need the regex
    setMinLetterCount(minTriggerWordLetters);
    QRegularExpression regex(constructedRegex);
    regex.optimize();
    setMatchRegex(regex);
}

bool AbstractRunner::hasMatchRegex() const
{
    return d->hasMatchRegex;
}

void AbstractRunner::addMatch(const QueryMatch &match)
{
    if (match.idIsDetByData()) {
        d->matches.insert(match.data().toString(), match);
    } else {
        d->matches.insert(match.id(), match);
    }
}

void AbstractRunner::addMatch(const QString &id, const QueryMatch &match)
{
    d->matches.insert(id, match);
}

void AbstractRunner::removeMatch(const QString &id)
{
    d->matches.remove(id);
}

void AbstractRunner::clearMatches()
{
    d->matches.clear();
}

QueryMatch AbstractRunner::getMatch(const QString &id) const
{
    return d->matches.value(id);
}

QHash<QString, Plasma::QueryMatch> AbstractRunner::matches() const
{
    return d->matches;
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

void AbstractRunnerPrivate::init()
{
    if (runnerDescription.isValid()) {
        const auto rawData = runnerDescription.rawData();
        minLetterCount = rawData.value(QStringLiteral("X-Plasma-Runner-Min-Letter-Count")).toInt();
        if (rawData.contains(QStringLiteral("X-Plasma-Runner-Match-Regex"))) {
            matchRegex = QRegularExpression(rawData.value(QStringLiteral("X-Plasma-Runner-Match-Regex")).toString());
            hasMatchRegex = matchRegex.isValid() && !matchRegex.pattern().isEmpty();
        }
    }
}

void AbstractRunnerPrivate::init(const KPluginMetaData &pluginMetaData)
{
    runnerDescription = pluginMetaData;
    init();
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 72) && KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
void AbstractRunnerPrivate::init(const KService::Ptr service)
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
    const KPluginInfo pluginInfo(service);
    runnerDescription = pluginInfo.isValid() ? pluginInfo.toMetaData() : KPluginMetaData();
QT_WARNING_POP
    init();
}
#endif

void AbstractRunnerPrivate::init(const QString &path)
{
    runnerDescription = KPluginMetaData(path + QStringLiteral("/metadata.desktop"));
    init();
}

} // Plasma namespace


#include "moc_abstractrunner.cpp"
