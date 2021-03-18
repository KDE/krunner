/*
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testremoterunner.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QImage>

#include <iostream>

#include "krunner1adaptor.h"

// Test DBus runner, if the search term contains "foo" it returns a match, otherwise nothing
// Run prints a line to stdout

TestRemoteRunner::TestRemoteRunner(const QString &serviceName, bool showLifecycleMethodCalls)
{
    new Krunner1Adaptor(this);
    qDBusRegisterMetaType<RemoteMatch>();
    qDBusRegisterMetaType<RemoteMatches>();
    qDBusRegisterMetaType<RemoteAction>();
    qDBusRegisterMetaType<RemoteActions>();
    qDBusRegisterMetaType<RemoteImage>();
    Q_ASSERT(QDBusConnection::sessionBus().registerService(serviceName));
    Q_ASSERT(QDBusConnection::sessionBus().registerObject(QStringLiteral("/dave"), this));
    m_showLifecycleMethodCalls = showLifecycleMethodCalls;
}

static RemoteImage serializeImage(const QImage &image)
{
    QImage convertedImage = image.convertToFormat(QImage::Format_RGBA8888);
    RemoteImage remoteImage;
    remoteImage.width = convertedImage.width();
    remoteImage.height = convertedImage.height();
    remoteImage.rowStride = convertedImage.bytesPerLine();
    remoteImage.hasAlpha = true, remoteImage.bitsPerSample = 8;
    remoteImage.channels = 4, remoteImage.data = QByteArray(reinterpret_cast<const char *>(convertedImage.constBits()), convertedImage.sizeInBytes());
    return remoteImage;
}

RemoteMatches TestRemoteRunner::Match(const QString &searchTerm)
{
    RemoteMatches ms;
    std::cout << "Matching:" << qPrintable(searchTerm) << std::endl;
    if (searchTerm == QLatin1String("fooCostomIcon")) {
        RemoteMatch m;
        m.id = QStringLiteral("id2");
        m.text = QStringLiteral("Match 1");
        m.type = Plasma::QueryMatch::ExactMatch;
        m.relevance = 0.8;
        QImage icon(10, 10, QImage::Format_RGBA8888);
        icon.fill(Qt::blue);
        m.properties[QStringLiteral("icon-data")] = QVariant::fromValue(serializeImage(icon));

        ms << m;
    } else if (searchTerm.contains(QLatin1String("foo"))) {
        RemoteMatch m;
        m.id = QStringLiteral("id1");
        m.text = QStringLiteral("Match 1");
        m.iconName = QStringLiteral("icon1");
        m.type = Plasma::QueryMatch::ExactMatch;
        m.relevance = 0.8;
        m.properties[QStringLiteral("actions")] = QStringList(QStringLiteral("action1"));
        ms << m;
    }
    return ms;
}

RemoteActions TestRemoteRunner::Actions()
{
    RemoteAction action;
    action.id = QStringLiteral("action1");
    action.text = QStringLiteral("Action 1");
    action.iconName = QStringLiteral("document-browser");

    RemoteAction action2;
    action2.id = QStringLiteral("action2");
    action2.text = QStringLiteral("Action 2");
    action2.iconName = QStringLiteral("document-browser");

    return RemoteActions({action, action2});
}

void TestRemoteRunner::Run(const QString &id, const QString &actionId)
{
    std::cout << "Running:" << qPrintable(id) << ":" << qPrintable(actionId) << std::endl;
    std::cout.flush();
}

void TestRemoteRunner::Teardown()
{
    if (m_showLifecycleMethodCalls) {
        std::cout << "Teardown" << std::endl;
        std::cout.flush();
    }
}

QVariantMap TestRemoteRunner::Config()
{
    if (m_showLifecycleMethodCalls) {
        std::cout << "Config" << std::endl;
        std::cout.flush();
    }

    return {
        {"MatchRegex", "^fo"},
        {"MinLetterCount", 4},
    };
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto arguments = app.arguments();
    Q_ASSERT(arguments.count() >= 2);
    TestRemoteRunner r(arguments[1], arguments.count() == 3);
    app.exec();
}
