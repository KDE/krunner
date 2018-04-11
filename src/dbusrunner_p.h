/*
 *   Copyright (C) 2017 David Edmundson <davidedmundson@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <KRunner/AbstractRunner>

#include "dbusutils_p.h"
class OrgKdeKrunner1Interface;
#include <QHash>
#include <QList>
#include <QSet>
#include <QMutex>

class DBusRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    explicit DBusRunner(const KService::Ptr service, QObject *parent = nullptr);
    ~DBusRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;
    QList<QAction*> actionsForMatch(const Plasma::QueryMatch &match) override;

private:
    void requestActions();
    void setActions(const RemoteActions &remoteActions);
    QMutex m_mutex; //needed round any variable also accessed from Match
    QString m_path;
    QSet<QString> m_matchingServices;
    QHash<QString, QList<QAction*> > m_actions;
};
