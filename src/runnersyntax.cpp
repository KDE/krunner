/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnersyntax.h"

#include <KLocalizedString>

namespace Plasma
{

class RunnerSyntaxPrivate
{
public:
    RunnerSyntaxPrivate(const QString &s, const QString &d)
        : description(d)
    {
        exampleQueries.append(s);
    }

    QStringList exampleQueries;
    QString description;
    QString termDescription;
};

RunnerSyntax::RunnerSyntax(const QString &exampleQuery, const QString &description)
    : d(new RunnerSyntaxPrivate(exampleQuery, description))
{
}

RunnerSyntax::RunnerSyntax(const RunnerSyntax &other)
    : d(new RunnerSyntaxPrivate(*other.d))
{
}

RunnerSyntax::~RunnerSyntax()
{
    delete d;
}

RunnerSyntax &RunnerSyntax::operator=(const RunnerSyntax &rhs)
{
    *d = *rhs.d;
    return *this;
}

void RunnerSyntax::addExampleQuery(const QString &exampleQuery)
{
    d->exampleQueries.append(exampleQuery);
}

QStringList RunnerSyntax::exampleQueries() const
{
    return d->exampleQueries;
}

QStringList RunnerSyntax::exampleQueriesWithTermDescription() const
{
    QStringList queries;
    const QString termDesc(QLatin1Char('<') + searchTermDescription() + QLatin1Char('>'));
    for (QString query : qAsConst(d->exampleQueries)) {
        queries << query.replace(QStringLiteral(":q:"), termDesc);
    }

    return queries;
}

void RunnerSyntax::setDescription(const QString &description)
{
    d->description = description;
}

QString RunnerSyntax::description() const
{
    QString description = d->description;
    description.replace(QLatin1String(":q:"), QLatin1Char('<') + searchTermDescription() + QLatin1Char('>'));
    return description;
}

void RunnerSyntax::setSearchTermDescription(const QString &description)
{
    d->termDescription = description;
}

QString RunnerSyntax::searchTermDescription() const
{
    if (d->termDescription.isEmpty()) {
        return i18n("search term");
    }

    return d->termDescription;
}

} // Plasma namespace

