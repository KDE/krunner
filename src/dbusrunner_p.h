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
    explicit DBusRunner(QObject *parent, const KPluginMetaData &data);

    void match(KRunner::RunnerContext &context) override;
    void reloadConfiguration() override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;

private:
    void teardown();

    // Returns RemoteActions with service name as key
    void requestActions();
    void requestConfig();
    static QImage decodeImage(const RemoteImage &remoteImage);
    QSet<QString> m_matchingServices;
    QHash<QString, QList<KRunner::Action>> m_actions;
    const QString m_path;
    const bool m_hasUniqueResults;
    const bool m_requestActionsOnce;
    bool m_actionsOnceRequested = false;
    bool m_actionsForSessionRequested = false;
    bool m_matchWasCalled = false;
    bool m_callLifecycleMethods = false;
    const QString m_ifaceName;
    QSet<QString> m_requestedActionServices;
    const QString ifaceName = QStringLiteral("org.kde.krunner1");
};
