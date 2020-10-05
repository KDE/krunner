/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_ABSTRACTRUNNER_H
#define PLASMA_ABSTRACTRUNNER_H

#include <QObject>
#include <QStringList>
#include <QIcon>

#include <KConfigGroup>
#include <KService>
#include <KPluginInfo>
#include <KPluginMetaData>

#include <plasma_version.h>
#include <plasma/plasma_export.h> // for PLASMA_ENABLE_DEPRECATED_SINCE

#include "krunner_export.h"
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

enum RunnerReturnPluginMetaDataConstant { RunnerReturnPluginMetaData }; // KF6: remove again

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
    Q_PROPERTY(bool matchingSuspended READ isMatchingSuspended WRITE suspendMatching NOTIFY matchingSuspended)
    Q_PROPERTY(QString id READ id)
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QIcon icon READ icon)
    Q_PROPERTY(int minLetterCount READ minLetterCount WRITE setMinLetterCount)
    Q_PROPERTY(QRegularExpression matchRegex READ matchRegex WRITE setMatchRegex)

    public:
        /** Specifies a nominal speed for the runner */
        enum Speed {
            SlowSpeed,
            NormalSpeed
        };

        /** Specifies a priority for the runner */
        enum Priority {
            LowestPriority = 0,
            LowPriority,
            NormalPriority,
            HighPriority,
            HighestPriority
        };

        /** An ordered list of runners */
        typedef QList<AbstractRunner*> List;

        virtual ~AbstractRunner();

        /**
         * This is the main query method. It should trigger creation of
         * QueryMatch instances through RunnerContext::addMatch and
         * RunnerContext::addMatches. It is called internally by performMatch().
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
         * Triggers a call to match. This will call match() internally.
         *
         * @param context the search context used in executing this match.
         */
        void performMatch(Plasma::RunnerContext &context);

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 71)
        /**
         * If the runner has options that the user can interact with to modify
         * what happens when run or one of the actions created in match
         * is called, the runner should return true
         * @deprecated Since 5.0, this feature has been defunct
         */
        KRUNNER_DEPRECATED_VERSION_BELATED(5, 71,  5, 0, "No longer use, feature removed")
        bool hasRunOptions();
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 71)
        /**
         * If hasRunOptions() returns true, this method may be called to get
         * a widget displaying the options the user can interact with to modify
         * the behaviour of what happens when a given match is selected.
         *
         * @param widget the parent of the options widgets.
         * @deprecated Since 5.0, this feature has been defunct
         */
        KRUNNER_DEPRECATED_VERSION_BELATED(5, 71,  5, 0, "No longer use, feature removed")
        virtual void createRunOptions(QWidget *widget);
#endif

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
         * Return a list of categories that this runner provides. By default
         * this list just contains the runners name. It is used by the runner manager
         * to disable certain runners if all the categories they provide have
         * been disabled.
         *
         * This list of categories is also used to provide a better way to
         * configure the runner instead of the typical on / off switch.
         */
        virtual QStringList categories() const;

        /**
         * Returns the icon which accurately describes the category \p category.
         * This is meant to be used in a configuration dialog when showing
         * all the categories.
         *
         * By default this returns the icon of the AbstractRunner
         */
        virtual QIcon categoryIcon(const QString& category) const;

        /**
         * The nominal speed of the runner.
         * @see setSpeed
         */
        Speed speed() const;

        /**
         * The priority of the runner.
         * @see setPriority
         */
        Priority priority() const;

        /**
         * Returns the OR'ed value of all the Information types (as defined in RunnerContext::Type)
         * this runner is not interested in.
         * @return OR'ed value of black listed types
         */
        RunnerContext::Types ignoredTypes() const;

        /**
         * Sets the types this runner will ignore
         * @param types OR'ed listed of ignored types
         */
        void setIgnoredTypes(RunnerContext::Types types);

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

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 72)
        /**
         * @return the plugin info for this runner
         * @deprecated since 5.72, use metaData(Plasma::RunnerReturnPluginMetaDataConstant) instead, see its API docs
         */
        KRUNNER_DEPRECATED_VERSION(5, 72, "Use metaData(Plasma::RunnerReturnPluginMetaDataConstant) instead, see its API docs")
        KPluginInfo metadata() const;
#endif

        /**
         * @return the plugin metadata for this runner
         *
         * Overload to get non-deprecated metadata format. Use like this:
         * @code
         * KPluginMetaData md = runner->metadata(Plasma::RunnerReturnPluginMetaData);
         * @endcode
         * If you disable the deprecated version using the KRUNNER_DISABLE_DEPRECATED_BEFORE_AND_AT macro,
         * then you can omit Plasma::RunnerReturnPluginMetaDataConstant and use it like this:
         * @code
         * KPluginMetaData md = runner->metadata();
         * @endcode
         *
         * @since 5.72
         */
#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 72)
        KPluginMetaData metadata(RunnerReturnPluginMetaDataConstant) const;
#else
        KPluginMetaData metadata(RunnerReturnPluginMetaDataConstant = RunnerReturnPluginMetaData) const;
#endif
        /**
         * @return the icon for this Runner
         */
        QIcon icon() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 65) // not 5.28 here, this KRUNNER visibility control only added at 5.65
#if PLASMA_ENABLE_DEPRECATED_SINCE(5, 28) // Plasma::Package is defined with this condition
        /**
         * Accessor for the associated Package object if any.
         *
         * Note that the returned pointer is only valid for the lifetime of
         * the runner.
         *
         * @return the Package object, which may be invalid
         * @deprecated since 5.28, use KPackage::Package instead, no accessor in this class
         **/
        KRUNNER_DEPRECATED_VERSION(5, 28, "No longer use, feature removed")
        Package package() const;
#endif
#endif

        /**
         * Signal runner to reload its configuration.
         */
        virtual void reloadConfiguration();

        /**
         * @return the syntaxes the runner has registered that it accepts and understands
         * @since 4.3
         */
        QList<RunnerSyntax> syntaxes() const;

        /**
         * @return the default syntax for the runner or @c nullptr if no default syntax has been defined
         *
         * @since 4.4
         */
        RunnerSyntax *defaultSyntax() const;

        /**
         * @return true if the runner is currently busy with non-interuptable work, signaling that
         * new threads should not be created for it at this time
         * @since 4.6
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
         * @since 4.4
         */
        void prepare();

        /**
         * This signal is emitted when a session of matches is complete, giving runners
         * the opportunity to tear down anything set up as a result of the prepare()
         * method.
         * @since 4.4
         */
        void teardown();

        /**
         * Emitted when the runner enters or exits match suspension
         * @see matchingSuspended
         * @since 4.6
         */
        void matchingSuspended(bool suspended);

    protected:
        friend class RunnerManager;
        friend class RunnerManagerPrivate;

        explicit AbstractRunner(QObject *parent = nullptr, const QString &path = QString());
#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 72)
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
         /// @deprecated Since 5.72, use AbstractRunner(const KPluginMetaData &, QObject *)
        KRUNNER_DEPRECATED_VERSION(5, 72, "use AbstractRunner(const KPluginMetaData &, QObject *)")
        explicit AbstractRunner(const KService::Ptr service, QObject *parent = nullptr);
#endif
#endif
        /// @since 5.72
        explicit AbstractRunner(const KPluginMetaData &pluginMetaData, QObject *parent = nullptr);

        AbstractRunner(QObject *parent, const QVariantList &args);

        /**
         * Sets whether or not the runner is available for match requests. Useful to
         * prevent more thread spawning when the thread is in a busy state.
         */
        void suspendMatching(bool suspend);

        /**
         * Provides access to the runner's configuration object.
         */
        KConfigGroup config() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 71)
        /**
         * Sets whether or not the runner has options for matches
         * @deprecated Since 5.0, this feature has been defunct
         */
        KRUNNER_DEPRECATED_VERSION_BELATED(5, 71,  5, 0, "No longer use, feature removed")
        void setHasRunOptions(bool hasRunOptions);
#endif

        /**
         * Sets the nominal speed of the runner. Only slow runners need
         * to call this within their constructor because the default
         * speed is NormalSpeed. Runners that use D-Bus should call
         * this within their constructors.
         */
        void setSpeed(Speed newSpeed);

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
        virtual QList<QAction*> actionsForMatch(const Plasma::QueryMatch &match);

        /**
         * Creates and then adds an action to the action registry.
         * AbstractRunner assumes ownership of the created action.
         *
         * @param id A unique identifier string
         * @param icon The icon to display
         * @param text The text to display
         * @return the created QAction
         */
        QAction* addAction(const QString &id, const QIcon &icon, const QString &text);

        /**
         * Adds an action to the runner's action registry.
         *
         * The QAction must be created within the GUI thread;
         * do not create it within the match method of AbstractRunner.
         *
         * @param id A unique identifier string
         * @param action The QAction to be stored
         */
        void addAction(const QString &id, QAction *action);

        /**
         * Removes the action from the action registry.
         * AbstractRunner deletes the action once removed.
         *
         * @param id The id of the action to be removed
         */
        void removeAction(const QString &id);

        /**
         * Returns the action associated with the id
         */
        QAction* action(const QString &id) const;

        /**
         * Returns all registered actions
         */
        QHash<QString, QAction*> actions() const;

        /**
         * Clears the action registry.
         * The action pool deletes the actions.
         */
        void clearActions();

        /**
         * Add a match. This can be alter fetched using the getMatch method.
         * As a key the id of the match is used. But in case the id was indirectly specified when setting the data the
         * data as a string can be used to retrieve this entry.
         * @see QueryMatch::idIsDetByData()
         * @param match
         * @since 5.76
         */
        void addMatch(const Plasma::QueryMatch &match);

        /**
         * Add a match. This can be alter fetched using getMatch(id)
         * @param id
         * @param match
         * @since 5.76
         */
        void addMatch(const QString &id, const Plasma::QueryMatch &match);

        /**
         * Removes match with the given id.
         * @param id
         * @since 5.76
         */
        void removeMatch(const QString &id);

        /**
         * Clears all matches.
         * @since 5.76
         */
        void clearMatches();

        /**
         * Get match with the given id.
         * @param id
         * @return QueryMatch
         * @since 5.76
         */
        QueryMatch getMatch(const QString &id) const;

        /**
         * QHash of all the matches that are set for this runner.
         * @return QHash<QString, Plasma::QueryMatch>
         * @since 5.76
         */
        QHash<QString, Plasma::QueryMatch> matches() const;

        /**
         * Adds a registered syntax that this runner understands. This is used to
         * display to the user what this runner can understand and how it can be
         * used.
         *
         * @param syntax the syntax to register
         * @since 4.3
         */
        void addSyntax(const RunnerSyntax &syntax);

        /**
         * Set @p syntax as the default syntax for the runner; the default syntax will be
         * substituted to the empty query in single runner mode. This is also used to
         * display to the user what this runner can understand and how it can be
         * used.
         * The default syntax is automatically added to the list of registered syntaxes, there
         * is no need to add it using addSyntax.
         * Note that there can be only one default syntax; if called more than once, the last 
         * call will determine the default syntax.
         * A default syntax (even trivial) is required to advertise single runner mode
         *
         * @param syntax the syntax to register and to set as default
         * @since 4.4
         **/
        void setDefaultSyntax(const RunnerSyntax &syntax);

        /**
         * Sets the list of syntaxes; passing in an empty list effectively clears
         * the syntaxes.
         *
         * @param the syntaxes to register for this runner
         * @since 4.3
         */
        void setSyntaxes(const QList<RunnerSyntax> &syns);

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 73)
        /**
         * Loads the given DataEngine
         *
         * Tries to load the data engine given by @p name.  Each engine is
         * only loaded once, and that instance is re-used on all subsequent
         * requests.
         *
         * If the data engine was not found, an invalid data engine is returned
         * (see DataEngine::isValid()).
         *
         * Note that you should <em>not</em> delete the returned engine.
         *
         * @param name Name of the data engine to load
         * @return pointer to the data engine if it was loaded,
         *         or an invalid data engine if the requested engine
         *         could not be loaded
         *
         * @since 4.4
         * @deprecated Since 5.73, DataEngines are deprecated, use e.g. a shared library to provide the data instead.
         */
        KRUNNER_DEPRECATED_VERSION(5, 73, "DataEngines are deprecated, use e.g. a shared library to provide the data instead.")
        Q_INVOKABLE DataEngine *dataEngine(const QString &name) const;
#endif

        /**
         * Reimplement this slot to run any initialization routines on first load.
         * By default, it calls reloadConfiguration(); for scripted Runners this
         * method also sets up the ScriptEngine.
         */
        virtual void init();

        /**
         * Reimplement this slot if you want your runner
         * to support serialization and drag and drop
         * @since 4.5
         */
        virtual QMimeData *mimeDataForMatch(const Plasma::QueryMatch &match);

    private:
        AbstractRunnerPrivate *const d;
};

} // Plasma namespace


#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 72)
// Boilerplate to emit a version-controlled warning about the deprecated macro at leats with GCC
#if KRUNNER_DEPRECATED_WARNINGS_SINCE >= 0x054800 // 5.72.0
#   if defined(__GNUC__)
#       define K_EXPORT_PLASMA_RUNNER_DO_PRAGMA(x) _Pragma (#x)
#       define K_EXPORT_PLASMA_RUNNER_WARNING(x) K_EXPORT_PLASMA_RUNNER_DO_PRAGMA(message(#x))
#   else
#       define K_EXPORT_PLASMA_RUNNER_WARNING(x)
#   endif
#else
#   define K_EXPORT_PLASMA_RUNNER_WARNING(x)
#endif
/**
 * @relates Plasma::AbstractRunner
 *
 * Registers a runner plugin.
 *
 * @deprecated Since 5.72, use K_EXPORT_PLASMA_RUNNER_WITH_JSON(classname, jsonFile) instead
 */
#define K_EXPORT_PLASMA_RUNNER( libname, classname )     \
K_EXPORT_PLASMA_RUNNER_WARNING("Deprecated. Since 5.72, use K_EXPORT_PLASMA_RUNNER_WITH_JSON(classname, jsonFile) instead") \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)
#endif

/**
 * @relates Plasma::AbstractRunner
 *
 * Registers a runner plugin with JSON metadata.
 *
 * @param classname name of the AbstractRunner derived class.
 * @param jsonFile name of the JSON file to be compiled into the plugin as metadata
 *
 * @since 5.72
 */
#define K_EXPORT_PLASMA_RUNNER_WITH_JSON(classname, jsonFile) \
    K_PLUGIN_FACTORY_WITH_JSON(classname ## Factory, jsonFile, registerPlugin<classname>();) \
    K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 75)
/**
 * These plugins are Used by the plugin selector dialog to show
 * configuration options specific to this runner. These options
 * must be runner global and not pertain to a specific match.
 * @deprecated Since 5.0, use K_PLUGIN_FACTORY directly
 */
#define K_EXPORT_RUNNER_CONFIG( name, classname )     \
K_PLUGIN_FACTORY(ConfigFactory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)
#endif

#endif
