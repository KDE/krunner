/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_ABSTRACTRUNNER_H
#define PLASMA_ABSTRACTRUNNER_H

#include "krunner_export.h"

#include <QObject>
#include <QStringList>

class KConfigGroup;
class QIcon;
#include <KPluginMetaData>

#include <KPluginFactory>

#include <memory>

#include "querymatch.h"
#include "runnercontext.h"
#include "runnersyntax.h"

class QAction;
class QMimeData;
class QRegularExpression;

namespace Plasma
{
class DataEngine;
class Package;
class QueryMatch;
class AbstractRunnerPrivate;

/**
 * @class AbstractRunner abstractrunner.h <KRunner/AbstractRunner>
 *
 * @short An abstract base class for Plasma Runner plugins.
 *
 * Be aware that runners have to be thread-safe. This is due to the fact that
 * each runner is executed in its own thread for each new term. Thus, a runner
 * may be executed more than once at the same time. See match() for details.
 * To let krunner expose a global shortcut for the single runner query mode, the runner
 * must set the "X-Plasma-AdvertiseSingleRunnerMode" key to true in the .desktop file
 * and set a default syntax. See setDefaultSyntax() for details.
 *
 */
class KRUNNER_EXPORT AbstractRunner : public QObject
{
    Q_OBJECT

public:
    /** Specifies a priority for the runner */
    enum Priority {
        LowestPriority = 0,
        LowPriority,
        NormalPriority,
        HighPriority,
        HighestPriority,
    };

    /** An ordered list of runners */
    typedef QList<AbstractRunner *> List;

    ~AbstractRunner() override;

    /**
     * This is the main query method. It should trigger creation of
     * QueryMatch instances through RunnerContext::addMatch and
     * RunnerContext::addMatches.
     *
     * If the runner can run precisely the requested term (RunnerContext::query()),
     * it should create an exact match by setting the type to RunnerContext::ExactMatch.
     * The first runner that creates a QueryMatch will be the
     * default runner. Other runner's matches will be suggested in the
     * interface. Non-exact matches should be offered via RunnerContext::PossibleMatch.
     *
     * The match will be activated via run() if the user selects it.
     *
     * Each runner is executed in its own thread. Whenever the user input changes this
     * method is called again. Thus, it needs to be thread-safe. Also, all matches need
     * to be reported once this method returns. Asynchronous runners therefore need
     * to make use of a local event loop to wait for all matches.
     *
     * It is recommended to use local status data in async runners. The simplest way is
     * to have a separate class doing all the work like so:
     *
     * \code
     * void MyFancyAsyncRunner::match(RunnerContext &context)
     * {
     *     QEventLoop loop;
     *     MyAsyncWorker worker(context);
     *     connect(&worker, &MyAsyncWorker::finished, &loop, &MyAsyncWorker::quit);
     *     worker.work();
     *     loop.exec();
     * }
     * \endcode
     *
     * Here MyAsyncWorker creates all the matches and calls RunnerContext::addMatch
     * in some internal slot. It emits the finished() signal once done which will
     * quit the loop and make the match() method return. The local status is kept
     * entirely in MyAsyncWorker which makes match() trivially thread-safe.
     *
     * If a particular match supports multiple actions, set up the corresponding
     * actions in the actionsForMatch method. Do not call any of the action methods
     * within this method!
     *
     * Execution of the correct action should be handled in the run method.
     * @caution This method needs to be thread-safe since KRunner will simply
     * start a new thread for each new term.
     *
     * @warning Returning from this method means to end execution of the runner.
     *
     * @sa run(), RunnerContext::addMatch, RunnerContext::addMatches, QueryMatch
     */
    virtual void match(Plasma::RunnerContext &context);

    /**
     * Called whenever an exact or possible match associated with this
     * runner is triggered.
     *
     * @param context The context in which the match is triggered, i.e. for which
     *                the match was created.
     * @param match The actual match to run/execute.
     */
    virtual void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match);

    /**
     * The priority of the runner.
     * @see setPriority
     */
    Priority priority() const;

    /**
     * @return the user visible engine name for the Runner
     */
    QString name() const;

    /**
     * @return an id string for the Runner
     */
    QString id() const;

    /**
     * @return the description of this Runner
     */
    QString description() const;

    /**
     * @return the plugin metadata for this runner
     */
    KPluginMetaData metadata() const;
    /**
     * @return the icon for this Runner
     */
    QIcon icon() const;

    /**
     * Signal runner to reload its configuration.
     */
    virtual void reloadConfiguration();

    /**
     * @return the syntaxes the runner has registered that it accepts and understands
     */
    QList<RunnerSyntax> syntaxes() const;

    /**
     * @return true if the runner is currently busy with non-interuptable work, signaling that
     * new threads should not be created for it at this time
     */
    bool isMatchingSuspended() const;

    /**
     * This is the minimum letter count for the query. If the query is shorter than this value
     * and KRunner is not in the singleRunnerMode, the performMatch and consequently match method is not called.
     * This can be set using the X-Plasma-Runner-Min-Letter-Count property or the setMinLetterCount method.
     * @see setMinLetterCount
     * @see match
     * @see performMatch
     * @return minLetterCount property
     * @since 5.75
     */
    int minLetterCount() const;

    /**
     * Set the minLetterCount property
     * @param count
     * @since 5.75
     */
    void setMinLetterCount(int count);

    /**
     * If this regex is set with a not empty pattern it must match the query in
     * order for the performMatch/match being called.
     * Just like the minLetterCount property this check is ignored when the runner is in the singleRunnerMode.
     * In case both the regex and the letter count is set the letter count is checked first.
     * @return matchRegex property
     * @see hasMatchRegex
     * @since 5.75
     */
    QRegularExpression matchRegex() const;

    /**
     * Set the matchRegex property
     * @param regex
     * @since 5.75
     */
    void setMatchRegex(const QRegularExpression &regex);

    /**
     * Constructs internally a regex which requires the query to start with the trigger words.
     * Multiple words are concatenated with or, for instance: "^word1|word2|word3".
     * The trigger words are internally escaped.
     * Also the minLetterCount is set to the shortest word in the list.
     * @since 5.75
     * @see matchRegex
     */
    void setTriggerWords(const QStringList &triggerWords);

    /**
     * If the runner has a valid regex and non empty regex
     * @return hasMatchRegex
     * @since 5.75
     */
    bool hasMatchRegex() const;

Q_SIGNALS:
    /**
     * This signal is emitted when matching is about to commence, giving runners
     * an opportunity to prepare themselves, e.g. loading data sets or preparing
     * IPC or network connections. This method should be fast so as not to cause
     * slow downs. Things that take longer or which should be loaded once and
     * remain extant for the lifespan of the AbstractRunner should be done in init().
     * @see init()
     */
    void prepare();

    /**
     * This signal is emitted when a session of matches is complete, giving runners
     * the opportunity to tear down anything set up as a result of the prepare()
     * method.
     */
    void teardown();

    /**
     * Emitted when the runner enters or exits match suspension
     * @see matchingSuspended
     */
    void matchingSuspended(bool suspended);

protected:
    friend class RunnerManager;
    friend class RunnerManagerPrivate;

    /**
     * Constructor for a KRunner plugin
     * @param parent parent object for this runner
     * @param pluginMetaData metadata that was embedded in the runner
     * @param args for compatibility with KPluginFactory, should be passed on to the parent constructor
     * @since 5.72
     */
    AbstractRunner(QObject *parent, const KPluginMetaData &pluginMetaData, const QVariantList &args);

    /**
     * Sets whether or not the runner is available for match requests. Useful to
     * prevent more thread spawning when the thread is in a busy state.
     */
    void suspendMatching(bool suspend);

    /**
     * Provides access to the runner's configuration object.
     */
    KConfigGroup config() const;

    /**
     * Sets the priority of the runner. Lower priority runners are executed
     * only after higher priority runners.
     */
    void setPriority(Priority newPriority);

    /**
     * A given match can have more than action that can be performed on it.
     * For example, a song match returned by a music player runner can be queued,
     * added to the playlist, or played.
     *
     * Call this method to add actions that can be performed by the runner.
     * Actions must first be added to the runner's action registry.
     * Note: execution of correct action is left up to the runner.
     */
    virtual QList<QAction *> actionsForMatch(const Plasma::QueryMatch &match);

    /**
     * Adds a registered syntax that this runner understands. This is used to
     * display to the user what this runner can understand and how it can be
     * used.
     *
     * @param syntax the syntax to register
     */
    void addSyntax(const RunnerSyntax &syntax);

    /**
     * Sets the list of syntaxes; passing in an empty list effectively clears
     * the syntaxes.
     *
     * @param the syntaxes to register for this runner
     */
    void setSyntaxes(const QList<RunnerSyntax> &syns);

    /**
     * Reimplement this slot to run any initialization routines on first load.
     * By default, it calls reloadConfiguration(); for scripted Runners this
     * method also sets up the ScriptEngine.
     */
    virtual void init();

    /**
     * Reimplement this slot if you want your runner
     * to support serialization and drag and drop
     */
    virtual QMimeData *mimeDataForMatch(const Plasma::QueryMatch &match);

private:
    std::unique_ptr<AbstractRunnerPrivate> const d;
    bool hasUniqueResults();
    bool hasWeakResults();
    friend class RunnerContext;
    friend class RunnerContextPrivate;
    friend class QueryMatch;
};

} // Plasma namespace


#if !KRUNNER_ENABLE_DEPRECATED_SINCE(5, 91)
namespace KRunner
{
using AbstractRunner = Plasma::AbstractRunner;
using RunnerContext = Plasma::RunnerContext;
}
#endif

#endif
