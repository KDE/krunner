/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "querymatch.h"

#include <QAction>
#include <QIcon>
#include <QReadWriteLock>
#include <QSharedData>
#include <QVariant>

#include "krunner_debug.h"

#include "abstractrunner.h"

namespace Plasma
{
class QueryMatchPrivate : public QSharedData
{
public:
    QueryMatchPrivate(AbstractRunner *r)
        : QSharedData()
        , runner(r)
    {
    }

    QueryMatchPrivate(const QueryMatchPrivate &other)
        : QSharedData(other)
    {
        QReadLocker l(other.lock);
        runner = other.runner;
        type = other.type;
        relevance = other.relevance;
        selAction = other.selAction;
        enabled = other.enabled;
        idSetByData = other.idSetByData;
        matchCategory = other.matchCategory;
        id = other.id;
        text = other.text;
        subtext = other.subtext;
        icon = other.icon;
        iconName = other.iconName;
        data = other.data;
#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 82)
        mimeType = other.mimeType;
#endif
        urls = other.urls;
        actions = other.actions;
        multiLine = other.multiLine;
    }

    ~QueryMatchPrivate()
    {
        delete lock;
    }

    QReadWriteLock *lock = new QReadWriteLock(QReadWriteLock::Recursive);
    QPointer<AbstractRunner> runner;
    QueryMatch::Type type = QueryMatch::ExactMatch;
    QString matchCategory;
    QString id;
    QString text;
    QString subtext;
    QString mimeType;
    QList<QUrl> urls;
    QIcon icon;
    QString iconName;
    QVariant data;
    qreal relevance = .7;
    QAction *selAction = nullptr;
    bool enabled = true;
    bool idSetByData = false;
    QList<QAction *> actions;
    bool multiLine = false;
};

QueryMatch::QueryMatch(AbstractRunner *runner)
    : d(new QueryMatchPrivate(runner))
{
}

QueryMatch::QueryMatch(const QueryMatch &other)
    : d(other.d)
{
}

QueryMatch::~QueryMatch()
{
}

bool QueryMatch::isValid() const
{
    return d->runner != nullptr;
}

QString QueryMatch::id() const
{
    if (d->id.isEmpty() && d->runner) {
        return d->runner.data()->id();
    }

    return d->id;
}

void QueryMatch::setType(Type type)
{
    d->type = type;
}

QueryMatch::Type QueryMatch::type() const
{
    return d->type;
}

void QueryMatch::setMatchCategory(const QString &category)
{
    d->matchCategory = category;
}

QString QueryMatch::matchCategory() const
{
    if (d->matchCategory.isEmpty() && d->runner) {
        return d->runner->name();
    }
    return d->matchCategory;
}

void QueryMatch::setRelevance(qreal relevance)
{
    d->relevance = qMax(qreal(0.0), relevance);
}

qreal QueryMatch::relevance() const
{
    return d->relevance;
}

AbstractRunner *QueryMatch::runner() const
{
    return d->runner.data();
}

void QueryMatch::setText(const QString &text)
{
    QWriteLocker locker(d->lock);
    d->text = text;
}

void QueryMatch::setSubtext(const QString &subtext)
{
    QWriteLocker locker(d->lock);
    d->subtext = subtext;
}

void QueryMatch::setData(const QVariant &data)
{
    QWriteLocker locker(d->lock);
    d->data = data;

    if (d->id.isEmpty() || d->idSetByData) {
        const QString id = data.toString();
        if (!id.isEmpty()) {
            setId(data.toString());
            d->idSetByData = true;
        }
    }
}

void QueryMatch::setId(const QString &id)
{
    QWriteLocker locker(d->lock);
    if (d->runner && d->runner->hasUniqueResults()) {
        d->id = id;
    } else {
        if (d->runner) {
            d->id = d->runner.data()->id();
        }
        if (!id.isEmpty()) {
            d->id.append(QLatin1Char('_')).append(id);
        }
    }
    d->idSetByData = false;
}

void QueryMatch::setIcon(const QIcon &icon)
{
    QWriteLocker locker(d->lock);
    d->icon = icon;
}

void QueryMatch::setIconName(const QString &iconName)
{
    QWriteLocker locker(d->lock);
    d->iconName = iconName;
}

QVariant QueryMatch::data() const
{
    QReadLocker locker(d->lock);
    return d->data;
}

QString QueryMatch::text() const
{
    QReadLocker locker(d->lock);
    return d->text;
}

QString QueryMatch::subtext() const
{
    QReadLocker locker(d->lock);
    return d->subtext;
}

QIcon QueryMatch::icon() const
{
    QReadLocker locker(d->lock);
    return d->icon;
}

QString QueryMatch::iconName() const
{
    QReadLocker locker(d->lock);
    return d->iconName;
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 82)
void QueryMatch::setMimeType(const QString &mimeType)
{
    QWriteLocker locker(d->lock);
    d->mimeType = mimeType;
}

QString QueryMatch::mimeType() const
{
    QReadLocker locker(d->lock);
    return d->mimeType;
}
#endif

void QueryMatch::setUrls(const QList<QUrl> &urls)
{
    QWriteLocker locker(d->lock);
    d->urls = urls;
}

QList<QUrl> QueryMatch::urls() const
{
    QReadLocker locker(d->lock);
    return d->urls;
}

void QueryMatch::setEnabled(bool enabled)
{
    d->enabled = enabled;
}

bool QueryMatch::isEnabled() const
{
    return d->enabled && d->runner;
}

QAction *QueryMatch::selectedAction() const
{
    return d->selAction;
}

void QueryMatch::setSelectedAction(QAction *action)
{
    d->selAction = action;
}

bool QueryMatch::operator<(const QueryMatch &other) const
{
    if (d->type == other.d->type) {
        if (isEnabled() != other.isEnabled()) {
            return other.isEnabled();
        }

        if (!qFuzzyCompare(d->relevance, other.d->relevance)) {
            return d->relevance < other.d->relevance;
        }

        QReadLocker locker(d->lock);
        QReadLocker otherLocker(other.d->lock);
        // when resorting to sort by alpha, we want the
        // reverse sort order!
        return d->text > other.d->text;
    }

    return d->type < other.d->type;
}

void QueryMatch::setMultiLine(bool multiLine)
{
    d->multiLine = multiLine;
}

bool QueryMatch::isMultiLine() const
{
    return d->multiLine;
}

QueryMatch &QueryMatch::operator=(const QueryMatch &other)
{
    if (d != other.d) {
        d = other.d;
    }

    return *this;
}

bool QueryMatch::operator==(const QueryMatch &other) const
{
    return (d == other.d);
}

bool QueryMatch::operator!=(const QueryMatch &other) const
{
    return (d != other.d);
}

void QueryMatch::run(const RunnerContext &context) const
{
    if (d->runner) {
        d->runner.data()->run(context, *this);
    }
}

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 71)
bool QueryMatch::hasConfigurationInterface() const
{
    return false;
}
#endif

#if KRUNNER_BUILD_DEPRECATED_SINCE(5, 71)
void QueryMatch::createConfigurationInterface(QWidget *parent)
{
    Q_UNUSED(parent)
}
#endif

void QueryMatch::setActions(const QList<QAction *> &actions)
{
    QWriteLocker locker(d->lock);
    d->actions = actions;
}

void QueryMatch::addAction(QAction *action)
{
    QWriteLocker locker(d->lock);
    d->actions << action;
}

QList<QAction *> QueryMatch::actions() const
{
    QReadLocker locker(d->lock);
    return d->actions;
}

} // Plasma namespace
