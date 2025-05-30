/*
 * SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 */

#ifndef KRUNNER_RESULTSMODEL
#define KRUNNER_RESULTSMODEL

#include "krunner_export.h"

#include <KRunner/RunnerManager>
#include <QIcon>
#include <QSortFilterProxyModel>
#include <memory>

namespace KRunner
{
class ResultsModelPrivate;

/*!
 * \class KRunner::ResultsModel
 * \inheaderfile KRunner/ResultsModel
 * \inmodule KRunner
 *
 * \brief A model that exposes and sorts results for a given query.
 *
 * \since 6.0
 */
class KRUNNER_EXPORT ResultsModel : public QSortFilterProxyModel
{
    Q_OBJECT

    /*!
     * \property KRunner::ResultsModel::queryString
     *
     * The query string to run
     */
    Q_PROPERTY(QString queryString READ queryString WRITE setQueryString NOTIFY queryStringChanged)
    /*!
     * \property KRunner::ResultsModel::limit
     *
     * The preferred maximum number of matches in the model
     *
     * If there are lots of results from different categories,
     * the limit can be slightly exceeded.
     *
     * Default is 0, which means no limit.
     */
    Q_PROPERTY(int limit READ limit WRITE setLimit RESET resetLimit NOTIFY limitChanged)
    /*!
     * \property KRunner::ResultsModel::querying
     *
     * Whether the query is currently being run
     *
     * This can be used to show a busy indicator
     */
    Q_PROPERTY(bool querying READ querying NOTIFY queryingChanged)

    /*!
     * \property KRunner::ResultsModel::singleRunner
     *
     * The single runner to use for querying in single runner mode
     *
     * Defaults to empty string which means all runners
     */
    Q_PROPERTY(QString singleRunner READ singleRunner WRITE setSingleRunner NOTIFY singleRunnerChanged)

    /*!
     * \property KRunner::ResultsModel::singleRunnerMetaData
     */
    Q_PROPERTY(KPluginMetaData singleRunnerMetaData READ singleRunnerMetaData NOTIFY singleRunnerChanged)

    /*!
     * \property KRunner::ResultsModel::runnerManager
     */
    Q_PROPERTY(KRunner::RunnerManager *runnerManager READ runnerManager WRITE setRunnerManager NOTIFY runnerManagerChanged)

    /*!
     * \property KRunner::ResultsModel::favoriteIds
     */
    Q_PROPERTY(QStringList favoriteIds READ favoriteIds WRITE setFavoriteIds NOTIFY favoriteIdsChanged)

public:
    /*!
     *
     */
    explicit ResultsModel(const KConfigGroup &configGroup, const KConfigGroup &stateConfigGroup, QObject *parent = nullptr);

    /*!
     *
     */
    explicit ResultsModel(QObject *parent = nullptr);
    ~ResultsModel() override;

    /*!
     * \value IdRole
     * \value CategoryRelevanceRole
     * \value RelevanceRole
     * \value EnabledRole
     * \value CategoryRole
     * \value SubtextRole
     * \value ActionsRole
     * \value MultiLineRole
     * \value UrlsRole
     * \omitvalue QueryMatchRole
     * \omitvalue FavoriteIndexRole
     * \omitvalue FavoriteCountRole
     */
    enum Roles {
        IdRole = Qt::UserRole + 1,
        CategoryRelevanceRole,
        RelevanceRole,
        EnabledRole,
        CategoryRole,
        SubtextRole,
        ActionsRole,
        MultiLineRole,
        UrlsRole,
        QueryMatchRole,
        FavoriteIndexRole,
        FavoriteCountRole,
    };
    Q_ENUM(Roles)

    QString queryString() const;
    void setQueryString(const QString &queryString);
    Q_SIGNAL void queryStringChanged(const QString &queryString);

    /*!
     * IDs of favorite plugins. Those plugins are always in a fixed order before the other ones.
     *
     * \a ids KPluginMetaData::pluginId values of plugins
     */
    void setFavoriteIds(const QStringList &ids);
    QStringList favoriteIds() const;
    Q_SIGNAL void favoriteIdsChanged();

    int limit() const;
    void setLimit(int limit);
    void resetLimit();
    Q_SIGNAL void limitChanged();

    bool querying() const;
    Q_SIGNAL void queryingChanged();

    QString singleRunner() const;
    void setSingleRunner(const QString &runner);
    Q_SIGNAL void singleRunnerChanged();

    KPluginMetaData singleRunnerMetaData() const;

    QHash<int, QByteArray> roleNames() const override;

    /*!
     * Clears the model content and resets the runner context, i.e. no new items will appear.
     */
    Q_INVOKABLE void clear();

    /*!
     * Run the result at the given model index \a idx
     */
    Q_INVOKABLE bool run(const QModelIndex &idx);
    /*!
     * Run the action \a actionNumber at given model index \a idx
     */
    Q_INVOKABLE bool runAction(const QModelIndex &idx, int actionNumber);

    /*!
     * Get mime data for the result at given model index \a idx
     */
    Q_INVOKABLE QMimeData *getMimeData(const QModelIndex &idx) const;

    /*!
     * Get match for the result at given model index \a idx
     */
    KRunner::QueryMatch getQueryMatch(const QModelIndex &idx) const;

    KRunner::RunnerManager *runnerManager() const;
    /*!
     * \since 6.9
     */
    void setRunnerManager(KRunner::RunnerManager *manager);
    /*!
     * \since 6.9
     */
    Q_SIGNAL void runnerManagerChanged();

Q_SIGNALS:
    /*!
     * This signal is emitted when a an InformationalMatch is run, and it is advised
     * to update the search term, e.g. used for calculator runner results
     */
    void queryStringChangeRequested(const QString &queryString, int pos);

private:
    const std::unique_ptr<ResultsModelPrivate> d;
};

} // namespace KRunner
#endif
