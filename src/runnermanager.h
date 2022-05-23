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

#include "krunner_export.h"

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 91)
#include <KPluginInfo>
#endif
#include <KPluginMetaData>

#include <memory>

#include "abstractrunner.h"

class QAction;
class KConfigGroup;
namespace
{
class AbstractRunnerTest;
}

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
    Q_PROPERTY(bool retainPriorSearch READ retainPriorSearch)
    Q_PROPERTY(QString priorSearch READ priorSearch WRITE setPriorSearch)
    Q_PROPERTY(QStringList history READ history)
    Q_PROPERTY(bool historyEnabled READ historyEnabled NOTIFY historyEnabledChanged)

public:
    explicit RunnerManager(QObject *parent = nullptr);
    explicit RunnerManager(const QString &configFile, QObject *parent = nullptr);
#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
    /**
     * @deprecated Since 5.76, use "RunnerManager(const QString &configFile, QObject *parent)" instead.
     */
    KRUNNER_DEPRECATED_VERSION(5, 76, "use RunnerManager(const QString &configFile, QObject *parent) instead")
    explicit RunnerManager(KConfigGroup &config, QObject *parent = nullptr);
#endif
    ~RunnerManager() override;

    /**
     * Finds and returns a loaded runner or NULL
     * @param pluginName the name of the runner plugin
     * @return Pointer to the runner
     */
    AbstractRunner *runner(const QString &pluginName) const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 82)
    /**
     * @return the currently active "single mode" runner, or null if none
     * @since 4.4
     * @deprecated Since 5.81, the dedicated singleRunnerMode methods are deprecated, use runner(const QString &pluginName) with the singleRunnerId instead"
     */
    KRUNNER_DEPRECATED_VERSION(5,
                               82,
                               "The dedicated singleRunnerMode methods are deprecated, use runner(const QString &pluginName) with the singleRunnerId instead")
    AbstractRunner *singleModeRunner() const;

    /**
     * Puts the manager into "single runner" mode using the given
     * runner; if the runner does not exist or can not be loaded then
     * the single runner mode will not be started and singleModeRunner()
     * will return NULL
     * @param id the id of the runner to use
     * @since 4.4
     * @deprecated Since 5.82, the dedicated singleRunnerMode methods are deprecated, pass in the singleModeRunnerId into the launchQuery overload instead
     */
    KRUNNER_DEPRECATED_VERSION(5,
                               82,
                               "The dedicated singleRunnerMode methods are deprecated, pass in the singleModeRunnerId into the launchQuery overload instead")
    void setSingleModeRunnerId(const QString &id);

    /**
     * @return the id of the runner to use in single mode
     * @since 4.4
     * @deprecated Since 5.82, the dedicated singleRunnerMode methods are deprecated, use runner(const QString &pluginName) with the singleRunnerId instead
     */
    KRUNNER_DEPRECATED_VERSION(5, 82, "The dedicated singleRunnerMode methods are deprecated, save the variable before using it in launchQuery() instead")
    QString singleModeRunnerId() const;

    /**
     * @return true if the manager is set to run in single runner mode
     * @since 4.4
     * @deprecated Since 5.82, the dedicated singleRunnerMode methods are deprecated, call the RunnerContext::singleRunnerQueryMode on the searchContext instead
     */
    KRUNNER_DEPRECATED_VERSION(
        5,
        82,
        "The dedicated singleRunnerMode methods are deprecated, call the RunnerContext::singleRunnerQueryMode on the searchContext instead")
    bool singleMode() const;

    /**
     * Sets whether or not the manager is in single mode.
     *
     * @param singleMode true if the manager should be in single mode, false otherwise
     * @since 4.4
     * @deprecated Since 5.82, the dedicated singleRunnerMode methods are deprecated, the single mode is set to true when launchQuery is called with a non
     * empty and existing runnerId
     */
    KRUNNER_DEPRECATED_VERSION(5,
                               82,
                               "The dedicated singleRunnerMode methods are deprecated, the single mode is set to true when launchQuery is called with a non "
                               "empty and existing runnerId")
    void setSingleMode(bool singleMode);

    /**
     * @return the names of all runners that advertise single query mode
     * @since 4.4
     * @deprecated Since 5.81, filter the runners manually using the X-Plasma-AdvertiseSingleRunnerQueryMode of the metadata
     */
    KRUNNER_DEPRECATED_VERSION(5, 81, "filter the runners manually using the X-Plasma-AdvertiseSingleRunnerQueryMode of the metadata")
    QStringList singleModeAdvertisedRunnerIds() const;

    /**
     * Returns the translated name of a runner
     * @param id the id of the runner
     *
     * @since 4.4
     * @deprecated Since 5.81, call runner(const QString &id) and fetch the name from the returned object instead
     */
    KRUNNER_DEPRECATED_VERSION(5, 81, "call runner(const QString &id) and fetch the name from the returned object instead")
    QString runnerName(const QString &id) const;
#endif

    /**
     * @return the list of all currently loaded runners
     */
    QList<AbstractRunner *> runners() const;

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

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 79)
    /**
     * Runs a given match
     * @param id the id of the match to run
     * @deprecated Since 5.79, use run(const QueryMatch &match) instead
     */
    KRUNNER_DEPRECATED_VERSION(5, 79, "Use run(const QueryMatch &match) instead")
    void run(const QString &id);
#endif

    /**
     * Runs a given match. This also respects the extra handling for the InformationalMatch.
     * This also handles the history automatically
     * @param match the match to be executed
     * @return if the RunnerWindow should close
     * @since 5.78
     */
    bool runMatch(const QueryMatch &match);

    /**
     * Retrieves the list of actions, if any, for a match
     */
    QList<QAction *> actionsForMatch(const QueryMatch &match);

    /**
     * @return the current query term
     */
    QString query() const;

    /**
     * @return History of this runner for the current activity. If the RunnerManager is not history
     * aware the global entries will be returned.
     * @since 5.78
     */
    QStringList history() const;

    /**
     * Delete the given index from the history.
     * @param historyEntry
     * @since 5.78
     */
    Q_INVOKABLE void removeFromHistory(int index);

    /**
     * Get the suggested history entry for the typed query. If no entry is found an empty string is returned.
     * @param typedQuery
     * @return completion for typedQuery
     * @since 5.78
     */
    Q_INVOKABLE QString getHistorySuggestion(const QString &typedQuery) const;

    /**
     * Get the suggested prior search for this runner.
     * Just like the history this value can be activity specific, depending on the build/config.
     * @return priorSearch
     * @since 5.78
     */
    QString priorSearch() const;

    /**
     * Set the prior search for this runner. Setting an empty string will clear this value.
     * @since 5.78
     */
    void setPriorSearch(const QString &search);

    /**
     * If the prior search should be restored when KRunner is reopened
     * @since 5.78
     */
    bool retainPriorSearch();

    /**
     * If history completion is enabled, the default value is true.
     * @since 5.78
     */
    bool historyEnabled();

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

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 77)
    /**
     * Attempts to add the AbstractRunner from a Plasma::Package on disk.
     * Usually one can simply let the configuration of plugins
     * handle loading Runner plugins, but in cases where specific
     * runners should be loaded this allows for that to take place
     *
     * @param path the path to a Runner package to load
     * @since 4.5
     * @deprecated Since 5.0, the KPackage support was removed in Plasma 5.0
     */
    KRUNNER_DEPRECATED_VERSION_BELATED(5, 77, 5, 0, "the KPackage support was removed in Plasma 5.0")
    void loadRunner(const QString &path);
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 88)
    /**
     * @return the list of allowed plugins
     * @since 4.4
     * @deprecated Since 5.88, reading allowed runners from the config is deprecated, use @p runners() and get their @p AbstractRunner::id instead
     */
    KRUNNER_DEPRECATED_VERSION(5, 88, "reading allowed runners from the config is deprecated, use runners() and get their ids instead")
    QStringList allowedRunners() const;
#endif

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
    QMimeData *mimeDataForMatch(const QueryMatch &match) const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 79)
    /**
     * @return mime data of the specified match
     * @since 4.5
     * @deprecated Since 5.79, use mimeDataForMatch(const QueryMatch &match) instead
     */
    KRUNNER_DEPRECATED_VERSION(5, 79, "Use mimeDataForMatch(const QueryMatch &match) instead")
    QMimeData *mimeDataForMatch(const QString &matchId) const;
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 85)
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
     * @deprecated Since 5.85, the concept of parent apps for runners is deprecated, use no-arg overload instead
     **/
    KRUNNER_DEPRECATED_VERSION(5, 85, "The concept of parent apps for runners is deprecated, use no-arg overload instead")
    static QVector<KPluginMetaData> runnerMetaDataList(const QString &parentApp);
#endif

    /**
     * @return metadata list of all known Runner implementations
     * @since 5.72
     */
    static QVector<KPluginMetaData> runnerMetaDataList();

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
    void enableKNotifyPluginWatcher(); // TODO KF6 make enabling the watcher default behavior and remove the method

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
    void launchQuery(const QString &term); // TODO KF6 Merge with other overload and use default argument

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

    /**
     * Put the given search term in the KRunner search field
     * @param term The term that should be displayed
     * @param cursorPosition Where the cursor should be positioned
     * @since 5.78
     */
    void setSearchTerm(const QString &term, int cursorPosition);

    /**
     * @see @p historyEnabled
     * @since 5.78
     */
    void historyEnabledChanged();

private:
    Q_PRIVATE_SLOT(d, void jobDone(ThreadWeaver::JobPointer))
    KPluginMetaData convertDBusRunnerToJson(const QString &filename) const;

    std::unique_ptr<RunnerManagerPrivate> const d;

    friend class RunnerManagerPrivate;
    friend AbstractRunnerTest;
};

}
#if !KRUNNER_ENABLE_DEPRECATED_SINCE(5, 91)
namespace KRunner
{
using RunnerManager = Plasma::RunnerManager;
}
#endif

#endif
