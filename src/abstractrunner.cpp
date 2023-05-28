/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractrunner.h"

#include <QAction>
#include <QHash>
#include <QMimeData>
#include <QMutex>
#include <QRegularExpression>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include "krunner_debug.h"

namespace KRunner
{
class AbstractRunnerPrivate
{
public:
    explicit AbstractRunnerPrivate(AbstractRunner *r, const KPluginMetaData &pluginMetaData);
    const KPluginMetaData runnerDescription;
    AbstractRunner *runner;
    QList<RunnerSyntax> syntaxes;
    bool suspendMatching = false;
    int minLetterCount = 0;
    QRegularExpression matchRegex;
    bool hasMatchRegex = false;
    bool hasUniqueResults = false;
    bool hasWeakResults = false;
};

AbstractRunner::AbstractRunner(QObject *parent, const KPluginMetaData &pluginMetaData, const QVariantList & /*args*/)
    : QObject(parent)
    , d(new AbstractRunnerPrivate(this, pluginMetaData))
{
}

AbstractRunner::~AbstractRunner() = default;

KConfigGroup AbstractRunner::config() const
{
    KConfigGroup runners(KSharedConfig::openConfig(QStringLiteral("krunnerrc")), "Runners");
    return runners.group(id());
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

QList<QAction *> AbstractRunner::actionsForMatch(const KRunner::QueryMatch &match)
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

void AbstractRunner::run(const KRunner::RunnerContext & /*search*/, const KRunner::QueryMatch & /*action*/)
{
}

void AbstractRunner::match(KRunner::RunnerContext &)
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

AbstractRunnerPrivate::AbstractRunnerPrivate(AbstractRunner *r, const KPluginMetaData &pluginMetaData)
    : runnerDescription(pluginMetaData)
    , runner(r)
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

void AbstractRunner::matchInternal(KRunner::RunnerContext context)
{
    if (context.isValid()) { // Otherwise, we would just waste resources
        match(context);
    }
    Q_EMIT matchInternalFinished(context.query());
}

} // KRunner namespace

#include "moc_abstractrunner.cpp"
