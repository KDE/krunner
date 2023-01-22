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
    ~RunnerManager() override;

    /**
     * Finds and returns a loaded runner or NULL
     * @param pluginName the name of the runner plugin
     * @return Pointer to the runner
     */
    AbstractRunner *runner(const QString &pluginName) const;

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
     * @return mime data of the specified match
     * @since 4.5
     */
    QMimeData *mimeDataForMatch(const QueryMatch &match) const;

    /**
     * @return metadata list of all known Runner implementations
     * @since 5.72
     */
    static QVector<KPluginMetaData> runnerMetaDataList();

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
