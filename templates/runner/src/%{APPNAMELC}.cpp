/*
    SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "%{APPNAMELC}.h"

// KF
#include <KLocalizedString>

%{APPNAME}::%{APPNAME}(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
{
    setObjectName(QStringLiteral("%{APPNAME}"));
}

%{APPNAME}::~%{APPNAME}()
{
}


void %{APPNAME}::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();
    if (term.length() < 3) {
        return;
    }

    // TODO
}

void %{APPNAME}::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)
    Q_UNUSED(match)

    // TODO
}

K_EXPORT_PLASMA_RUNNER_WITH_JSON(%{APPNAME}, "plasma-runner-%{APPNAMELC}.json")

// needed for the QObject subclass declared as part of K_EXPORT_PLASMA_RUNNER_WITH_JSON
#include "%{APPNAMELC}.moc"
