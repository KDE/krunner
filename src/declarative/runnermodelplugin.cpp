/*
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnermodelplugin.h"

#include <QtQml>

#include "krunner_debug.h"

#include <KRunner/QueryMatch>

#include "runnermodel.h"

void RunnerModelPlugin::registerTypes(const char *uri)
{
    qCWarning(KRUNNER) << "Using deprecated import org.kde.runnermodel, please port to org.kde.plasma.core";
    Q_ASSERT(QLatin1String(uri) == QLatin1String("org.kde.runnermodel"));
    qmlRegisterType<RunnerModel>(uri, 2, 0, "RunnerModel");
    // to port this to Qt5.15-non-deprecated variant
    // qmlRegisterInterface<Plasma::QueryMatch>(uri, 1);
    // QueryMatch would need to get a Q_GAGDET added just for this
    // As this plugin is deprecated, this is not worth it,
    // so we just disable the deprecation warning
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
    QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
    qmlRegisterInterface<Plasma::QueryMatch>("QueryMatch");
    QT_WARNING_POP
    qRegisterMetaType<Plasma::QueryMatch *>("QueryMatch");
}
