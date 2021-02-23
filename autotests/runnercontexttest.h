/*
    SPDX-FileCopyrightText: 2010 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef RUNNER_CONTEXT_TEST_H
#define RUNNER_CONTEXT_TEST_H

#include <QTest>

#include "runnercontext.h"

class RunnerContextTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeDetection_data();
    void typeDetection();

private:
    Plasma::RunnerContext m_context;
};

#endif
