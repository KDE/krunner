/*
    SPDX-FileCopyrightText: 2011 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "runnermodel.h"

#include <QAction>
#include <QTimer>

#include "krunner_debug.h"

#include <KRunner/RunnerManager>

RunnerModel::RunnerModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_startQueryTimer(new QTimer(this))
    , m_runningChangedTimeout(new QTimer(this))
{
    m_startQueryTimer->setSingleShot(true);
    m_startQueryTimer->setInterval(10);
    connect(m_startQueryTimer, &QTimer::timeout, this, &RunnerModel::startQuery);

    // FIXME: HACK: some runners stay in a running but finished state, not possible to say if it's actually over
    m_runningChangedTimeout->setSingleShot(true);
    connect(m_runningChangedTimeout, &QTimer::timeout, this, &RunnerModel::queryHasFinished);
}

QHash<int, QByteArray> RunnerModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(Qt::DisplayRole, "display");
    roles.insert(Qt::DecorationRole, "decoration");
    roles.insert(Label, "label");
    roles.insert(Icon, "icon");
    roles.insert(Type, "type");
    roles.insert(Relevance, "relevance");
    roles.insert(Data, "data");
    roles.insert(Id, "id");
    roles.insert(SubText, "description");
    roles.insert(Enabled, "enabled");
    roles.insert(RunnerId, "runnerid");
    roles.insert(RunnerName, "runnerName");
    roles.insert(Actions, "actions");
    return roles;
}

int RunnerModel::rowCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : m_matches.count();
}

int RunnerModel::count() const
{
    return m_matches.count();
}

QStringList RunnerModel::runners() const
{
    return m_manager ? m_manager->allowedRunners() : m_pendingRunnersList;
}

void RunnerModel::setRunners(const QStringList &allowedRunners)
{
    // use sets to ensure comparison is order-independent
    const auto runners = this->runners();
    if (QSet<QString>(allowedRunners.begin(), allowedRunners.end()) == QSet<QString>(runners.begin(), runners.end())) {
        return;
    }
    if (m_manager) {
        m_manager->setAllowedRunners(allowedRunners);

        // automagically enter single runner mode if there's only 1 allowed runner
        m_manager->setSingleMode(allowedRunners.count() == 1);
    } else {
        m_pendingRunnersList = allowedRunners;
    }

    // to trigger single runner fun!
    if (allowedRunners.count() == 1) {
        m_singleRunnerId = allowedRunners.first();
        scheduleQuery(QString());
    } else {
        m_singleRunnerId.clear();
    }
    Q_EMIT runnersChanged();
}

void RunnerModel::run(int index)
{
    if (index >= 0 && index < m_matches.count()) {
        m_manager->run(m_matches.at(index));
    }
}

bool RunnerModel::isRunning() const
{
    return m_running;
}

QVariant RunnerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.parent().isValid() || index.column() > 0 || index.row() < 0 || index.row() >= m_matches.count()) {
        // index requested must be valid, but we have no child items!
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Label) {
        return m_matches.at(index.row()).text();
    } else if (role == Qt::DecorationRole || role == Icon) {
        return m_matches.at(index.row()).icon();
    } else if (role == Type) {
        return m_matches.at(index.row()).type();
    } else if (role == Relevance) {
        return m_matches.at(index.row()).relevance();
    } else if (role == Data) {
        return m_matches.at(index.row()).data();
    } else if (role == Id) {
        return m_matches.at(index.row()).id();
    } else if (role == SubText) {
        return m_matches.at(index.row()).subtext();
    } else if (role == Enabled) {
        return m_matches.at(index.row()).isEnabled();
    } else if (role == RunnerId) {
        return m_matches.at(index.row()).runner()->id();
    } else if (role == RunnerName) {
        return m_matches.at(index.row()).runner()->name();
    } else if (role == Actions) {
        QVariantList actions;
        Plasma::QueryMatch amatch = m_matches.at(index.row());
        const QList<QAction *> theactions = m_manager->actionsForMatch(amatch);
        for (QAction *action : theactions) {
            actions += QVariant::fromValue<QObject *>(action);
        }
        return actions;
    }

    return QVariant();
}

QString RunnerModel::currentQuery() const
{
    return m_manager ? m_manager->query() : QString();
}

void RunnerModel::scheduleQuery(const QString &query)
{
    m_pendingQuery = query;
    m_startQueryTimer->start();
}

void RunnerModel::startQuery()
{
    // avoid creating a manager just so we can run nothing
    // however, if we have one pending runner, then we'll be in single query mode
    // and a null query is a valid query
    if (!m_manager && m_pendingRunnersList.count() != 1 && m_pendingQuery.isEmpty()) {
        return;
    }

    if (createManager() || m_pendingQuery != m_manager->query()) {
        m_manager->launchQuery(m_pendingQuery, m_singleRunnerId);
        Q_EMIT queryChanged();
        m_running = true;
        Q_EMIT runningChanged(true);
    }
}

bool RunnerModel::createManager()
{
    if (!m_manager) {
        m_manager = new Plasma::RunnerManager(this);
        connect(m_manager, &Plasma::RunnerManager::matchesChanged, this, &RunnerModel::matchesChanged);
        connect(m_manager, &Plasma::RunnerManager::queryFinished, this, &RunnerModel::queryHasFinished);

        if (!m_pendingRunnersList.isEmpty()) {
            setRunners(m_pendingRunnersList);
            m_pendingRunnersList.clear();
        }
        // connect(m_manager, SIGNAL(queryFinished()), this, SLOT(queryFinished()));
        return true;
    }

    return false;
}

void RunnerModel::matchesChanged(const QList<Plasma::QueryMatch> &matches)
{
    bool fullReset = false;
    int oldCount = m_matches.count();
    int newCount = matches.count();
    if (newCount > oldCount) {
        // We received more matches than we had. If all common matches are the
        // same, we can just append new matches instead of resetting the whole
        // model
        for (int row = 0; row < oldCount; ++row) {
            if (!(m_matches.at(row) == matches.at(row))) {
                fullReset = true;
                break;
            }
        }
        if (!fullReset) {
            // Not a full reset, inserting rows
            beginInsertRows(QModelIndex(), oldCount, newCount - 1);
            m_matches = matches;
            endInsertRows();
            Q_EMIT countChanged();
        }
    } else {
        fullReset = true;
    }

    if (fullReset) {
        beginResetModel();
        m_matches = matches;
        endResetModel();
        Q_EMIT countChanged();
    }
    m_runningChangedTimeout->start(3000);
}

void RunnerModel::queryHasFinished()
{
    m_running = false;
    Q_EMIT runningChanged(false);
}
