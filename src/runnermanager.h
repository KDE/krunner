/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2007 Ryan P. Bitanga <ryan.bitanga@gmail.com>
    SPDX-FileCopyrightText: 2008 Jordi Polo <mumismo@gmail.com>
    SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnauŋmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KRUNNER_RUNNERMANAGER_H
#define KRUNNER_RUNNERMANAGER_H

#include <QList>
#include <QObject>

#include <KConfigWatcher>
#include <KPluginMetaData>

#include "abstractrunner.h"
#include "action.h"
#include "krunner_export.h"
#include <memory>

class KConfigGroup;
namespace KRunner
{
class AbstractRunnerTest;
}

namespace KRunner
{
class QueryMatch;
class AbstractRunner;
class RunnerContext;
class RunnerManagerPrivate;

/*!
 * \class KRunner::RunnerManager
 * \inheaderfile KRunner/RunnerManager
 * \inmodule KRunner
 *
 * \brief The RunnerManager class decides what installed runners are runnable,
 *        and their ratings. It is the main proxy to the runners.
 */
class KRUNNER_EXPORT RunnerManager : public QObject
{
    Q_OBJECT

    /*!
     * \property KRunner::RunnerManager::history
     */
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)

    /*!
     * \property KRunner::RunnerManager::querying
     */
    Q_PROPERTY(bool querying READ querying NOTIFY queryingChanged)

    /*!
     * \property KRunner::RunnerManager::historyEnabled
     */
    Q_PROPERTY(bool historyEnabled READ historyEnabled WRITE setHistoryEnabled NOTIFY historyEnabledChanged)

public:
    /*!
     * Constructs a RunnerManager with the given parameters
     *
     * \a configurationGroup Config group used for reading enabled plugins
     *
     * \a stateGroup Config group used for storing history
     *
     * \since 6.0
     */
    explicit RunnerManager(const KConfigGroup &pluginConfigGroup, const KConfigGroup &stateGroup, QObject *parent);

    /*!
     * Constructs a RunnerManager using the default locations for state/plugin config
     */
    explicit RunnerManager(QObject *parent = nullptr);
    ~RunnerManager() override;

    /*!
     * Finds and returns a loaded runner or a nullptr
     *
     * \a pluginId the name of the runner plugin
     *
     * Returns Pointer to the runner
     */
    AbstractRunner *runner(const QString &pluginId) const;

    /*!
     * Returns the list of all currently loaded runners
     */
    QList<AbstractRunner *> runners() const;

    /*!
     * Retrieves the current context
     *
     * Returns pointer to the current context
     */
    RunnerContext *searchContext() const;

    /*!
     * Retrieves all available matches found so far for the previously launched query
     */
    QList<QueryMatch> matches() const;

    /*!
     * Runs a given match. This also respects the extra handling for the InformationalMatch.
     *
     * This also handles the history automatically
     *
     * \a match the match to be executed
     *
     * \a selectedAction the action returned by QueryMatch::actions that has been selected by the user, nullptr if none
     *
     * Returns if the RunnerWindow should close
     *
     * \since 6.0
     */
    bool run(const QueryMatch &match, const KRunner::Action &action = {});

    /*!
     * Returns the current query term set in launchQuery
     */
    QString query() const;

    /*!
     * Returns History of this runner for the current activity. If the RunnerManager is not history
     * aware the global entries will be returned.
     * \since 5.78
     */
    QStringList history() const;

    /*!
     * Delete the given index from the history.
     *
     * \a historyEntry
     * \since 5.78
     */
    Q_INVOKABLE void removeFromHistory(int index);

    /*!
     * Get the suggested history entry for the typed query. If no entry is found an empty string is returned.
     *
     * \a typedQuery
     *
     * Returns completion for typedQuery
     * \since 5.78
     */
    Q_INVOKABLE QString getHistorySuggestion(const QString &typedQuery) const;

    /*!
     * If history completion is enabled, the default value is true.
     * \since 5.78
     */
    bool historyEnabled();

    /*!
     * If the RunnerManager is currently querying
     * \since 6.7
     */
    bool querying() const;

    /*!
     * Enables/disabled the history feature for the RunnerManager instance.
     * The value will not be persisted and is only kept during the object's lifetime.
     *
     * \since 6.0
     */
    void setHistoryEnabled(bool enabled);

    /*!
     * Causes a reload of the current configuration
     *
     * This gets called automatically when the config in the KCM is saved
     */
    void reloadConfiguration();

    /*!
     * Sets a whitelist for the plugins that can be loaded by this manager.
     *
     * Runners that are disabled through the config will not be loaded.
     *
     * \a plugins the plugin names of allowed runners
     */
    void setAllowedRunners(const QStringList &runners);

    /*!
     * Attempts to add the AbstractRunner plugin represented
     * by the plugin info passed in. Usually one can simply let
     * the configuration of plugins handle loading Runner plugins,
     * but in cases where specific runners should be loaded this
     * allows for that to take place
     *
     * \note Consider using setAllowedRunners in case you want to only allow specific runners
     *
     * \a pluginMetaData the metaData to use to load the plugin
     *
     * Returns the loaded runner or nullptr
     */
    AbstractRunner *loadRunner(const KPluginMetaData &pluginMetaData);

    /*!
     * Returns mime data of the specified match
     */
    QMimeData *mimeDataForMatch(const QueryMatch &match) const;

    /*!
     * Returns metadata list of all known Runner plugins
     * \since 5.72
     */
    static QList<KPluginMetaData> runnerMetaDataList();

public Q_SLOTS:
    /*!
     * Call this method when the runners should be prepared for a query session.
     * Call matchSessionComplete when the query session is finished for the time
     * being.
     * \sa matchSessionComplete
     */
    void setupMatchSession();

    /*!
     * Call this method when the query session is finished for the time
     * being.
     * \sa prepareForMatchSession
     */
    void matchSessionComplete();

    /*!
     * Launch a query, this will create threads and return immediately.
     * When the information will be available can be known using the
     * matchesChanged signal.
     *
     * \a term the term we want to find matches for
     *
     * \a runnerId optional, if only one specific runner is to be used;
     *               providing an id will put the manager into single runner mode
     */
    void launchQuery(const QString &term, const QString &runnerId = QString());

    /*!
     * Reset the current data and stops the query
     */
    void reset();

    /*!
     * Set the environment identifier for recording history and launch counts
     * \internal
     * \since 6.0
     */
    Q_INVOKABLE void setHistoryEnvironmentIdentifier(const QString &identifier);

Q_SIGNALS:
    /*!
     * Emitted each time a new match is added to the list
     */
    void matchesChanged(const QList<KRunner::QueryMatch> &matches);

    /*!
     * Emitted when the launchQuery finish
     */
    void queryFinished();

    /*!
     * Emitted when the querying status has changed
     * \since 6.7
     */

    void queryingChanged();

    /*!
     * Put the given search term in the KRunner search field
     *
     * \a term The term that should be displayed
     *
     * \a cursorPosition Where the cursor should be positioned
     * \since 6.0
     */
    void requestUpdateQueryString(const QString &term, int cursorPosition);

    /*!
     * \sa historyEnabled
     * \since 5.78
     */
    void historyEnabledChanged();

    /*!
     * Emitted when the history has changed
     * \since 6.21
     */

    void historyChanged();

private:
    // exported for dbusrunnertest
    KPluginMetaData convertDBusRunnerToJson(const QString &filename) const;
    KRUNNER_NO_EXPORT Q_INVOKABLE void onMatchesChanged();

    std::unique_ptr<RunnerManagerPrivate> d;
    KConfigWatcher::Ptr m_stateWatcher;

    friend class RunnerManagerPrivate;
    friend AbstractRunnerTest;
    friend AbstractRunner;
};

}
#endif
