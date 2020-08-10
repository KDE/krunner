/*
    SPDX-FileCopyrightText: 2014 Vishesh Handa <vhanda@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>

#include "runnermanager.h"

using namespace Plasma;

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setQuitLockEnabled(false);

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("query"), QStringLiteral("words to query"));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("r") << QStringLiteral("runner"),
                                        QStringLiteral("Name of the runner"),
                                        QStringLiteral("runnerName")));

    parser.addHelpOption();
    parser.process(app);

    QString query = parser.positionalArguments().join(QLatin1Char(' '));
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
        for (const auto& match : qAsConst(matches)) {
            qDebug() << match.matchCategory() << match.text();
        }
        app.quit();
    });

    manager.launchQuery(query, runnerName);
    return app.exec();
}
