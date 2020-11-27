#.rst:
# KF5KRunnerMacros
# ---------------------------
#
# This module provides the ``krunner_generate_runner_test`` which takes the X-KDE-PluginInfo-Name runner
# attribute and creates the `AbstractRunnerTest` class in the `abstractrunnertest.h` file.
# The contained class will contain the `runner` and `manager` properties.
# The `initProperties()` method will (re)create them. Also the `doQuery()` method is provided,
# which will query the runner manager and verify that the query has finished.

# SPDX-FileCopyrightText: (C) 2020 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause

set(_KRUNNER_TEST_TEMPLATE_H "${CMAKE_CURRENT_LIST_DIR}/abstractrunnertest.h.in")

function(krunner_generate_runner_test PLUGIN_ID)
    set(PLUGIN_BUILD_DIR "${CMAKE_BINARY_DIR}/bin")
    configure_file("${_KRUNNER_TEST_TEMPLATE_H}" "abstractrunnertest.h")
endfunction()
