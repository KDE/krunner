/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2020-2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "abstractrunner.h"
#include "abstractrunner_p.h"

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
AbstractRunner::AbstractRunner(QObject *parent, const KPluginMetaData &pluginMetaData)
    : QObject(parent)
    , d(new AbstractRunnerPrivate(this, pluginMetaData))
{
    setObjectName(pluginMetaData.pluginId()); // Only for debugging purposes
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

void AbstractRunner::setSyntaxes(const QList<RunnerSyntax> &syntaxes)
{
    d->syntaxes = syntaxes;
}

QList<RunnerSyntax> AbstractRunner::syntaxes() const
{
    return d->syntaxes;
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

QString AbstractRunner::name() const
{
    return d->runnerDescription.name();
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

void AbstractRunner::matchInternal(KRunner::RunnerContext context)
{
    if (context.isValid()) { // Otherwise, we would just waste resources
        match(context);
    }
    Q_EMIT matchInternalFinished(context.query());
}

} // KRunner namespace

#include "moc_abstractrunner.cpp"
