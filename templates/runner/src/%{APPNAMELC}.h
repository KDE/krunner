/*
    SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef %{APPNAMEUC}_H
#define %{APPNAMEUC}_H

#include <KRunner/AbstractRunner>

class %{APPNAME} : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    %{APPNAME}(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~%{APPNAME}() override;

public: // KRunner::AbstractRunner API
    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;
};

#endif
