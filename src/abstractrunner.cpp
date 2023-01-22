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

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include "krunner_debug.h"

namespace Plasma
{
AbstractRunner::AbstractRunner(QObject *parent, const KPluginMetaData &pluginMetaData, const QVariantList &args)
    : QObject(parent)
    , d(new AbstractRunnerPrivate(this))
{
    Q_UNUSED(args)

    d->init(pluginMetaData);
}

AbstractRunner::~AbstractRunner() = default;

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

bool AbstractRunner::hasUniqueResults()
{
    return d->hasUniqueResults;
}

bool AbstractRunner::hasWeakResults()
{
    return d->hasWeakResults;
}

void AbstractRunner::setSyntaxes(const QList<RunnerSyntax> &syntaxes)
{
    d->syntaxes = syntaxes;
}

QList<RunnerSyntax> AbstractRunner::syntaxes() const
{
    return d->syntaxes;
}

QList<QAction *> AbstractRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    return match.isValid() ? match.actions() : QList<QAction *>();
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

AbstractRunner::Priority AbstractRunner::priority() const
{
    return d->priority;
}

void AbstractRunner::setPriority(Priority priority)
{
    d->priority = priority;
}

void AbstractRunner::run(const Plasma::RunnerContext &search, const Plasma::QueryMatch &action)
{
    Q_UNUSED(search);
    Q_UNUSED(action);
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
    return QIcon::fromTheme(d->runnerDescription.iconName());
}

QString AbstractRunner::id() const
{
    return d->runnerDescription.pluginId();
}

QString AbstractRunner::description() const
{
    return d->runnerDescription.description();
}

KPluginMetaData AbstractRunner::metadata() const
{
    return d->runnerDescription;
}

void AbstractRunner::init()
{
    reloadConfiguration();
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
    setMatchRegex(QRegularExpression(constructedRegex));
}

bool AbstractRunner::hasMatchRegex() const
{
    return d->hasMatchRegex;
}

AbstractRunnerPrivate::AbstractRunnerPrivate(AbstractRunner *r)
    : priority(AbstractRunner::NormalPriority)
    , blackListed(RunnerContext::None)
    , runner(r)
    , fastRuns(0)
    , defaultSyntax(nullptr)
    , hasRunOptions(false)
    , suspendMatching(false)
{
}

AbstractRunnerPrivate::~AbstractRunnerPrivate()
{
}

void AbstractRunnerPrivate::init()
{
    minLetterCount = runnerDescription.value(QStringLiteral("X-Plasma-Runner-Min-Letter-Count"), 0);
    if (runnerDescription.isValid()) {
        const auto rawData = runnerDescription.rawData();
        if (rawData.contains(QStringLiteral("X-Plasma-Runner-Match-Regex"))) {
            matchRegex = QRegularExpression(rawData.value(QStringLiteral("X-Plasma-Runner-Match-Regex")).toString());
            hasMatchRegex = matchRegex.isValid() && !matchRegex.pattern().isEmpty();
        }
        hasUniqueResults = runnerDescription.value(QStringLiteral("X-Plasma-Runner-Unique-Results"), false);
        hasWeakResults = runnerDescription.value(QStringLiteral("X-Plasma-Runner-Weak-Results"), false);
    }
}

void AbstractRunnerPrivate::init(const KPluginMetaData &pluginMetaData)
{
    runnerDescription = pluginMetaData;
    init();
}

} // Plasma namespace

#include "moc_abstractrunner.cpp"
