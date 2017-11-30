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

#include "dbusrunner_p.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusError>
#include <QDBusPendingReply>
#include <QAction>
#include <QIcon>
#include <QVariantMap>

#include <KPluginMetaData>

#include "krunner_debug.h"
#include "krunner_iface.h"

DBusRunner::DBusRunner(const KService::Ptr service, QObject *parent)
    : Plasma::AbstractRunner(service, parent)
{
    qDBusRegisterMetaType<RemoteMatch>();
    qDBusRegisterMetaType<RemoteMatches>();
    qDBusRegisterMetaType<RemoteAction>();
    qDBusRegisterMetaType<RemoteActions>();

    QString serviceName = service->property(QStringLiteral("X-Plasma-DBusRunner-Service")).toString();
    QString path = service->property(QStringLiteral("X-Plasma-DBusRunner-Path")).toString();

    if (serviceName.isEmpty() || path.isEmpty()) {
        qCWarning(KRUNNER) << "Invalid entry:" << service->name();
        return;
    }

    m_interface = new OrgKdeKrunner1Interface(serviceName, path, QDBusConnection::sessionBus(), this);

    connect(this, &AbstractRunner::prepare, this, &DBusRunner::requestActions);
}

DBusRunner::~DBusRunner() = default;

void DBusRunner::requestActions()
{
    auto reply = m_interface->Actions();
    auto watcher = new QDBusPendingCallWatcher(reply);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        watcher->deleteLater();
        clearActions();
        QDBusReply<RemoteActions> reply = *watcher;
        if (!reply.isValid()) {
            return;
        }
        for(const RemoteAction &action: reply.value()) {
            auto a = addAction(action.id, QIcon::fromTheme(action.iconName), action.text);
            a->setData(action.id);
        }
    });
}

void DBusRunner::match(Plasma::RunnerContext &context)
{
    if (!m_interface) {
        return;
    }

    auto reply = m_interface->Match(context.query());
    reply.waitForFinished(); //AbstractRunner::match is called in a new thread, may as well block
    if (reply.isError()) {
        qCDebug(KRUNNER) << "Error calling" << m_interface->service() << " :" << reply.error().name() << reply.error().message();
        return;
    }
    for(const RemoteMatch &match: reply.value()) {
        Plasma::QueryMatch m(this);

        m.setText(match.text);
        m.setData(match.id);
        m.setIconName(match.iconName);
        m.setType(match.type);
        m.setRelevance(match.relevance);

        //split is essential items are as native DBus types, optional extras are in the property map (which is obviously a lot slower to parse)
        m.setUrls(QUrl::fromStringList(match.properties.value(QStringLiteral("urls")).toStringList()));
        m.setMatchCategory(match.properties.value(QStringLiteral("category")).toString());
        m.setSubtext(match.properties.value(QStringLiteral("subtext")).toString());

        context.addMatch(m);
    }
}

QList<QAction*> DBusRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)
    return actions().values();
}

void DBusRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context);
    if (!m_interface) {
        return;
    }

    QString actionId;
    QString matchId = match.data().toString();

    if (match.selectedAction()) {
        actionId = match.selectedAction()->data().toString();
    }

    m_interface->Run(matchId, actionId); //don't wait for reply before returning to process
}
