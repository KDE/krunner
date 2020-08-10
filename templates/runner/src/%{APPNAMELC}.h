/*
    SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef %{APPNAMEUC}_H
#define %{APPNAMEUC}_H

#include <KRunner/AbstractRunner>

class %{APPNAME} : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    %{APPNAME}(QObject *parent, const QVariantList &args);
    ~%{APPNAME}() override;

public: // Plasma::AbstractRunner API
    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;
};

#endif
