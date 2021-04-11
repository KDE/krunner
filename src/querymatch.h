/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_QUERYMATCH_H
#define PLASMA_QUERYMATCH_H

#include <QList>
#include <QSharedDataPointer>
#include <QUrl>

#include "krunner_export.h"

class QAction;
class QIcon;
class QString;
class QVariant;
class QWidget;

namespace Plasma
{
class RunnerContext;
class AbstractRunner;
class QueryMatchPrivate;

/**
 * @class QueryMatch querymatch.h <KRunner/QueryMatch>
 *
 * @short A match returned by an AbstractRunner in response to a given
 * RunnerContext.
 */
class KRUNNER_EXPORT QueryMatch
{
public:
    /**
     * The type of match. Value is important here as it is used for sorting
     */
    enum Type {
        NoMatch = 0, /**< Null match */
        CompletionMatch = 10, /**< Possible completion for the data of the query */
        PossibleMatch = 30, /**< Something that may match the query */
        InformationalMatch = 50, /**< A purely informational, non-runnable match,
                                   such as the answer to a question or calculation.
                                   The data of the match will be converted to a string
                                   and set in the search field */
        HelperMatch = 70, /**< A match that represents an action not directly related
                             to activating the given search term, such as a search
                             in an external tool or a command learning trigger. Helper
                             matches tend to be generic to the query and should not
                             be autoactivated just because the user hits "Enter"
                             while typing. They must be explicitly selected to
                             be activated, but unlike InformationalMatch cause
                             an action to be triggered. */
        ExactMatch = 100, /**< An exact match to the query */
    };

    /**
     * Constructs a PossibleMatch associated with a given RunnerContext
     * and runner.
     *
     * @param runner the runner this match belongs to
     */
    explicit QueryMatch(AbstractRunner *runner = nullptr);

    /**
     * Copy constructor
     */
    QueryMatch(const QueryMatch &other);

    ~QueryMatch();
    QueryMatch &operator=(const QueryMatch &other);
    bool operator==(const QueryMatch &other) const;
    bool operator!=(const QueryMatch &other) const;
    bool operator<(const QueryMatch &other) const;

    /**
     * @return the runner associated with this action
     */
    AbstractRunner *runner() const;

    /**
     * Requests this match to activae using the given context
     *
     * @param context the context to use in conjunction with this run
     *
     * @sa AbstractRunner::run
     */
    void run(const RunnerContext &context) const;

    /**
     * @return true if the match is valid and can therefore be run,
     *         an invalid match does not have an associated AbstractRunner
     */
    bool isValid() const;

    /**
     * Sets the type of match this action represents.
     */
    void setType(Type type);

    /**
     * The type of action this is. Defaults to PossibleMatch.
     */
    Type type() const;

    /**
     * Sets information about the type of the match which can
     * be used to categorize the match.
     *
     * This string should be translated as it can be displayed
     * in an UI
     */
    void setMatchCategory(const QString &category);

    /**
     * Extra information about the match which can be used
     * to categorize the type.
     *
     * By default this returns the internal name of the runner
     * which returned this result
     */
    QString matchCategory() const;

    /**
     * Sets the relevance of this action for the search
     * it was created for.
     *
     * @param relevance a number between 0 and 1.
     */
    void setRelevance(qreal relevance);

    /**
     * The relevance of this action to the search. By default,
     * the relevance is 1.
     *
     * @return a number between 0 and 1
     */
    qreal relevance() const;

    /**
     * Sets data to be used internally by the associated
     * AbstractRunner.
     *
     * When set, it is also used to form
     * part of the id() for this match. If that is inappropriate
     * as an id, the runner may generate its own id and set that
     * with setId(const QString&) directly after calling setData
     */
    void setData(const QVariant &data);

    /**
     * @return the data associated with this match; usually runner-specific
     */
    QVariant data() const;

    /**
     * Sets the id for this match; useful if the id does not
     * match data().toString(). The id must be unique to all
     * matches from this runner, and should remain constant
     * for the same query for best results.
     *
     * @param id the new identifying string to use to refer
     *           to this entry
     */
    void setId(const QString &id);

    /**
     * @return a string that can be used as an ID for this match,
     * even between different queries. It is based in part
     * on the source of the match (the AbstractRunner) and
     * distinguishing information provided by the runner,
     * ensuring global uniqueness as well as consistency
     * between query matches.
     */
    QString id() const;

    /**
     * Sets the main title text for this match; should be short
     * enough to fit nicely on one line in a user interface
     *
     * @param text the text to use as the title
     */
    void setText(const QString &text);

    /**
     * @return the title text for this match
     */
    QString text() const;

    /**
     * Sets the descriptive text for this match; can be longer
     * than the main title text
     *
     * @param text the text to use as the description
     */
    void setSubtext(const QString &text);

    /**
     * @return the descriptive text for this match
     */
    QString subtext() const;

    /**
     * Sets the icon associated with this match
     *
     * Prefer using setIconName.
     *
     * @param icon the icon to show along with the match
     */
    void setIcon(const QIcon &icon);

    /**
     * @return the icon for this match
     */
    QIcon icon() const;

    /**
     * Sets the icon name associated with this match
     *
     * @param icon the name of the icon to show along with the match
     * @since 5.24
     */
    void setIconName(const QString &iconName);

    /**
     * @return the name of the icon for this match
     * @since 5.24
     */
    QString iconName() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 82)
    /**
     * Sets the MimeType, if any, associated with this match.
     * This overrides the MimeType provided by QueryContext, and should only be
     * set when it is different from the QueryContext MimeType
     * @deprecated Since 5.82, deprecated for lack of usage
     */
    KRUNNER_DEPRECATED_VERSION(5, 82, "deprecated for lack of usage")
    void setMimeType(const QString &mimeType);

    /**
     * @return the mimtype for this match, or QString() is none
     * @deprecated Since 5.82, deprecated for lack of usage
     */
    KRUNNER_DEPRECATED_VERSION(5, 82, "deprecated for lack of usage")
    QString mimeType() const;
#endif

    /**
     * Sets the urls, if any, associated with this match
     */
    void setUrls(const QList<QUrl> &urls);

    /**
     * @return the mimtype for this match, or QString() is none
     */
    QList<QUrl> urls() const;

    /**
     * Sets whether or not this match can be activited
     *
     * @param enable true if the match is enabled and therefore runnable
     */
    void setEnabled(bool enable);

    /**
     * @return true if the match is enabled and therefore runnable, otherwise false
     */
    bool isEnabled() const;

    /**
     * Set the actions for this match.
     * This method allows you to set the actions inside of the AbstractRunner::match() method
     * and the default implementation of AbstractRunner::actionsForMatch() will return these.
     * @since 5.75
     */
    void setActions(const QList<QAction *> &actions);

    /**
     * Adds an action to this match
     * @since 5.75
     * @see setActions
     */
    void addAction(QAction *actions);

    /**
     * List of actions set for this match
     * @return actions
     * @since 5.75
     */
    QList<QAction *> actions() const;

    /**
     * The current action.
     */
    QAction *selectedAction() const;

    /**
     * Sets the selected action
     */
    void setSelectedAction(QAction *action);

    /**
     * Set if the text should be displayed as a multiLine string
     * @param multiLine
     * @since 5.82
     */
    void setMultiLine(bool multiLine);

    /**
     * If the text should be displayed as a multiLine string
     * If no explicit value is set set using setMultiline it will default to false
     * @return bool
     * @since 5.82
     */
    bool isMultiLine() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 71)
    /**
     * @return true if this match can be configured before being run
     * @since 4.3
     * @deprecated Since 5.0, this feature has been defunct
     */
    KRUNNER_DEPRECATED_VERSION_BELATED(5, 71, 5, 0, "No longer use, feature removed")
    bool hasConfigurationInterface() const;
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 71)
    /**
     * If hasConfigurationInterface() returns true, this method may be called to get
     * a widget displaying the options the user can interact with to modify
     * the behaviour of what happens when the match is run.
     *
     * @param widget the parent of the options widgets.
     * @since 4.3
     * @deprecated Since 5.0, this feature has been defunct
     */
    KRUNNER_DEPRECATED_VERSION_BELATED(5, 71, 5, 0, "No longer use, feature removed")
    void createConfigurationInterface(QWidget *parent);
#endif

private:
    QSharedDataPointer<QueryMatchPrivate> d;
};

}

#endif
