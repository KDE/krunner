/*
 *   Copyright (C) 2017,2018 David Edmundson <davidedmundson@kde.org>
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

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QDBusMetaType>
#include <QAction>
#include <QIcon>
#include <QMutexLocker>


#include "krunner_debug.h"
#include "dbusutils_p.h"

#define IFACE_NAME "org.kde.krunner1"

DBusRunner::DBusRunner(const KService::Ptr service, QObject *parent)
    : Plasma::AbstractRunner(service, parent)
    , m_mutex(QMutex::NonRecursive)
{
    qDBusRegisterMetaType<RemoteMatch>();
    qDBusRegisterMetaType<RemoteMatches>();
    qDBusRegisterMetaType<RemoteAction>();
    qDBusRegisterMetaType<RemoteActions>();

    QString requestedServiceName = service->property(QStringLiteral("X-Plasma-DBusRunner-Service")).toString();
    m_path = service->property(QStringLiteral("X-Plasma-DBusRunner-Path")).toString();

    if (requestedServiceName.isEmpty() || m_path.isEmpty()) {
        qCWarning(KRUNNER) << "Invalid entry:" << service->name();
        return;
    }

    if (requestedServiceName.endsWith(QLatin1Char('*'))) {
        requestedServiceName.chop(1);
        //find existing matching names
        auto namesReply = QDBusConnection::sessionBus().interface()->registeredServiceNames();
        if (namesReply.isValid()) {
            const auto names = namesReply.value();
            for (const QString& serviceName : names) {
                if (serviceName.startsWith(requestedServiceName)) {
                    m_matchingServices << serviceName;
                }
            }
        }
        //and watch for changes
        connect(QDBusConnection::sessionBus().interface(), &QDBusConnectionInterface::serviceOwnerChanged,
            this, [this, requestedServiceName](const QString &serviceName, const QString &oldOwner, const QString &newOwner) {
                if (!serviceName.startsWith(requestedServiceName)) {
                    return;
                }
                if (!oldOwner.isEmpty() && !newOwner.isEmpty()) {
                    //changed owner, but service still exists. Don't need to adjust anything
                    return;
                }
                QMutexLocker lock(&m_mutex);
                if (!newOwner.isEmpty()) {
                    m_matchingServices.insert(serviceName);
                }
                if (!oldOwner.isEmpty()) {
                    m_matchingServices.remove(serviceName);
                }
            });
    } else {
        //don't check when not wildcarded, as it could be used with DBus-activation
        m_matchingServices << requestedServiceName;
    }

    connect(this, &AbstractRunner::prepare, this, &DBusRunner::requestActions);
}

DBusRunner::~DBusRunner() = default;

void DBusRunner::requestActions()
{
    clearActions();
    m_actions.clear();

    //in the multi-services case, register separate actions from each plugin in case they happen to be somehow different
    //then match together in matchForAction()

    for (const QString &service: qAsConst(m_matchingServices)) {
        auto getActionsMethod = QDBusMessage::createMethodCall(service, m_path, QStringLiteral(IFACE_NAME), QStringLiteral("Actions"));
        QDBusPendingReply<RemoteActions> reply = QDBusConnection::sessionBus().asyncCall(getActionsMethod);

        auto watcher = new QDBusPendingCallWatcher(reply);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, service]() {
            watcher->deleteLater();
            QDBusReply<RemoteActions> reply = *watcher;
            if (!reply.isValid()) {
                return;
            }
            const auto actions = reply.value();
            for(const RemoteAction &action: actions) {
                auto a = addAction(action.id, QIcon::fromTheme(action.iconName), action.text);
                a->setData(action.id);
                m_actions[service].append(a);
            }
        });
    }
}

void DBusRunner::match(Plasma::RunnerContext &context)
{
    QSet<QString> services;
    {
        QMutexLocker lock(&m_mutex);
        services = m_matchingServices;
    }
    //we scope watchers to make sure the lambda that captures context by reference definitely gets disconnected when this function ends
    QList<QSharedPointer<QDBusPendingCallWatcher>> watchers;

    for (const QString& service : qAsConst(services)) {
        auto matchMethod = QDBusMessage::createMethodCall(service, m_path, QStringLiteral(IFACE_NAME), QStringLiteral("Match"));
        matchMethod.setArguments(QList<QVariant>({context.query()}));
        QDBusPendingReply<RemoteMatches> reply = QDBusConnection::sessionBus().asyncCall(matchMethod);

        auto watcher = new QDBusPendingCallWatcher(reply);
        watchers << QSharedPointer<QDBusPendingCallWatcher>(watcher);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, service, &context, reply]() {
            if (reply.isError()) {
                qCDebug(KRUNNER) << "Error calling" << service << " :" << reply.error().name() << reply.error().message();
                return;
            }
            const auto matches = reply.value();
            for(const RemoteMatch &match: matches) {
                Plasma::QueryMatch m(this);

                m.setText(match.text);
                m.setId(match.id);
                m.setData(service);
                m.setIconName(match.iconName);
                m.setType(match.type);
                m.setRelevance(match.relevance);

                //split is essential items are as native DBus types, optional extras are in the property map (which is obviously a lot slower to parse)
                m.setUrls(QUrl::fromStringList(match.properties.value(QStringLiteral("urls")).toStringList()));
                m.setMatchCategory(match.properties.value(QStringLiteral("category")).toString());
                m.setSubtext(match.properties.value(QStringLiteral("subtext")).toString());

                context.addMatch(m);
            };
        }, Qt::DirectConnection); // process reply in the watcher's thread (aka the one running ::match  not the one owning the runner)
    }
    //we're done matching when every service replies
    for (auto w : qAsConst(watchers)) {
        w->waitForFinished();
    }
}

QList<QAction*> DBusRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)
    const QString service = match.data().toString();
    return m_actions.value(service);
}

void DBusRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context);

    QString actionId;
    QString matchId = match.id().mid(id().length() + 1); //QueryMatch::setId mangles the match ID with runnerID + '_'. This unmangles it
    QString service = match.data().toString();

    if (match.selectedAction()) {
        actionId = match.selectedAction()->data().toString();
    }

    auto runMethod = QDBusMessage::createMethodCall(service, m_path, QStringLiteral(IFACE_NAME), QStringLiteral("Run"));
    runMethod.setArguments(QList<QVariant>({matchId, actionId}));
    QDBusConnection::sessionBus().call(runMethod, QDBus::NoBlock);
}
