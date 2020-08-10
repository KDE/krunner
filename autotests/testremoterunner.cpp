/*
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "testremoterunner.h"

#include <QCoreApplication>
#include <QDBusConnection>

#include <iostream>

#include "krunner1adaptor.h"

//Test DBus runner, if the search term contains "foo" it returns a match, otherwise nothing
//Run prints a line to stdout

TestRemoteRunner::TestRemoteRunner(const QString &serviceName)
{
    new Krunner1Adaptor(this);
    qDBusRegisterMetaType<RemoteMatch>();
    qDBusRegisterMetaType<RemoteMatches>();
    qDBusRegisterMetaType<RemoteAction>();
    qDBusRegisterMetaType<RemoteActions>();
    QDBusConnection::sessionBus().registerService(serviceName);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/dave"), this);
}

RemoteMatches TestRemoteRunner::Match(const QString& searchTerm)
{
    RemoteMatches ms;
    if (searchTerm.contains(QLatin1String("foo"))) {
        RemoteMatch m;
        m.id = QStringLiteral("id1");
        m.text = QStringLiteral("Match 1");
        m.iconName = QStringLiteral("icon1");
        m.type = Plasma::QueryMatch::ExactMatch;
        m.relevance = 0.8;
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

    return RemoteActions({action});
}

void TestRemoteRunner::Run(const QString &id, const QString &actionId)
{
    std::cout << "Running:" << qPrintable(id) << ":" << qPrintable(actionId) << std::endl;
    std::cout.flush();
}

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);
    const auto arguments = app.arguments();
    Q_ASSERT(arguments.count() == 2);
    TestRemoteRunner r(arguments[1]);
    app.exec();
}
