/*
    SPDX-FileCopyrightText: 2017, 2018 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-FileCopyrightText: 2020 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbusrunner_p.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingReply>
#include <QIcon>
#include <QMutexLocker>
#include <qobjectdefs.h>

#include "dbusutils_p.h"
#include "krunner_debug.h"

#define IFACE_NAME "org.kde.krunner1"

DBusRunner::DBusRunner(QObject *parent, const KPluginMetaData &pluginMetaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, pluginMetaData, args)
{
    qDBusRegisterMetaType<RemoteMatch>();
    qDBusRegisterMetaType<RemoteMatches>();
    qDBusRegisterMetaType<RemoteAction>();
    qDBusRegisterMetaType<RemoteActions>();
    qDBusRegisterMetaType<RemoteImage>();
    qRegisterMetaType<QMap<QString, RemoteActions>>();

    QString requestedServiceName = pluginMetaData.value(QStringLiteral("X-Plasma-DBusRunner-Service"));
    m_path = pluginMetaData.value(QStringLiteral("X-Plasma-DBusRunner-Path"));
    m_hasUniqueResults = pluginMetaData.rawData().value(QStringLiteral("X-Plasma-Runner-Unique-Results")).toBool();
    m_callLifecycleMethods = pluginMetaData.value(QStringLiteral("X-Plasma-API")) == QLatin1String("DBus2");

    if (requestedServiceName.isEmpty() || m_path.isEmpty()) {
        qCWarning(KRUNNER) << "Invalid entry:" << pluginMetaData.name();
        return;
    }

    if (requestedServiceName.endsWith(QLatin1Char('*'))) {
        requestedServiceName.chop(1);
        // find existing matching names
        auto namesReply = QDBusConnection::sessionBus().interface()->registeredServiceNames();
        if (namesReply.isValid()) {
            const auto names = namesReply.value();
            for (const QString &serviceName : names) {
                if (serviceName.startsWith(requestedServiceName)) {
                    m_matchingServices << serviceName;
                }
            }
        }
        // and watch for changes
        connect(QDBusConnection::sessionBus().interface(),
                &QDBusConnectionInterface::serviceOwnerChanged,
                this,
                [this, requestedServiceName](const QString &serviceName, const QString &oldOwner, const QString &newOwner) {
                    if (!serviceName.startsWith(requestedServiceName)) {
                        return;
                    }
                    if (!oldOwner.isEmpty() && !newOwner.isEmpty()) {
                        // changed owner, but service still exists. Don't need to adjust anything
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
        // don't check when not wildcarded, as it could be used with DBus-activation
        m_matchingServices << requestedServiceName;
    }

    m_requestActionsOnce = pluginMetaData.rawData().value(QStringLiteral("X-Plasma-Request-Actions-Once")).toVariant().toBool();
    connect(this, &AbstractRunner::teardown, this, &DBusRunner::teardown);

    // Load the runner syntaxes
    const QStringList syntaxes = pluginMetaData.rawData().value(QStringLiteral("X-Plasma-Runner-Syntaxes")).toVariant().toStringList();
    const QStringList syntaxDescriptions = pluginMetaData.rawData().value(QStringLiteral("X-Plasma-Runner-Syntax-Descriptions")).toVariant().toStringList();
    const int descriptionCount = syntaxDescriptions.count();
    for (int i = 0; i < syntaxes.count(); ++i) {
        const QString &query = syntaxes.at(i);
        const QString description = i < descriptionCount ? syntaxDescriptions.at(i) : QString();
        addSyntax(Plasma::RunnerSyntax(query, description));
    }
}

DBusRunner::~DBusRunner() = default;

void DBusRunner::reloadConfiguration()
{
    // If we have already loaded a config, but the runner is told to reload it's config
    if (m_callLifecycleMethods) {
        suspendMatching(true);
        requestConfig();
    }
}

void DBusRunner::createQActionsFromRemoteActions(const QMap<QString, RemoteActions> &remoteActions)
{
    for (auto it = remoteActions.begin(), end = remoteActions.end(); it != end; it++) {
        const QString service = it.key();
        const RemoteActions actions = it.value();
        auto &serviceActions = m_actions[service];
        qDeleteAll(serviceActions);
        serviceActions.clear();
        for (const RemoteAction &action : actions) {
            auto a = new QAction(QIcon::fromTheme(action.iconName), action.text, this);
            a->setData(action.id);
            serviceActions.append(a);
        }
    }
}

void DBusRunner::teardown()
{
    if (m_matchWasCalled) {
        for (const QString &service : std::as_const(m_matchingServices)) {
            auto method = QDBusMessage::createMethodCall(service, m_path, QStringLiteral(IFACE_NAME), QStringLiteral("Teardown"));
            QDBusConnection::sessionBus().asyncCall(method);
        }
    }
    m_actionsForSessionRequested = false;
    m_matchWasCalled = false;
}

QMap<QString, RemoteActions> DBusRunner::requestActions()
{
    // in the multi-services case, register separate actions from each plugin in case they happen to be somehow different
    // then match together in matchForAction()
    QMap<QString, RemoteActions> returnedActions;
    for (const QString &service : std::as_const(m_matchingServices)) {
        // if we only want to request the actions once and have done so we want to skip the service
        // but in case it got newly loaded we need to request the actions, BUG: 435350
        if (m_requestActionsOnce) {
            if (m_requestedActionServices.contains(service)) {
                continue;
            } else {
                m_requestedActionServices << service;
            }
        }

        auto getActionsMethod = QDBusMessage::createMethodCall(service, m_path, QStringLiteral(IFACE_NAME), QStringLiteral("Actions"));
        QDBusPendingReply<RemoteActions> reply = QDBusConnection::sessionBus().call(getActionsMethod);
        if (!reply.isValid()) {
            qCDebug(KRUNNER) << "Error requesting actions; calling" << service << " :" << reply.error().name() << reply.error().message();
        } else {
            returnedActions.insert(service, reply.value());
        }
    }
    return returnedActions;
}

void DBusRunner::requestConfig()
{
    const QString service = *m_matchingServices.constBegin();
    auto getConfigMethod = QDBusMessage::createMethodCall(service, m_path, QStringLiteral(IFACE_NAME), QStringLiteral("Config"));
    QDBusPendingReply<QVariantMap> reply = QDBusConnection::sessionBus().asyncCall(getConfigMethod);

    auto watcher = new QDBusPendingCallWatcher(reply);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, service]() {
        watcher->deleteLater();
        QDBusReply<QVariantMap> reply = *watcher;
        if (!reply.isValid()) {
            suspendMatching(false);
            qCDebug(KRUNNER) << "Error requesting config; calling" << service << " :" << reply.error().name() << reply.error().message();
            return;
        }
        const QVariantMap config = reply.value();
        for (auto it = config.cbegin(), end = config.cend(); it != end; ++it) {
            if (it.key() == QLatin1String("MatchRegex")) {
                QRegularExpression regex(it.value().toString());
                setMatchRegex(regex);
            } else if (it.key() == QLatin1String("MinLetterCount")) {
                setMinLetterCount(it.value().toInt());
            } else if (it.key() == QLatin1String("TriggerWords")) {
                setTriggerWords(it.value().toStringList());
            } else if (it.key() == QLatin1String("Actions")) {
                const auto remoteActions = it.value().value<RemoteActions>();
                createQActionsFromRemoteActions(QMap<QString, RemoteActions>{{service, remoteActions}});
                m_actionsOnceRequested = true;
                m_actionsForSessionRequested = true;
            }
        }
        suspendMatching(false);
    });
}

void DBusRunner::match(Plasma::RunnerContext &context)
{
    QSet<QString> services;
    {
        QMutexLocker lock(&m_mutex);
        services = m_matchingServices;
        m_matchWasCalled = true;

        // Request the actions
        if ((m_requestActionsOnce && !m_actionsOnceRequested) // We only want to fetch the actions once but haven't done so yet
            || (!m_actionsForSessionRequested)) { // We want to fetch the actions for each match session
            m_actionsOnceRequested = true;
            m_actionsForSessionRequested = true;
            auto actions = requestActions();
            const auto actionsArg = QArgument<QMap<QString, RemoteActions>>("QMap<QString, RemoteActions>", actions);
            QMetaObject::invokeMethod(this, "createQActionsFromRemoteActions", actionsArg);
        }
    }
    // we scope watchers to make sure the lambda that captures context by reference definitely gets disconnected when this function ends
    QList<QSharedPointer<QDBusPendingCallWatcher>> watchers;

    for (const QString &service : std::as_const(services)) {
        auto matchMethod = QDBusMessage::createMethodCall(service, m_path, QStringLiteral(IFACE_NAME), QStringLiteral("Match"));
        matchMethod.setArguments(QList<QVariant>({context.query()}));
        QDBusPendingReply<RemoteMatches> reply = QDBusConnection::sessionBus().asyncCall(matchMethod);

        auto watcher = new QDBusPendingCallWatcher(reply);
        watchers << QSharedPointer<QDBusPendingCallWatcher>(watcher);
        connect(
            watcher,
            &QDBusPendingCallWatcher::finished,
            this,
            [this, service, &context, reply]() {
                if (reply.isError()) {
                    qCDebug(KRUNNER) << "Error requesting matches; calling" << service << " :" << reply.error().name() << reply.error().message();
                    return;
                }
                const auto matches = reply.value();
                for (const RemoteMatch &match : matches) {
                    Plasma::QueryMatch m(this);

                    m.setText(match.text);
                    m.setIconName(match.iconName);
                    m.setType(match.type);
                    m.setRelevance(match.relevance);

                    // split is essential items are as native DBus types, optional extras are in the property map (which is obviously a lot slower to parse)
                    m.setUrls(QUrl::fromStringList(match.properties.value(QStringLiteral("urls")).toStringList()));
                    m.setMatchCategory(match.properties.value(QStringLiteral("category")).toString());
                    m.setSubtext(match.properties.value(QStringLiteral("subtext")).toString());
                    if (match.properties.contains(QStringLiteral("actions"))) {
                        m.setData(QVariantList({service, match.properties.value(QStringLiteral("actions"))}));
                    } else {
                        m.setData(QVariantList({service}));
                    }
                    m.setId(match.id);

                    const QVariant iconData = match.properties.value(QStringLiteral("icon-data"));
                    if (iconData.isValid()) {
                        const auto iconDataArgument = iconData.value<QDBusArgument>();
                        if (iconDataArgument.currentType() == QDBusArgument::StructureType
                            && iconDataArgument.currentSignature() == QLatin1String("(iiibiiay)")) {
                            const RemoteImage remoteImage = qdbus_cast<RemoteImage>(iconDataArgument);
                            const QImage decodedImage = decodeImage(remoteImage);
                            if (!decodedImage.isNull()) {
                                const QPixmap pix = QPixmap::fromImage(decodedImage);
                                QIcon icon(pix);
                                m.setIcon(icon);
                                // iconName normally takes precedence
                                m.setIconName(QString());
                            }
                        } else {
                            qCWarning(KRUNNER) << "Invalid signature of icon-data property:" << iconDataArgument.currentSignature();
                        }
                    }

                    context.addMatch(m);
                };
            },
            Qt::DirectConnection); // process reply in the watcher's thread (aka the one running ::match  not the one owning the runner)
    }
    // we're done matching when every service replies
    for (auto w : std::as_const(watchers)) {
        w->waitForFinished();
    }
}

QList<QAction *> DBusRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    const QVariantList data = match.data().toList();
    if (data.count() > 1) {
        const QStringList actionIds = data.at(1).toStringList();
        const QList<QAction *> actionList = m_actions.value(data.constFirst().toString());
        QList<QAction *> requestedActions;
        for (QAction *action : actionList) {
            if (actionIds.contains(action->data().toString())) {
                requestedActions << action;
            }
        }
        return requestedActions;
    } else {
        return m_actions.value(data.constFirst().toString());
    }
}

void DBusRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context);

    QString actionId;
    QString matchId;
    if (m_hasUniqueResults) {
        matchId = match.id();
    } else {
        matchId = match.id().mid(id().length() + 1); // QueryMatch::setId mangles the match ID with runnerID + '_'. This unmangles it
    }
    const QString service = match.data().toList().constFirst().toString();

    if (match.selectedAction()) {
        actionId = match.selectedAction()->data().toString();
    }

    auto runMethod = QDBusMessage::createMethodCall(service, m_path, QStringLiteral(IFACE_NAME), QStringLiteral("Run"));
    runMethod.setArguments(QList<QVariant>({matchId, actionId}));
    QDBusConnection::sessionBus().call(runMethod, QDBus::NoBlock);
}

QImage DBusRunner::decodeImage(const RemoteImage &remoteImage)
{
    auto copyLineRGB32 = [](QRgb *dst, const char *src, int width) {
        const char *end = src + width * 3;
        for (; src != end; ++dst, src += 3) {
            *dst = qRgb(src[0], src[1], src[2]);
        }
    };

    auto copyLineARGB32 = [](QRgb *dst, const char *src, int width) {
        const char *end = src + width * 4;
        for (; src != end; ++dst, src += 4) {
            *dst = qRgba(src[0], src[1], src[2], src[3]);
        }
    };

    if (remoteImage.width <= 0 || remoteImage.width >= 2048 || remoteImage.height <= 0 || remoteImage.height >= 2048 || remoteImage.rowStride <= 0) {
        qCWarning(KRUNNER) << "Invalid image metadata (width:" << remoteImage.width << "height:" << remoteImage.height << "rowStride:" << remoteImage.rowStride
                           << ")";
        return QImage();
    }

    QImage::Format format = QImage::Format_Invalid;
    void (*copyFn)(QRgb *, const char *, int) = nullptr;
    if (remoteImage.bitsPerSample == 8) {
        if (remoteImage.channels == 4) {
            format = QImage::Format_ARGB32;
            copyFn = copyLineARGB32;
        } else if (remoteImage.channels == 3) {
            format = QImage::Format_RGB32;
            copyFn = copyLineRGB32;
        }
    }
    if (format == QImage::Format_Invalid) {
        qCWarning(KRUNNER) << "Unsupported image format (hasAlpha:" << remoteImage.hasAlpha << "bitsPerSample:" << remoteImage.bitsPerSample
                           << "channels:" << remoteImage.channels << ")";
        return QImage();
    }

    QImage image(remoteImage.width, remoteImage.height, format);
    const QByteArray pixels = remoteImage.data;
    const char *ptr = pixels.data();
    const char *end = ptr + pixels.length();
    for (int y = 0; y < remoteImage.height; ++y, ptr += remoteImage.rowStride) {
        if (Q_UNLIKELY(ptr + remoteImage.channels * remoteImage.width > end)) {
            qCWarning(KRUNNER) << "Image data is incomplete. y:" << y << "height:" << remoteImage.height;
            break;
        }
        copyFn(reinterpret_cast<QRgb *>(image.scanLine(y)), ptr, remoteImage.width);
    }

    return image;
}
