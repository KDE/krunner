/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "abstractrunner.h"

using namespace Plasma;

class FakeRunner : public AbstractRunner
{
public:
    FakeRunner()
        : AbstractRunner(nullptr, KPluginMetaData(QStringLiteral("metadata.desktop")), QVariantList())
    {
    }
};
