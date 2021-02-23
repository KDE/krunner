/*
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KRunner/AbstractRunner>

#include "dbusutils_p.h"
#include <QHash>
#include <QImage>
#include <QList>
#include <QMutex>
#include <QSet>

class DBusRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    explicit DBusRunner(const KPluginMetaData &pluginMetaData, QObject *parent = nullptr);
    ~DBusRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;
    QList<QAction *> actionsForMatch(const Plasma::QueryMatch &match) override;

private:
    void requestActions();
    void setActions(const RemoteActions &remoteActions);
    static QImage decodeImage(const RemoteImage &remoteImage);
    QMutex m_mutex; // needed round any variable also accessed from Match
    QString m_path;
    QSet<QString> m_matchingServices;
    QHash<QString, QList<QAction *>> m_actions;
};
