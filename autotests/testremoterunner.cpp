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

#include <QCoreApplication>
#include <QDBusConnection>

#include <iostream>

#include "testremoterunner.h"
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
    Q_ASSERT(app.arguments().count() == 2);
    TestRemoteRunner r(app.arguments()[1]);
    app.exec();
}
