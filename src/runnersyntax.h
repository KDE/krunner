/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_RUNNERSYNTAX_H
#define PLASMA_RUNNERSYNTAX_H

#include <QStringList>
#include <memory>

#include "krunner_export.h"

namespace Plasma
{
class RunnerSyntaxPrivate;
/**
 * @class RunnerSyntax runnersyntax.h <KRunner/RunnerSyntax>
 * @since 4.3
 *
 * Represents a query prototype that the runner accepts. These can be
 * created and registered with AbstractRunner::addSyntax(Syntax &) to
 * allow applications to show to the user what the runner is currently
 * capable of doing.
 *
 * Lets say the runner has a trigger word and then the user can type anything after that. In that case you could use
 * ":q:" as a placeholder, which will get expanded to i18n("search term") and be put in brackets.
 * @code
   Plasma::RunnerSyntax syntax(QStringLiteral("sometriggerword :q:"), i18n("Description for this syntax"));
   addSyntax(syntax);
 * @endcode
 *
 * But if the query the user has to enter is sth. specific like a program,
 * url or file you should use a custom placeholder to make it easier to understand.
 * @code
   Plasma::RunnerSyntax syntax(QStringLiteral("sometriggereword <%1>").arg(i18n("program name"))), i18n("Description for this syntax"));
   addSyntax(syntax);
 * @endcode
 */
class KRUNNER_EXPORT RunnerSyntax
{
public:
    /**
     * Constructs a simple syntax object
     *
     * @param exampleQuery See the class description for examples and placeholder conventions.
     * @param description A description of what the described syntax does from
     *                    the user's point of view.
     */
    RunnerSyntax(const QString &exampleQuery, const QString &description);

    /**
     * Copy constructor
     */
    RunnerSyntax(const RunnerSyntax &other);

    ~RunnerSyntax();

    /**
     * Assignment operator
     */
    RunnerSyntax &operator=(const RunnerSyntax &rhs);

    /**
     * Adds a synonymous example query to this Syntax. Some runners may
     * accept multiple formulations of keywords to trigger the same behaviour.
     * This allows the runner to show these relationships by grouping the
     * example queries into one Syntax object
     *
     * @param exampleQuery See the class description for examples and placeholder conventions.
     */
    void addExampleQuery(const QString &exampleQuery);

    /**
     * @return the example queries associated with this Syntax object
     */
    QStringList exampleQueries() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
    /**
     * @return the example queries associated with this Syntax object, with
     * the searchTermDescription replacing instances of :q:. Used for showing
     * the queries in the user interface.
     * @deprecated Since 5.76, the description should be directly set when creating the example query.
     * To display the queries in the user interface. Use exampleQueries() instead.
     */
    KRUNNER_DEPRECATED_VERSION(5, 76, "The descriptions should be directly set when creating the example query. Use exampleQueries() instead.")
    QStringList exampleQueriesWithTermDescription() const;
#endif

    /**
     * Sets the description for the syntax, describing what it does from
     * the user's point of view.
     */
    void setDescription(const QString &description);

    /**
     * @return the description of what the syntax does from the user's
     *         point of view
     */
    QString description() const;

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
    /**
     * Sets the text that should be used to replace instances of :q:
     * in the text. By default this is the generic phrase "search term".
     * If the syntax expects a specific kind of input, it may be defined
     * here. A syntax used by a runner that changes the brightness of the display
     * may set this to "brightness" for instance.
     * @deprecated Since 5.76, set the description directly when creating the example query. Use <my query description> instead of :q: when creating the string
     */
    KRUNNER_DEPRECATED_VERSION(
        5,
        76,
        "Set the description directly when creating the example query. Use <my query description> instead of :q: when creating the string")
    void setSearchTermDescription(const QString &description);
#endif

#if KRUNNER_ENABLE_DEPRECATED_SINCE(5, 76)
    /**
     * @return a description of the search term for this syntax
     * @deprecated Since 5.76, the description should be directly set when creating the example query.
     */
    KRUNNER_DEPRECATED_VERSION(5, 76, "Feature is obsolete, the search term description should be set inside of the example query directly")
    QString searchTermDescription() const;
#endif

private:
    std::unique_ptr<RunnerSyntaxPrivate> const d;
};

} // namespace Plasma

#if !KRUNNER_ENABLE_DEPRECATED_SINCE(5, 91)
namespace KRunner
{
using RunnerSyntax = Plasma::RunnerSyntax;
}
#endif

#endif // multiple inclusion guard
