/*
    SPDX-FileCopyrightText: 2011 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef RUNNERMODEL_H
#define RUNNERMODEL_H

#include <QAbstractListModel>
#include <QStringList>

#include <KRunner/QueryMatch>

namespace Plasma
{
class RunnerManager;
class QueryMatch;
} // namespace Plasma

class QTimer;

/**
 * This model provides bindings to use KRunner from QML
 *
 * @author Aaron Seigo <aseigo@kde.org>
 */
class RunnerModel : public QAbstractListModel
{
    Q_OBJECT

    /**
     * @property string set the KRunner query
     */
    Q_PROPERTY(QString query WRITE scheduleQuery READ currentQuery NOTIFY queryChanged)

    /**
     * @property Array The list of all allowed runner plugins that will be executed
     */
    Q_PROPERTY(QStringList runners WRITE setRunners READ runners NOTIFY runnersChanged)

    /**
     * @property int The number of rows of the model
     */
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    /**
     * @property bool running: true when queries are in execution
     */
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

public:
    /**
     * @enum Roles of the model, they will be accessible from delegates
     */
    enum Roles {
        Type = Qt::UserRole + 1,
        Label,
        Icon,
        Relevance,
        Data,
        Id,
        SubText,
        Enabled,
        RunnerId,
        RunnerName,
        Actions,
    };

    explicit RunnerModel(QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;

    QString currentQuery() const;

    QStringList runners() const;
    void setRunners(const QStringList &allowedRunners);

    Q_SCRIPTABLE void run(int row);

    bool isRunning() const;

    int rowCount(const QModelIndex &) const override;
    int count() const;
    QVariant data(const QModelIndex &, int) const override;

public Q_SLOTS:
    void scheduleQuery(const QString &query);

Q_SIGNALS:
    void queryChanged();
    void countChanged();
    void runnersChanged();
    void runningChanged(bool running);

private Q_SLOTS:
    void startQuery();

private:
    bool createManager();

private Q_SLOTS:
    void matchesChanged(const QList<Plasma::QueryMatch> &matches);
    void queryHasFinished();

private:
    Plasma::RunnerManager *m_manager = nullptr;
    QList<Plasma::QueryMatch> m_matches;
    QStringList m_pendingRunnersList;
    QString m_singleRunnerId;
    QString m_pendingQuery;
    QTimer *const m_startQueryTimer;
    QTimer *const m_runningChangedTimeout;
    bool m_running = false;
};

#endif
