// SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
// SPDX-License-Identifier: LGPL-2.0-or-later
#ifndef KRUNNER_ACTION_H
#define KRUNNER_ACTION_H

#include "krunner_export.h"

#include <QString>
#include <QVariant>
#include <memory>

namespace KRunner
{
class ActionPrivate;
class KRUNNER_EXPORT Action final
{
public:
    /**
     *
     */
    explicit Action(const QString &id, const QString &text, const QString &iconName);
    /**
     *
     */
    explicit Action(const QString &id, const QString &text, const QIcon &icon);

    /// Empty constructor
    Action();

    ~Action();

    /**
     *
     */
    Action(const KRunner::Action &other);

    Action &operator=(const Action &other);

    operator bool() const
    {
        return !id().isEmpty();
    }

    QString id() const;
    QString text() const;
    QString iconName() const;
    QIcon icon() const;

private:
    std::unique_ptr<ActionPrivate> d;
};

using Actions = QList<KRunner::Action>;
}
#endif
