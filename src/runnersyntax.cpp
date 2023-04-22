/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnersyntax.h"

#include <KLocalizedString>

namespace KRunner
{
class RunnerSyntaxPrivate
{
public:
    RunnerSyntaxPrivate(const QStringList &_exampleQueries, const QString &_description)
        : description(_description)
    {
        for (const QString &query : _exampleQueries) {
            addExampleQuery(query);
        }
    }

    void addExampleQuery(const QString &s)
    {
        const QString termDesc(QLatin1Char('<') + termDescription + QLatin1Char('>'));
        exampleQueries.append(QString(s).replace(QStringLiteral(":q:"), termDesc));
    }

    QStringList exampleQueries;
    QString description;
    QString termDescription = i18n("search term");
};

RunnerSyntax::RunnerSyntax(const QStringList &exampleQueries, const QString &description)
    : d(new RunnerSyntaxPrivate(exampleQueries, description))
{
}

RunnerSyntax::RunnerSyntax(const RunnerSyntax &other)
    : d(new RunnerSyntaxPrivate(*other.d))
{
}

RunnerSyntax::~RunnerSyntax() = default;

RunnerSyntax &RunnerSyntax::operator=(const RunnerSyntax &rhs)
{
    *d = *rhs.d;
    return *this;
}

void RunnerSyntax::addExampleQuery(const QString &exampleQuery)
{
    d->addExampleQuery(exampleQuery);
}

QStringList RunnerSyntax::exampleQueries() const
{
    return d->exampleQueries;
}

QString RunnerSyntax::description() const
{
    return d->description;
}

} // KRunner namespace
