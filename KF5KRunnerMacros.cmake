#.rst:
# KF5KRunnerMacros
# ---------------------------
#
# This module provides the ``krunner_generate_runner_test`` function for
# generating a setup for a runner test.
# ::
#
#   krunner_generate_runner_test(<runner id>
#       RUNNER_ID <runner id>
#       CLASS_NAME <runner name>
#       PLUGIN_BUILD_DIR <dir where the builded plugins will be located>
#       OUTPUT_FILE <output header>
#   )
#
# A header file, ``<header_file_name>``, will be generated along with a corresponding
# source file, which will be added to ``<sources_var>``. These will provide a
# KCModuleData that can be referred to from C++ code using ``<class_name>``.
# This module will autoregister settings declared on ``setting_header.h`` with class name ``SettingClass``.
# Multiple settings classes / settings headers can be specified.

include(CMakeParseArguments)

set(_KRUNNER_TEST_TEMPLATE_H "${CMAKE_CURRENT_LIST_DIR}/abstractrunnertest.h.in")

function(krunner_generate_runner_test PLUGIN_ID)
    set(options)
    set(oneValueArgs CLASS_NAME PLUGIN_BUILD_DIR OUTPUT_FILE)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "" ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments to krunner_generate_runner_test: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_PLUGIN_BUILD_DIR)
        set(ARG_PLUGIN_BUILD_DIR "${CMAKE_BINARY_DIR}/bin")
    endif()
    if(NOT ARG_CLASS_NAME)
        set(ARG_CLASS_NAME "AbstractRunnerTest")
    endif()
    if(NOT ARG_OUTPUT_FILE)
        set(ARG_OUTPUT_FILE "abstractrunnertest.h")
    endif()

    set(PLUGIN_BUILD_DIR ${ARG_PLUGIN_BUILD_DIR})
    set(CLASS_NAME ${ARG_CLASS_NAME})
    set(OUTPUT_FILE ${ARG_OUTPUT_FILE})
    configure_file("${_KRUNNER_TEST_TEMPLATE_H}" "${ARG_OUTPUT_FILE}")
endfunction()
