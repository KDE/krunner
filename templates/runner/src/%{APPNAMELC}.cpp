/*
    SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "%{APPNAMELC}.h"

// KF
#include <KLocalizedString>

%{APPNAME}::%{APPNAME}(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::AbstractRunner(parent, data, args)
{
    setObjectName(QStringLiteral("%{APPNAME}"));
    // Disallow short queries
    setMinLetterCount(3);
}

%{APPNAME}::~%{APPNAME}()
{
}


void %{APPNAME}::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();
    // TODO
}

void %{APPNAME}::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)
    Q_UNUSED(match)

    // TODO
}

K_PLUGIN_CLASS_WITH_JSON(%{APPNAME}, "%{APPNAMELC}.json")

// needed for the QObject subclass declared as part of K_PLUGIN_CLASS_WITH_JSON
#include "%{APPNAMELC}.moc"
