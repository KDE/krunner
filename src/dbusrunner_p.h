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
    explicit DBusRunner(QObject *parent, const KPluginMetaData &pluginMetaData, const QVariantList &args);
    ~DBusRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void reloadConfiguration() override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;
    QList<QAction *> actionsForMatch(const Plasma::QueryMatch &match) override;

public Q_SLOTS:
    void teardown();
    // This method should only be called from the main thread!
    void createQActionsFromRemoteActions(const QMap<QString, RemoteActions> &remoteActions);

private:
    // Returns RemoteActions with service name as key
    QMap<QString, RemoteActions> requestActions();
    void requestConfig();
    static QImage decodeImage(const RemoteImage &remoteImage);
    QMutex m_mutex; // needed round any variable also accessed from Match
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
};
