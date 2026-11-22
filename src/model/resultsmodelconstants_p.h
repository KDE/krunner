/*
    SPDX-FileCopyrightText: 2026 Farhad Hedayatifard <hf.farhad@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QtTypes>

namespace KRunner
{
// Single line text and subtext longer than this are cut off by the model, see RunnerResultsModel::data.
// This is far more than anyone reads in one line; it only guards against absurd amounts of text.
inline constexpr qsizetype maxDisplayTextLength = 2000;
}
