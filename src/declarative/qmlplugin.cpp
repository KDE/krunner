// SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include <QQmlExtensionPlugin>
#include <qqml.h>

#include "resultsmodel.h"

class KRunnerQuickPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char *uri) override
    {
        qmlRegisterType<KRunner::ResultsModel>(uri, 1, 0, "ResultsModel");
    }
};

#include "qmlplugin.moc"
