/*
 * SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 */

#pragma once

#include <KRunner/RunnerManager>
#include <QIcon>
#include <QScopedPointer>
#include <QSortFilterProxyModel>

namespace KRunner
{
class ResultsModel : public QSortFilterProxyModel
{
    Q_OBJECT

    /**
     * The query string to run
     */
    Q_PROPERTY(QString queryString READ queryString WRITE setQueryString NOTIFY queryStringChanged)
    /**
     * The preferred maximum number of matches in the model
     *
     * If there are lots of results from different catergories,
     * the limit can be slightly exceeded.
     *
     * Default is 0, which means no limit.
     */
    Q_PROPERTY(int limit READ limit WRITE setLimit RESET resetLimit NOTIFY limitChanged)
    /**
     * Whether the query is currently being run
     *
     * This can be used to show a busy indicator
     */
    Q_PROPERTY(bool querying READ querying NOTIFY queryingChanged)

    /**
     * The single runner to use for querying in single runner mode
     *
     * Defaults to empty string which means all runners
     */
    Q_PROPERTY(QString runner READ runner WRITE setRunner NOTIFY runnerChanged)
    // FIXME rename to singleModeRunnerName or something
    Q_PROPERTY(QString runnerName READ runnerName NOTIFY runnerChanged)
    Q_PROPERTY(QIcon runnerIcon READ runnerIcon NOTIFY runnerChanged)
    Q_PROPERTY(KRunner::RunnerManager *runnerManager READ runnerManager CONSTANT)

public:
    explicit ResultsModel(QObject *parent = nullptr);
    ~ResultsModel() override;

    enum Roles {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        RelevanceRole,
        EnabledRole,
        CategoryRole,
        SubtextRole,
        DuplicateRole,
        ActionsRole,
        MultiLineRole,
    };
    Q_ENUM(Roles)

    QString queryString() const;
    void setQueryString(const QString &queryString);
    Q_SIGNAL void queryStringChanged(const QString &queryString);

    int limit() const;
    void setLimit(int limit);
    void resetLimit();
    Q_SIGNAL void limitChanged();

    bool querying() const;
    Q_SIGNAL void queryingChanged();

    QString runner() const;
    void setRunner(const QString &runner);
    Q_SIGNAL void runnerChanged();

    QString runnerName() const;
    QIcon runnerIcon() const;

    QHash<int, QByteArray> roleNames() const override;

    /**
     * Clears the model content and resets the runner context, i.e. no new items will appear.
     */
    Q_INVOKABLE void clear();

    /**
     * Run the result at the given model index @p idx
     */
    Q_INVOKABLE bool run(const QModelIndex &idx);
    /**
     * Run the action @p actionNumber at given model index @p idx
     */
    Q_INVOKABLE bool runAction(const QModelIndex &idx, int actionNumber);

    /**
     * Get mime data for the result at given model index @p idx
     */
    Q_INVOKABLE QMimeData *getMimeData(const QModelIndex &idx) const;

    KRunner::RunnerManager *runnerManager() const;

Q_SIGNALS:
    /**
     * This signal is emitted when a an InformationalMatch is run, and it is advised
     * to update the search term, e.g. used for calculator runner results
     */
    void queryStringChangeRequested(const QString &queryString, int pos);

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace Milou
