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
        , termDescription(i18n("search term"))
    {
        addExampleQuery(s);
    }

    void addExampleQuery(const QString &s)
    {
        const QString termDesc(QLatin1Char('<') + termDescription + QLatin1Char('>'));
        exampleQueries.append(QString(s).replace(QStringLiteral(":q:"), termDesc));
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

} // Plasma namespace
