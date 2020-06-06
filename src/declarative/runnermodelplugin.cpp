/*
 *   Copyright 2011 by Marco Martin <mart@kde.org>

 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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


