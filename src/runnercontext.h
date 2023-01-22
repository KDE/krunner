/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_RUNNERCONTEXT_H
#define PLASMA_RUNNERCONTEXT_H

#include <QList>
#include <QObject>
#include <QSharedDataPointer>

#include "krunner_export.h"

class KConfigGroup;

namespace Plasma
{
class QueryMatch;
class AbstractRunner;
class RunnerContextPrivate;

/**
 * @class RunnerContext runnercontext.h <KRunner/RunnerContext>
 *
 * @short The RunnerContext class provides information related to a search,
 *        including the search term, metadata on the search term and collected
 *        matches.
 */
class KRUNNER_EXPORT RunnerContext : public QObject
{
    Q_OBJECT

public:
    enum Type {
        None = 0,
        UnknownType = 1,
        Directory = 2,
        File = 4,
        NetworkLocation = 8,
        Executable = 16,
        ShellCommand = 32,
        Help = 64,
        FileSystem = Directory | File | Executable | ShellCommand,
    };

    Q_DECLARE_FLAGS(Types, Type)

    explicit RunnerContext(QObject *parent = nullptr);

    /**
     * Copy constructor
     */
    RunnerContext(RunnerContext &other, QObject *parent = nullptr);

    /**
     * Assignment operator
     * @since 4.4
     */
    RunnerContext &operator=(const RunnerContext &other);

    ~RunnerContext() override;

    /**
     * Resets the search term for this object.
     * This removes all current matches in the process and
     * turns off single runner query mode.
     */
    void reset();

    /**
     * Sets the query term for this object and attempts to determine
     * the type of the search.
     */
    void setQuery(const QString &term);

    /**
     * @return the current search query term.
     */
    QString query() const;

    /**
     * @returns true if this context is no longer valid and therefore
     * matching using it should abort. Most useful as an optimization technique
     * inside of AbstractRunner subclasses in the match method, e.g.:
     *
     * while (.. a possibly large iteration) {
     *     if (!context.isValid()) {
     *         return;
     *     }
     *
     *     ... some processing ...
     * }
     *
     * While not required to be used within runners, it provides a nice way
     * to avoid unnecessary processing in runners that may run for an extended
     * period (as measured in 10s of ms) and therefore improve the user experience.
     * @since 4.2.3
     */
    bool isValid() const;

    /**
     * Appends lists of matches to the list of matches.
     *
     * @param matches the matches to add
     * @return true if matches were added, false if matches were e.g. outdated
     */
    bool addMatches(const QList<QueryMatch> &matches);

    /**
     * Appends a match to the existing list of matches.
     *
     * If you are going to be adding multiple matches, use @see addMatches instead.
     *
     * @param match the match to add
     * @return true if the match was added, false otherwise.
     */
    bool addMatch(const QueryMatch &match);

    /**
     * Retrieves all available matches for the current search term.
     *
     * @return a list of matches
     */
    QList<QueryMatch> matches() const;

    /**
     * Request that KRunner updates the query string and stasy open, even after running a match.
     * This method is const so it can be called in a const context.
     *
     * @param text Text that will be displayed in the search field
     * @param cursorPosition Position of the cursor, if this is different than the length of the text,
     * the characters between the position and text will be selected
     *
     * @since 5.90
     */
    void requestQueryStringUpdate(const QString &text, int cursorPosition) const;

    /**
     * Sets single runner query mode. Note that a call to reset() will
     * turn off single runner query mode.
     *
     * @note This method is considered internal. To set the single runner mode you should pass in a runnerId to @ref RunnerManager::launchQuery
     * @see reset()
     * @internal
     * @since 4.4
     */
    // TODO KF6 Make private
    void setSingleRunnerQueryMode(bool enabled);

    /**
     * @return true if the current query is a single runner query
     * @since 4.4
     */
    bool singleRunnerQueryMode() const;

    /**
     * Set this to true in the AbstractRunner::run method to prevent the entry
     * from being saved to the history.
     * @since 5.90
     */
    void ignoreCurrentMatchForHistory() const;

    /**
     * Sets the launch counts for the associated match ids
     *
     * If a runner adds a match to this context, the context will check if the
     * match id has been launched before and increase the matches relevance
     * correspondingly. In this manner, any front end can implement adaptive search
     * by sorting items according to relevance.
     *
     * @param config the config group where launch data was stored
     */
    void restore(const KConfigGroup &config);

    /**
     * @param config the config group where launch data should be stored
     */
    void save(KConfigGroup &config);

    /**
     * Run a match using the information from this context
     *
     * The context will also keep track of the number of times the match was
     * launched to sort future matches according to user habits
     *
     * @param match the match to run
     */
    void run(const QueryMatch &match);

Q_SIGNALS:
    void matchesChanged();

private:
    QString requestedQueryString() const;
    int requestedCursorPosition() const;
    bool shouldIgnoreCurrentMatchForHistory() const;
    friend class RunnerManager;
    QExplicitlySharedDataPointer<RunnerContextPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RunnerContext::Types)

}

#endif
