/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_ABSTRACTRUNNER_H
#define PLASMA_ABSTRACTRUNNER_H

#include "krunner_export.h"

#include <QObject>
#include <QStringList>

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 94)
#include <KConfigGroup>
#include <QIcon>
#else
class KConfigGroup;
class QIcon;
#endif
#include <KPluginMetaData>

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 91)
#include <KPluginInfo>
#include <KService>
#else
#include <KPluginFactory>
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 65)
#include <plasma/plasma_export.h> // for PLASMA_ENABLE_DEPRECATED_SINCE
#include <plasma_version.h>
#endif

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
#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 81)
    /** Specifies a nominal speed for the runner */
    enum Speed {
        SlowSpeed,
        NormalSpeed,
    };
#endif

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

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 81)
    /**
     * Triggers a call to match. This will call match() internally.
     *
     * @param context the search context used in executing this match.
     * @deprecated Since 5.81, use match(Plasma::RunnerContext &context) instead.
     * This method contains logic to delay slow runners, which is now deprecated. Consequently you
     * should call match(Plasma::RunnerContext &context) directly.
     */
    KRUNNER_DEPRECATED_VERSION(5, 81, "use match(Plasma::RunnerContext &context) instead")
    void performMatch(Plasma::RunnerContext &context);
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 71)
    /**
     * If the runner has options that the user can interact with to modify
     * what happens when run or one of the actions created in match
     * is called, the runner should return true
     * @deprecated Since 5.0, this feature has been defunct
     */
    KRUNNER_DEPRECATED_VERSION_BELATED(5, 71, 5, 0, "No longer use, feature removed")
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
    KRUNNER_DEPRECATED_VERSION_BELATED(5, 71, 5, 0, "No longer use, feature removed")
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

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
    /**
     * Return a list of categories that this runner provides. By default
     * this list just contains the runners name. It is used by the runner manager
     * to disable certain runners if all the categories they provide have
     * been disabled.
     *
     * This list of categories is also used to provide a better way to
     * configure the runner instead of the typical on / off switch.
     * @deprecated Since 5.76, feature is unused. You can still set the category property in the QueryMatch
     */
    KRUNNER_DEPRECATED_VERSION(5, 76, "Feature is unused")
    virtual QStringList categories() const;
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 76)
    /**
     * Returns the icon which accurately describes the category \p category.
     * This is meant to be used in a configuration dialog when showing
     * all the categories.
     *
     * By default this returns the icon of the AbstractRunner
     * * @deprecated Since 5.0, feature removed
     */
    KRUNNER_DEPRECATED_VERSION_BELATED(5, 76, 5, 0, "No longer use, feature removed")
    virtual QIcon categoryIcon(const QString &category) const;
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 81)
    /**
     * The nominal speed of the runner.
     * @see setSpeed
     */
    KRUNNER_DEPRECATED_VERSION(5, 81, "the concept of delayed runners is deprecated, see method docs of setSpeed(Speed) for details")
    Speed speed() const;
#endif

    /**
     * The priority of the runner.
     * @see setPriority
     */
    Priority priority() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
    /**
     * Returns the OR'ed value of all the Information types (as defined in RunnerContext::Type)
     * this runner is not interested in.
     * @return OR'ed value of black listed types
     * @deprecated This feature is deprecated
     */
    KRUNNER_DEPRECATED_VERSION(5, 76, "feature is deprecated")
    RunnerContext::Types ignoredTypes() const;
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
    /**
     * Sets the types this runner will ignore. If the value from RunnerContext::type() is contained in the ignored types
     * the match() method won't be called. This way there is no unnecessary thread spawned. The same RunnerContext from
     * which the type gets read is later passed into the match(Plasma::RunnerContext &context) method call.
     * @param types OR'ed listed of ignored types
     * @deprecated feature is deprecated. Consider using the minLetterCount and matchRegex properties instead. These
     * properties also prevent thread spawning, but can be used far more precise.
     * If you want to have this kind of optimization for older KRunner versions you could wrap this
     * inside of an version if statement:
     * @code
         #if KRUNNER_VERSION < QT_VERSION_CHECK(5, 76, 0)
         //set ignore types
         #endif
     * @endcode
     * The minLetterCount and matchRegex can be set with a similar version check or added to the desktop file.
     * If an older KRunner version loads such a desktop file these unknown properties are just ignored.
     * @see minLetterCount
     * @see matchRegex
     */
    KRUNNER_DEPRECATED_VERSION(5, 76, "feature is deprecated. Consider using the minLetterCount and matchRegex properties instead")
    void setIgnoredTypes(RunnerContext::Types types);
#endif

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

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
    /**
     * @return the default syntax for the runner or @c nullptr if no default syntax has been defined
     *
     * @since 4.4
     * @deprecated Since 5.76, feature is unused.
     */
    KRUNNER_DEPRECATED_VERSION(5, 76, "No longer use, feature is unused")
    RunnerSyntax *defaultSyntax() const;
#endif

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

    /**
     * Constructor for a KRunner plugin
     * @param parent parent object for this runner
     * @param pluginMetaData metadata that was embedded in the runner
     * @param args for compatibility with KPluginFactory, should be passed on to the parent constructor
     * @since 5.72
     */
    AbstractRunner(QObject *parent, const KPluginMetaData &pluginMetaData, const QVariantList &args);

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 77)
    KRUNNER_DEPRECATED_VERSION(5, 77, "use AbstractRunner(QObject *, const KPluginMetaData &, const QVariantList &)")
    explicit AbstractRunner(QObject *parent = nullptr, const QString &path = QString());
#else
    explicit AbstractRunner(QObject *parent = nullptr, const QString &path = QString()) = delete;
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 72)
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
    /// @deprecated Since 5.72, use AbstractRunner(const KPluginMetaData &, QObject *)
    KRUNNER_DEPRECATED_VERSION(5, 72, "use AbstractRunner(const KPluginMetaData &, QObject *)")
    explicit AbstractRunner(const KService::Ptr service, QObject *parent = nullptr);
#endif
#endif
#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 86)
    /// @since 5.72
    explicit AbstractRunner(const KPluginMetaData &pluginMetaData, QObject *parent = nullptr);
#endif
#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 77)
    /// @deprecated Since 5.77, use AbstractRunner(QObject *, const KPluginMetaData &, const QVariantList &)
    KRUNNER_DEPRECATED_VERSION(5, 77, "use AbstractRunner(QObject *, const KPluginMetaData &, const QVariantList &)")
    AbstractRunner(QObject *parent, const QVariantList &args);
#endif

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
    KRUNNER_DEPRECATED_VERSION_BELATED(5, 71, 5, 0, "No longer use, feature removed")
    void setHasRunOptions(bool hasRunOptions);
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 81)
    /**
     * Sets the nominal speed of the runner. Only slow runners need
     * to call this within their constructor because the default
     * speed is NormalSpeed. Runners that use D-Bus should call
     * this within their constructors.
     * @deprecated Since 5.81, the concept of delayed runners is deprecated.
     * If you have resource or memory intensive tasks consider porting the runner to a D-Bus runner.
     * Otherwise you can set the priority of the runner to LowPriority and implement the wait using a QTimer and an
     * event loop. It is important to check if the RunnerContext is still valid after the waiting interval.
     */
    KRUNNER_DEPRECATED_VERSION(5, 81, "the concept of delayed runners is deprecated, see method docs for porting instructions")
    void setSpeed(Speed newSpeed);
#endif

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

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 86)
    /**
     * Creates and then adds an action to the action registry.
     * AbstractRunner assumes ownership of the created action.
     *
     * @see QueryMatch::setActions
     * @see actionsForMatch
     *
     * @param id A unique identifier string
     * @param icon The icon to display
     * @param text The text to display
     * @return the created QAction
     * @deprecated Since 5.86 create the QAction instance manually
     * @code
     * // in the runner class definition
     * QList<QAction *> m_actions;
     * // when initializing the runner, optionally set the date if needed
     * auto action = new QAction(QIcon::fromTheme(iconName), text, this);
     * m_actions << action;
     * // when creating the match
     * match.setActions(m_actions);
     * @endcode
     */
    KRUNNER_DEPRECATED_VERSION(5, 86, "create the QAction instance manually")
    QAction *addAction(const QString &id, const QIcon &icon, const QString &text);

    /**
     * Adds an action to the runner's action registry.
     *
     * The QAction must be created within the GUI thread;
     * do not create it within the match method of AbstractRunner.
     *
     * @param id A unique identifier string
     * @param action The QAction to be stored
     * @deprecated Since 5.86, create the QAction instance manually
     */
    KRUNNER_DEPRECATED_VERSION(5, 86, "create the QAction instance manually")
    void addAction(const QString &id, QAction *action);

    /**
     * Removes the action from the action registry.
     * AbstractRunner deletes the action once removed.
     *
     * @param id The id of the action to be removed
     * @deprecated Since 5.86, deprecated for lack of usage
     */
    KRUNNER_DEPRECATED_VERSION(5, 86, "deprecated for lack of usage")
    void removeAction(const QString &id);

    /**
     * Returns the action associated with the id
     * @deprecated Since 5.86, create the QAction instances manually and store them in a custom container instead
     */
    KRUNNER_DEPRECATED_VERSION(5, 86, "create the QAction instances manually and store them in a custom container instead")
    QAction *action(const QString &id) const;

    /**
     * Returns all registered actions
     * @deprecated Since 5.86, create the QAction instances manually and store them in a custom container instead
     */
    KRUNNER_DEPRECATED_VERSION(5, 86, "create the QAction instances manually and store them in a custom container instead")
    QHash<QString, QAction *> actions() const;

    /**
     * Clears the action registry.
     * The action pool deletes the actions.
     * @deprecated Since 5.86, use a custom container to store the QAction instances instead
     */
    KRUNNER_DEPRECATED_VERSION(5, 86, "use a custom container to store the QAction instances instead")
    void clearActions();
#endif

    /**
     * Adds a registered syntax that this runner understands. This is used to
     * display to the user what this runner can understand and how it can be
     * used.
     *
     * @param syntax the syntax to register
     * @since 4.3
     */
    void addSyntax(const RunnerSyntax &syntax);

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
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
     * @deprecated Since 5.76, feature is unused. Use addSyntax() instead.
     **/
    KRUNNER_DEPRECATED_VERSION(5, 76, "Feature unused, use addSyntax() instead")
    void setDefaultSyntax(const RunnerSyntax &syntax);
#endif

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
    std::unique_ptr<AbstractRunnerPrivate> const d;
    bool hasUniqueResults();
    bool hasWeakResults();
    friend class RunnerContext;
    friend class RunnerContextPrivate;
    friend class QueryMatch;
};

} // Plasma namespace

// clang-format off
#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 72)
// Boilerplate to emit a version-controlled warning about the deprecated macro at least with GCC
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
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();)
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 88)
/**
 * @relates Plasma::AbstractRunner
 *
 * Registers a runner plugin with JSON metadata.
 *
 * @param classname name of the AbstractRunner derived class.
 * @param jsonFile name of the JSON file to be compiled into the plugin as metadata
 *
 * @since 5.72
 * @deprecated Since 5.88 use K_PLUGIN_CLASS_WITH_JSON instead
 */
#define K_EXPORT_PLASMA_RUNNER_WITH_JSON(classname, jsonFile) \
    K_PLUGIN_FACTORY_WITH_JSON(classname ## Factory, jsonFile, registerPlugin<classname>();)
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 75)
/**
 * These plugins are Used by the plugin selector dialog to show
 * configuration options specific to this runner. These options
 * must be runner global and not pertain to a specific match.
 * @deprecated Since 5.0, use K_PLUGIN_FACTORY directly
 */
#define K_EXPORT_RUNNER_CONFIG( name, classname )     \
K_PLUGIN_FACTORY(ConfigFactory, registerPlugin<classname>();)
#endif
// clang-format on

#if !KRUNNER_ENABLE_DEPRECATED_SINCE(5, 91)
namespace KRunner
{
using AbstractRunner = Plasma::AbstractRunner;
using RunnerContext = Plasma::RunnerContext;
}
#endif

#endif
