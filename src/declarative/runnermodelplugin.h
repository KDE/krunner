/*
    SPDX-FileCopyrightText: 2011 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2011 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef RUNNERMODELPLUGIN_H
#define RUNNERMODELPLUGIN_H

#include <QQmlExtensionPlugin>

/*
 * FIXME: This plugin is deprecated, it should be removed for plasma2
 */
class RunnerModelPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override;
};

#endif
