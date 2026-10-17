/*
    SPDX-FileCopyrightText: 2022 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KConfigGroup>
#include <KDesktopFile>
#include <KPluginMetaData>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

template<class T = QString>
inline void copyIfExists(const KConfigGroup &grp, QJsonObject &obj, const char *key, const T &t = QString())
{
    copyAndRenameIfExists(grp, obj, key, key, t);
}

template<class T>
inline void copyAndRenameIfExists(const KConfigGroup &grp, QJsonObject &obj, const char *oldKey, const char *key, const T &t)
{
    if (grp.hasKey(oldKey)) {
        obj.insert(QLatin1String(key), grp.readEntry(oldKey, t));
    }
}
inline KPluginMetaData parseMetaDataFromDesktopFile(const QString &fileName)
{
    KDesktopFile file(fileName);
    const KConfigGroup grp = file.desktopGroup();

    QJsonObject kplugin;
    copyIfExists(grp, kplugin, "Name");
    copyIfExists(grp, kplugin, "Icon");
    copyAndRenameIfExists(grp, kplugin, "X-KDE-PluginInfo-Name", "Id", QString());
    copyAndRenameIfExists(grp, kplugin, "Comment", "Description", QString());
    copyAndRenameIfExists(grp, kplugin, "X-KDE-PluginInfo-EnabledByDefault", "EnabledByDefault", false);
    copyAndRenameIfExists(grp, kplugin, "X-KDE-PluginInfo-Version", "Version", QString());
    copyAndRenameIfExists(grp, kplugin, "X-KDE-PluginInfo-License", "License", QString());
    copyAndRenameIfExists(grp, kplugin, "X-KDE-PluginInfo-Copyright", "Copyright", QString());
    copyAndRenameIfExists(grp, kplugin, "X-KDE-PluginInfo-Website", "Website", QString());

    QJsonObject author;
    author.insert(QLatin1String("Name"), grp.readEntry("X-KDE-PluginInfo-Author"));
    author.insert(QLatin1String("Email"), grp.readEntry("X-KDE-PluginInfo-Email"));
    author.insert(QLatin1String("Website"), grp.readEntry("X-KDE-PluginInfo-Website"));
    if (!author.value(QLatin1String("Name")).toString().isEmpty()) {
        kplugin.insert(QLatin1String("Authors"), QJsonArray{author});
    }

    QJsonObject root;
    root.insert(QLatin1String("KPlugin"), kplugin);

    copyIfExists(grp, root, "X-Plasma-DBusRunner-Service");
    copyIfExists(grp, root, "X-Plasma-DBusRunner-Path");
    copyIfExists(grp, root, "X-Plasma-Runner-Unique-Results", false);
    copyIfExists(grp, root, "X-Plasma-Runner-Weak-Results", false);
    copyIfExists(grp, root, "X-Plasma-API");
    copyIfExists(grp, root, "X-Plasma-Request-Actions-Once", false);
    copyIfExists(grp, root, "X-Plasma-Runner-Min-Letter-Count", 0);
    copyIfExists(grp, root, "X-Plasma-Runner-Match-Regex");
    copyIfExists(grp, root, "X-KDE-ConfigModule"); // DBus-Runners may also specify KCMs
    root.insert(QLatin1String("X-Plasma-Runner-Syntaxes"), QJsonArray::fromStringList(grp.readEntry("X-Plasma-Runner-Syntaxes", QStringList())));
    root.insert(QLatin1String("X-Plasma-Runner-Syntax-Descriptions"),
                QJsonArray::fromStringList(grp.readEntry("X-Plasma-Runner-Syntax-Descriptions", QStringList())));

    return KPluginMetaData(root, fileName);
}
