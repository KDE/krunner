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
            FileSystem = Directory | File | Executable | ShellCommand
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

        ~RunnerContext();

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
         * The type of item the search term might refer to.
         * @see Type
         */
        Type type() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
        /**
         * A list of categories of which results should be returned.
         * This list is typically populated from the AbstractRunner::categories
         * function.
         * @deprecated Since 5.76, feature is unused and not supported by most runners
         */
        KRUNNER_DEPRECATED_VERSION(5, 76, "feature is unused and not supported by most runners")
        QStringList enabledCategories() const;
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
        /**
         * Sets the list of enabled categories. Runners can use this list
         * to optimize themselves by only returning results from the enabled
         * categories
         * @deprecated Since 5.76, feature is unused and not supported by most runners
         */
        KRUNNER_DEPRECATED_VERSION(5, 76, "feature is unused and not supported by most runners")
        void setEnabledCategories(const QStringList &categories);
#endif

        /**
         * The mimetype that the search term refers to, if discoverable.
         *
         * @return QString() if the mimetype can not be determined, otherwise
         *         the mimetype of the object being referred to by the search
         *         string.
         */
        QString mimeType() const;

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
         * Removes a match from the existing list of matches.
         *
         * If you are going to be removing multiple matches, use removeMatches instead.
         *
         * @param matchId the id of match to remove
         *
         * @return true if the match was removed, false otherwise.
         * @since 4.4
         */
        bool removeMatch(const QString matchId);

        /**
         * Removes lists of matches from the existing list of matches.
         *
         * This method is thread safe and causes the matchesChanged() signal to be emitted.
         *
         * @param matchIdList the list of matches id to remove
         *
         * @return true if at least one match was removed, false otherwise.
         * @since 4.4
         */
        bool removeMatches(const QStringList matchIdList);

        /**
         * Removes lists of matches from a given AbstractRunner
         *
         * This method is thread safe and causes the matchesChanged() signal to be emitted.
         *
         * @param runner the AbstractRunner from which to remove matches
         *
         * @return true if at least one match was removed, false otherwise.
         * @since 4.10
         */
        bool removeMatches(AbstractRunner *runner);

        /**
         * Retrieves all available matches for the current search term.
         *
         * @return a list of matches
         */
        QList<QueryMatch> matches() const;

        /**
         * Retrieves a match by id.
         *
         * @param id the id of the match to return
         * @return the match associated with this id, or an invalid QueryMatch object
         *         if the id does not exist
         */
        QueryMatch match(const QString &id) const;

        /**
         * Sets single runner query mode. Note that a call to reset() will
         * turn off single runner query mode.
         *
         * @see reset()
         * @since 4.4
         */
         void setSingleRunnerQueryMode(bool enabled);

        /**
         * @return true if the current query is a single runner query
         * @since 4.4
         */
        bool singleRunnerQueryMode() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
        /**
         * Sets the launch counts for the associated match ids
         *
         * If a runner adds a match to this context, the context will check if the
         * match id has been launched before and increase the matches relevance
         * correspondingly. In this manner, any front end can implement adaptive search
         * by sorting items according to relevance.
         *
         * @param config the config group where launch data was stored
         * @deprecated Since 5.76, The launch counts are now directly set and only read inside of this class.
         * Use setLaunchCounts() if you want to load a different value than the default one set by the RunnerManager
         */
    KRUNNER_DEPRECATED_VERSION(5, 76, "The launch counts are now directly set and only read inside of this class."
                                      "Use setLaunchCounts() if you want to load a different value than the default one set by the RunnerManager")
    void restore(const KConfigGroup &config);
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
        /**
         * @param config the config group where launch data should be stored
         * @deprecated Since 5.76, The launch counts are now directly set and only read inside of this class.
         * Use setLaunchCounts() if you want to load a different value than the default one set by the RunnerManager
         */
        KRUNNER_DEPRECATED_VERSION(5, 76, "The launch counts are now directly set and only read inside of this class.")
        void save(KConfigGroup &config);
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
        /**
         * Run a match using the information from this context
         *
         * The context will also keep track of the number of times the match was
         * launched to sort future matches according to user habits
         *
         * @param match the match to run
         */
        KRUNNER_DEPRECATED_VERSION(5, 76, "The launch counts are now directly set and only read inside of this class.")
        void run(const QueryMatch &match);
#endif

        void setLaunchCounts(const QHash<QString, QHash<QString, int>> &launchCounts);

    Q_SIGNALS:
        void matchesChanged();

    private:
        QExplicitlySharedDataPointer<RunnerContextPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RunnerContext::Types)

}

#endif
