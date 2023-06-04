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
#include <QSet>

class DBusRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    explicit DBusRunner(QObject *parent, const KPluginMetaData &pluginMetaData);
    ~DBusRunner() override;

    void match(KRunner::RunnerContext &context) override;
    void reloadConfiguration() override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;

private:
    void teardown();
    void createQActionsFromRemoteActions(const QMap<QString, RemoteActions> &remoteActions);

    // Returns RemoteActions with service name as key
    QMap<QString, RemoteActions> requestActions();
    void requestConfig();
    static QImage decodeImage(const RemoteImage &remoteImage);
    QString m_path;
    QSet<QString> m_matchingServices;
    QHash<QString, QList<QAction *>> m_actions;
    bool m_hasUniqueResults = false;
    bool m_requestActionsOnce = false;
    bool m_actionsOnceRequested = false;
    bool m_actionsForSessionRequested = false;
    bool m_matchWasCalled = false;
    bool m_callLifecycleMethods = false;
    QSet<QString> m_requestedActionServices;
    QObject *parentForActions;
};
