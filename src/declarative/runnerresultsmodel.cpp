/*
 * SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 */

#include "runnerresultsmodel.h"

#include <QAction>
#include <QSet>

#include <KRunner/RunnerManager>

#include "resultsmodel.h"

namespace KRunner
{
RunnerResultsModel::RunnerResultsModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_manager(new RunnerManager(QStringLiteral("krunnerrc"), this))
{
    connect(m_manager, &RunnerManager::matchesChanged, this, &RunnerResultsModel::onMatchesChanged);
    connect(m_manager, &RunnerManager::queryFinished, this, [this] {
        setQuerying(false);
    });
    connect(m_manager, &RunnerManager::requestUpdateQueryString, this, &RunnerResultsModel::queryStringChangeRequested);
}

RunnerResultsModel::~RunnerResultsModel() = default;

KRunner::QueryMatch RunnerResultsModel::fetchMatch(const QModelIndex &idx) const
{
    const QString category = m_categories.value(int(idx.internalId() - 1));
    return m_matches.value(category).value(idx.row());
}

void RunnerResultsModel::onMatchesChanged(const QList<KRunner::QueryMatch> &matches)
{
    // Build the list of new categories and matches
    QSet<QString> newCategories;
    // here we use QString as key since at this point we don't care about the order
    // of categories but just what matches we have for each one.
    // Below when we populate the actual m_matches we'll make sure to keep the order
    // of existing categories to avoid pointless model changes.
    QHash<QString /*category*/, QVector<KRunner::QueryMatch>> newMatches;
    for (const auto &match : matches) {
        const QString category = match.matchCategory();
        newCategories.insert(category);
        newMatches[category].append(match);
    }

    // Get rid of all categories that are no longer present
    auto it = m_categories.begin();
    while (it != m_categories.end()) {
        const int categoryNumber = int(std::distance(m_categories.begin(), it));

        if (!newCategories.contains(*it)) {
            beginRemoveRows(QModelIndex(), categoryNumber, categoryNumber);
            m_matches.remove(*it);
            it = m_categories.erase(it);
            endRemoveRows();
        } else {
            ++it;
        }
    }

    // Update the existing categories by adding/removing new/removed rows and
    // updating changed ones
    for (auto it = m_categories.constBegin(); it != m_categories.constEnd(); ++it) {
        Q_ASSERT(newCategories.contains(*it));

        const int categoryNumber = int(std::distance(m_categories.constBegin(), it));
        const QModelIndex categoryIdx = index(categoryNumber, 0);

        // don't use operator[] as to not insert an empty list
        // TODO why? shouldn't m_categories and m_matches be in sync?
        auto oldCategoryIt = m_matches.find(*it);
        Q_ASSERT(oldCategoryIt != m_matches.end());

        auto &oldMatchesInCategory = *oldCategoryIt;
        const auto newMatchesInCategory = newMatches.value(*it);

        Q_ASSERT(!oldMatchesInCategory.isEmpty());
        Q_ASSERT(!newMatches.isEmpty());

        // Emit a change for all existing matches if any of them changed
        // TODO only emit a change for the ones that changed
        bool emitDataChanged = false;

        const int oldCount = oldMatchesInCategory.count();
        const int newCount = newMatchesInCategory.count();

        const int countCeiling = qMin(oldCount, newCount);

        for (int i = 0; i < countCeiling; ++i) {
            auto &oldMatch = oldMatchesInCategory[i];
            if (oldMatch != newMatchesInCategory.at(i)) {
                oldMatch = newMatchesInCategory.at(i);
                emitDataChanged = true;
            }
        }

        // Now that the source data has been updated, emit the data changes we noted down earlier
        if (emitDataChanged) {
            Q_EMIT dataChanged(index(0, 0, categoryIdx), index(countCeiling - 1, 0, categoryIdx));
        }

        // Signal insertions for any new items
        if (newCount > oldCount) {
            beginInsertRows(categoryIdx, oldCount, newCount - 1);
            oldMatchesInCategory = newMatchesInCategory;
            endInsertRows();
        } else if (newCount < oldCount) {
            beginRemoveRows(categoryIdx, newCount, oldCount - 1);
            oldMatchesInCategory = newMatchesInCategory;
            endRemoveRows();
        }

        // Remove it from the "new" categories so in the next step we can add all genuinely new categories in one go
        newCategories.remove(*it);
    }

    // Finally add all the new categories
    if (!newCategories.isEmpty()) {
        beginInsertRows(QModelIndex(), m_categories.count(), m_categories.count() + newCategories.count() - 1);

        for (const QString &newCategory : newCategories) {
            const auto matchesInNewCategory = newMatches.value(newCategory);

            m_matches[newCategory] = matchesInNewCategory;
            m_categories.append(newCategory);
        }

        endInsertRows();
    }

    Q_ASSERT(m_categories.count() == m_matches.count());

    m_hasMatches = !m_matches.isEmpty();

    Q_EMIT matchesChanged();
}

QString RunnerResultsModel::queryString() const
{
    return m_queryString;
}

void RunnerResultsModel::setQueryString(const QString &queryString, const QString &runner)
{
    // If our query and runner are the same we don't need to query again
    if (m_queryString.trimmed() == queryString.trimmed() && m_prevRunner == runner) {
        return;
    }

    m_prevRunner = runner;
    m_queryString = queryString;
    m_hasMatches = false;
    if (queryString.isEmpty()) {
        clear();
    } else if (!queryString.trimmed().isEmpty()) {
        m_manager->launchQuery(queryString, runner);
        setQuerying(true);
    }
    Q_EMIT queryStringChanged(queryString);
}

bool RunnerResultsModel::querying() const
{
    return m_querying;
}

void RunnerResultsModel::setQuerying(bool querying)
{
    if (m_querying != querying) {
        m_querying = querying;
        Q_EMIT queryingChanged();
    }
}

void RunnerResultsModel::clear()
{
    m_manager->reset();
    m_manager->matchSessionComplete();

    setQuerying(false);

    beginResetModel();
    m_categories.clear();
    m_matches.clear();
    endResetModel();

    m_hasMatches = false;
}

bool RunnerResultsModel::run(const QModelIndex &idx)
{
    KRunner::QueryMatch match = fetchMatch(idx);
    if (match.isValid() && match.isEnabled()) {
        return m_manager->run(match);
    }
    return false;
}

bool RunnerResultsModel::runAction(const QModelIndex &idx, int actionNumber)
{
    KRunner::QueryMatch match = fetchMatch(idx);
    if (!match.isValid() || !match.isEnabled()) {
        return false;
    }

    const auto actions = m_manager->actionsForMatch(match);
    if (actionNumber < 0 || actionNumber >= actions.count()) {
        return false;
    }

    match.setSelectedAction(actions.at(actionNumber));
    return m_manager->run(match);
}

int RunnerResultsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

int RunnerResultsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) { // root level
        return m_categories.count();
    }

    if (parent.internalId()) {
        return 0;
    }

    const QString category = m_categories.value(parent.row());
    return m_matches.value(category).count();
}

QVariant RunnerResultsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.internalId()) { // runner match
        if (int(index.internalId() - 1) >= m_categories.count()) {
            return QVariant();
        }

        KRunner::QueryMatch match = fetchMatch(index);
        if (!match.isValid()) {
            return QVariant();
        }

        switch (role) {
        case Qt::DisplayRole:
            return match.text();
        case Qt::DecorationRole:
            if (!match.iconName().isEmpty()) {
                return match.iconName();
            }
            return match.icon();
        case ResultsModel::TypeRole:
            return match.type();
        case ResultsModel::RelevanceRole:
            return match.relevance();
        case ResultsModel::IdRole:
            return match.id();
        case ResultsModel::EnabledRole:
            return match.isEnabled();
        case ResultsModel::CategoryRole:
            return match.matchCategory();
        case ResultsModel::SubtextRole:
            return match.subtext();
        case ResultsModel::MultiLineRole:
            return match.isMultiLine();
        case ResultsModel::ActionsRole: {
            const auto actions = m_manager->actionsForMatch(match);
            if (actions.isEmpty()) {
                return QVariantList();
            }

            QVariantList actionsList;
            actionsList.reserve(actions.size());

            for (QAction *action : actions) {
                actionsList.append(QVariant::fromValue(action));
            }

            return actionsList;
        }
        }

        return QVariant();
    }

    // category
    if (index.row() >= m_categories.count()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return m_categories.at(index.row());

    // Returns the highest type/role within the group
    case ResultsModel::TypeRole: {
        int highestType = 0;
        for (int i = 0; i < rowCount(index); ++i) {
            const int type = this->index(i, 0, index).data(ResultsModel::TypeRole).toInt();
            if (type > highestType) {
                highestType = type;
            }
        }
        return highestType;
    }
    case ResultsModel::RelevanceRole: {
        qreal highestRelevance = 0.0;
        for (int i = 0; i < rowCount(index); ++i) {
            const qreal relevance = this->index(i, 0, index).data(ResultsModel::RelevanceRole).toReal();
            if (relevance > highestRelevance) {
                highestRelevance = relevance;
            }
        }
        return highestRelevance;
    }
    }

    return QVariant();
}

QModelIndex RunnerResultsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column != 0) {
        return QModelIndex();
    }

    if (parent.isValid()) {
        const QString category = m_categories.value(parent.row());
        const auto matches = m_matches.value(category);
        if (row < matches.count()) {
            return createIndex(row, column, int(parent.row() + 1));
        }

        return QModelIndex();
    }

    if (row < m_categories.count()) {
        return createIndex(row, column, nullptr);
    }

    return QModelIndex();
}

QModelIndex RunnerResultsModel::parent(const QModelIndex &child) const
{
    if (child.internalId()) {
        return createIndex(int(child.internalId() - 1), 0, nullptr);
    }

    return QModelIndex();
}

QMimeData *RunnerResultsModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty()) {
        return nullptr;
    }

    KRunner::QueryMatch match = fetchMatch(indexes.first());
    if (!match.isValid()) {
        return nullptr;
    }

    return m_manager->mimeDataForMatch(match);
}

KRunner::RunnerManager *RunnerResultsModel::runnerManager() const
{
    return m_manager;
}

}
