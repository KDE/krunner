// SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
// SPDX-License-Identifier: LGPL-2.0-or-later
#include "action.h"

#include <QIcon>

namespace KRunner
{
class ActionPrivate
{
public:
    explicit ActionPrivate(const QString id, const QString text, const QString iconName, const QIcon icon)
        : m_id(id)
        , m_text(text)
        , m_iconName(iconName)
        , m_icon(icon)
    {
    }
    explicit ActionPrivate() = default;
    explicit ActionPrivate(const ActionPrivate &action) = default;
    const QString m_id;
    const QString m_text;
    const QString m_iconName;
    const QIcon m_icon;
};

Action::Action(const QString &id, const QString &iconName, const QString &text)
    : d(new ActionPrivate(id, text, iconName, QIcon()))
{
}
Action::Action(const QString &id, const QIcon &icon, const QString &text)
    : d(new ActionPrivate(id, text, QString(), icon))
{
}
Action::Action(const Action &action)
    : d(new ActionPrivate(*action.d))
{
}
Action::Action()
    : d(new ActionPrivate())
{
}

Action::~Action() = default;
Action &Action::operator=(const Action &other)
{
    d.reset(new ActionPrivate(*other.d));
    return *this;
}

QString Action::id() const
{
    return d->m_id;
}
QString Action::text() const
{
    return d->m_text;
}
QString Action::iconName() const
{
    return d->m_iconName;
}
QIcon Action::icon() const
{
    return d->m_icon;
}
}
