/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2007 Ryan P. Bitanga <ryan.bitanga@gmail.com>
    SPDX-FileCopyrightText: 2008 Jordi Polo <mumismo@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_RUNNERMANAGER_H
#define PLASMA_RUNNERMANAGER_H

#include <QList>
#include <QObject>

#include <KPluginInfo>
#include <KPluginMetaData>

#include "krunner_export.h"
#include "abstractrunner.h"

class QAction;
class KConfigGroup;

namespace Plasma
{
    class QueryMatch;
    class AbstractRunner;
    class RunnerContext;
    class RunnerManagerPrivate;

/**
 * @class RunnerManager runnermanager.h <KRunner/RunnerManager>
 *
 * @short The RunnerManager class decides what installed runners are runnable,
 *        and their ratings. It is the main proxy to the runners.
 */
class KRUNNER_EXPORT RunnerManager : public QObject
{
    Q_OBJECT

    public:
        explicit RunnerManager(QObject *parent=nullptr);
        explicit RunnerManager(const QString &configFile, QObject *parent=nullptr);
        explicit RunnerManager(KConfigGroup &config, QObject *parent=nullptr);
        ~RunnerManager();

        /**
         * Finds and returns a loaded runner or NULL
         * @param pluginName the name of the runner plugin
         * @return Pointer to the runner
         */
        AbstractRunner *runner(const QString &pluginName) const;

        /**
         * @return the currently active "single mode" runner, or null if none
         * @since 4.4
         */
        AbstractRunner *singleModeRunner() const;

        /**
         * Puts the manager into "single runner" mode using the given
         * runner; if the runner does not exist or can not be loaded then
         * the single runner mode will not be started and singleModeRunner()
         * will return NULL
         * @param id the id of the runner to use
         * @since 4.4
         */
        void setSingleModeRunnerId(const QString &id);

        /**
         * @return the id of the runner to use in single mode
         * @since 4.4
         */
        QString singleModeRunnerId() const;

        /**
         * @return true if the manager is set to run in single runner mode
         * @since 4.4
         */
        bool singleMode() const;

        /**
         * Sets whether or not the manager is in single mode.
         *
         * @param singleMode true if the manager should be in single mode, false otherwise
         * @since 4.4
         */
        void setSingleMode(bool singleMode);

        /**
         * Returns the translated name of a runner
         * @param id the id of the runner
         *
         * @since 4.4
         */
        QString runnerName(const QString &id) const;

        /**
         * @return the list of all currently loaded runners
         */
        QList<AbstractRunner *> runners() const;

        /**
         * @return the names of all runners that advertise single query mode
         * @since 4.4
         */
        QStringList singleModeAdvertisedRunnerIds() const;

        /**
         * Retrieves the current context
         * @return pointer to the current context
         */
        RunnerContext *searchContext() const;

        /**
         * Retrieves all available matches found so far for the previously launched query
         * @return List of matches
         */
        QList<QueryMatch> matches() const;

        /**
         * Runs a given match
         * @param match the match to be executed
         */
        void run(const QueryMatch &match);

        /**
         * Runs a given match
         * @param id the id of the match to run
         */
        void run(const QString &id);

        /**
         * Retrieves the list of actions, if any, for a match
         */
        QList<QAction*> actionsForMatch(const QueryMatch &match);

        /**
         * @return the current query term
         */
        QString query() const;

        /**
         * Causes a reload of the current configuration
         */
        void reloadConfiguration();

        /**
         * Sets a whitelist for the plugins that can be loaded by this manager.
         *
         * @param plugins the plugin names of allowed runners
         * @since 4.4
         */
        void setAllowedRunners(const QStringList &runners);

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
        /**
         * Sets the list of categories which matches should be
         * returned for. It also internally tries not to execute the
         * runners which do not fall in this category.
         * @deprecated Since 5.76, feature is unused and not supported by most runners
         */
        KRUNNER_DEPRECATED_VERSION(5, 76, "feature is unused and not supported by most runners")
        void setEnabledCategories(const QStringList &categories);
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 72)
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
        /**
         * Attempts to add the AbstractRunner plugin represented
         * by the KService passed in. Usually one can simply let
         * the configuration of plugins handle loading Runner plugins,
         * but in cases where specific runners should be loaded this
         * allows for that to take place
         *
         * @param service the service to use to load the plugin
         * @since 4.5
         * @deprecated Since 5.72, use loadRunner(const KPluginMetaData &)
         */
        KRUNNER_DEPRECATED_VERSION(5, 72, "use loadRunner(const KPluginMetaData &)")
        void loadRunner(const KService::Ptr service);
#endif
#endif

        /**
         * Attempts to add the AbstractRunner plugin represented
         * by the plugin info passed in. Usually one can simply let
         * the configuration of plugins handle loading Runner plugins,
         * but in cases where specific runners should be loaded this
         * allows for that to take place
         *
         * @param pluginMetaData the metaData to use to load the plugin
         * @since 5.72
         */
        void loadRunner(const KPluginMetaData &pluginMetaData);

        /**
         * Attempts to add the AbstractRunner from a Plasma::Package on disk.
         * Usually one can simply let the configuration of plugins
         * handle loading Runner plugins, but in cases where specific
         * runners should be loaded this allows for that to take place
         *
         * @param path the path to a Runner package to load
         * @since 4.5
         */
        void loadRunner(const QString &path);

        /**
         * @return the list of allowed plugins
         * @since 4.4
         */
        QStringList allowedRunners() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
        /**
         * @return the list of enabled categories
         * @deprecated Since 5.76, feature is unused and not supported by most runners
         */
        KRUNNER_DEPRECATED_VERSION(5, 76, "feature is unused and not supported by most runners")
        QStringList enabledCategories() const;
#endif

        /**
         * @return mime data of the specified match
         * @since 4.5
         */
        QMimeData * mimeDataForMatch(const QueryMatch &match) const;

        /**
         * @return mime data of the specified match
         * @since 4.5
         */
        QMimeData * mimeDataForMatch(const QString &matchId) const;

        /**
         * Returns a list of all known Runner implementations
         *
         * @param parentApp the application to filter runners on. Uses the
         *                  X-KDE-ParentApp entry (if any) in the plugin metadata.
         *                  The default value of QString() will result in a
         *                  list containing only runners not specifically
         *                  registered to an application.
         * @return list of metadata of known runners
         * @since 5.72
         **/
        static QVector<KPluginMetaData> runnerMetaDataList(const QString &parentApp = QString());

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 72)
        /**
         * Returns a list of all known Runner implementations
         *
         * @param parentApp the application to filter applets on. Uses the
         *                  X-KDE-ParentApp entry (if any) in the plugin info.
         *                  The default value of QString() will result in a
         *                  list containing only applets not specifically
         *                  registered to an application.
         * @return list of AbstractRunners
         * @since 4.6
         * @deprecated since 5.72, use runnerMetaDataList() instead
         **/
        KRUNNER_DEPRECATED_VERSION(5, 72, "Use runnerMetaDataList() instead")
        static KPluginInfo::List listRunnerInfo(const QString &parentApp = QString());
#endif

        /**
         * If you call this method the manager will create a KConfigWatcher
         * which reload its runners or the runner configuration when the settings in the KCM are edited.
         * @since 5.73
         * @see reloadConfiguration
         */
        void enableKNotifyPluginWatcher();

        void addMatchToHistory(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match);

        QStringList getHistory() const;

    public Q_SLOTS:
        /**
         * Call this method when the runners should be prepared for a query session.
         * Call matchSessionComplete when the query session is finished for the time
         * being.
         * @since 4.4
         * @see matchSessionComplete
         */
        void setupMatchSession();

        /**
         * Call this method when the query session is finished for the time
         * being.
         * @since 4.4
         * @see prepareForMatchSession
         */
        void matchSessionComplete();

        /**
         * Launch a query, this will create threads and return immediately.
         * When the information will be available can be known using the
         * matchesChanged signal.
         *
         * @param term the term we want to find matches for
         * @param runnerId optional, if only one specific runner is to be used;
         *               providing an id will put the manager into single runner mode
         */
        void launchQuery(const QString &term, const QString &runnerId);

        /**
         * Convenience version of above
         */
        void launchQuery(const QString &term);

        /**
         * Reset the current data and stops the query
         */
        void reset();

    Q_SIGNALS:
        /**
         * Emitted each time a new match is added to the list
         */
        void matchesChanged(const QList<Plasma::QueryMatch> &matches);

        /**
         * Emitted when the launchQuery finish
         * @since 4.5
         */
        void queryFinished();

    private:
        Q_PRIVATE_SLOT(d, void scheduleMatchesChanged())
        Q_PRIVATE_SLOT(d, void matchesChanged())
        Q_PRIVATE_SLOT(d, void jobDone(ThreadWeaver::JobPointer))
        Q_PRIVATE_SLOT(d, void unblockJobs())
        Q_PRIVATE_SLOT(d, void runnerMatchingSuspended(bool))

        RunnerManagerPrivate * const d;

        friend class RunnerManagerPrivate;
};

}

#endif

