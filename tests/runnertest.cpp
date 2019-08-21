/*
 *   Copyright 2014 Vishesh Handa <vhanda@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
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
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>

#include "runnermanager.h"

using namespace Plasma;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setQuitLockEnabled(false);

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("query"), QStringLiteral("words to query"));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("r") << QStringLiteral("runner"),
                                        QStringLiteral("Name of the runner"),
                                        QStringLiteral("runnerName")));

    parser.addHelpOption();
    parser.process(app);

    QString query = parser.positionalArguments().join(QStringLiteral(" "));
    if (query.isEmpty()) {
        parser.showHelp(1);
    }
    QString runnerName = parser.value(QStringLiteral("runner"));

    RunnerManager manager;

    QList<Plasma::QueryMatch> matches;
    QObject::connect(&manager, &RunnerManager::matchesChanged, [&](const QList<Plasma::QueryMatch>& list) {
        matches = list;
    });
    QObject::connect(&manager, &RunnerManager::queryFinished, [&]() {
        qDebug() << "Found matches:";
        for (const auto& match : matches) {
            qDebug() << match.matchCategory() << match.text();
        }
        app.quit();
    });

    manager.launchQuery(query, runnerName);
    return app.exec();
}
