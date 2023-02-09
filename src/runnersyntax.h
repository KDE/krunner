/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KRUNNER_RUNNERSYNTAX_H
#define KRUNNER_RUNNERSYNTAX_H

#include <QStringList>
#include <memory>

#include "krunner_export.h"

namespace KRunner
{
class RunnerSyntaxPrivate;
/**
 * @class RunnerSyntax runnersyntax.h <KRunner/RunnerSyntax>
 *
 * Represents a query prototype that the runner accepts. These can be
 * created and registered with AbstractRunner::addSyntax(RunnerSyntax) to
 * allow applications to show to the user what the runner is currently
 * capable of doing.
 *
 * Lets say the runner has a trigger word and then the user can type anything after that. In that case you could use
 * ":q:" as a placeholder, which will get expanded to i18n("search term") and be put in brackets.
 * @code
   KRunner::RunnerSyntax syntax(QStringLiteral("sometriggerword :q:"), i18n("Description for this syntax"));
   addSyntax(syntax);
 * @endcode
 *
 * But if the query the user has to enter is sth. specific like a program,
 * url or file you should use a custom placeholder to make it easier to understand.
 * @code
   KRunner::RunnerSyntax syntax(QStringLiteral("sometriggereword <%1>").arg(i18n("program name"))), i18n("Description for this syntax"));
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
    explicit RunnerSyntax(const QString &exampleQuery, const QString &description);

    /**
     * Copy constructor
     */
    explicit RunnerSyntax(const RunnerSyntax &other);

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

    /**
     * Sets the user-visible description for the syntax, describing what it does from
     * the user's point of view.
     */
    void setDescription(const QString &description);

    /**
     * @return the user visible description of what the syntax does
     */
    QString description() const;
private:
    std::unique_ptr<RunnerSyntaxPrivate> const d;
};

} // namespace KRunner

#endif // multiple inclusion guard
