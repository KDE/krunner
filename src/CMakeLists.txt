# SPDX-FileCopyrightText: KDE Contributors
# SPDX-License-Identifier: BSD-2-Clause

# TODO: merge QML plugin from Milou into this instead and provide a plugin again officially,
# named "org.kde.krunner" to follow naming pattern for QML plugins from KF modules
if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.0.0)
    # deprecated plugin org.kde.runnermodel, no users known
    add_subdirectory(declarative)
endif()

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library(KF5Runner SHARED)
add_library(KF5::Runner ALIAS KF5Runner)

target_sources(KF5Runner PRIVATE
    abstractrunner.cpp
    abstractrunner.h
    abstractrunner_p.h
    abstractrunnertest.h
    dbusrunner.cpp
    dbusrunner_p.h
    dbusutils_p.h
    querymatch.cpp
    querymatch.h
    runnercontext.cpp
    runnercontext.h
    runnerjobs.cpp
    runnerjobs_p.h
    runnermanager.cpp
    runnermanager.h
    runnersyntax.cpp
    runnersyntax.h

)
ecm_qt_declare_logging_category(KF5Runner
    HEADER krunner_debug.h
    IDENTIFIER KRUNNER
    CATEGORY_NAME kf.runner
    OLD_CATEGORY_NAMES org.kde.krunner
    DESCRIPTION "KRunner"
    EXPORT KRUNNER
)
set_property(SOURCE "data/org.kde.krunner1.xml" PROPERTY INCLUDE dbusutils_p.h)

ecm_generate_export_header(KF5Runner
    BASE_NAME KRunner
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 5.28 5.71 5.72 5.73 5.76 5.77 5.79 5.81 5.82 5.85 5.86 5.88
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)

set(KRunner_BUILD_INCLUDE_DIRS ${KRunner_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(KF5Runner INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/KRunner>")
target_include_directories(KF5Runner PUBLIC "$<BUILD_INTERFACE:${KRunner_BUILD_INCLUDE_DIRS}>")


target_link_libraries(KF5Runner
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
    PRIVATE
        Qt${QT_MAJOR_VERSION}::DBus
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        KF5::ConfigCore
        KF5::Service
        KF5::I18n
        KF5::ThreadWeaver
        KF5::CoreAddons #KShell
        KF5::KIOCore #KProtocolInfo
)
if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
   EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.65.0)
    target_link_libraries(KF5Runner
        PUBLIC
            KF5::Plasma # Must be public because abstractrunner.h needs plasma/version.h
    )
endif()
if(HAVE_KACTIVITIES)
    target_link_libraries(KF5Runner PRIVATE KF5::Activities)
endif()

set_target_properties(KF5Runner
    PROPERTIES VERSION ${KRUNNER_VERSION}
    SOVERSION 5
    EXPORT_NAME "Runner"
)

ecm_generate_headers(KRunner_CamelCase_HEADERS
  HEADER_NAMES
    AbstractRunner
    RunnerContext
    RunnerManager
    RunnerSyntax
    QueryMatch
    AbstractRunnerTest

    PREFIX KRunner
    REQUIRED_HEADERS KRunner_HEADERS
)

# Install files

install(TARGETS KF5Runner
        EXPORT KF5RunnerTargets
        ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${KRunner_CamelCase_HEADERS}
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KRunner/KRunner
        COMPONENT Devel)

install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/krunner_export.h
            ${KRunner_HEADERS}
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KRunner/krunner
        COMPONENT Devel)

install(FILES
   data/servicetypes/plasma-runner.desktop
   DESTINATION ${KDE_INSTALL_KSERVICETYPESDIR})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/kservicetypes5)
add_custom_command(TARGET KF5Runner PRE_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/data/servicetypes/plasma-runner.desktop
    ${CMAKE_BINARY_DIR}/bin/data/kservicetypes5)

ecm_qt_install_logging_categories(
    EXPORT KRUNNER
    FILE krunner.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)


if(BUILD_QCH)
    ecm_add_qch(
        KF5Runner_QCH
        NAME KRunner
        BASE_NAME KF5Runner
        VERSION ${KF_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${KRunner_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt5Core_QCH
        INCLUDE_DIRS
            ${KRunner_BUILD_INCLUDE_DIRS}
        BLANK_MACROS
            KRUNNER_EXPORT
            KRUNNER_DEPRECATED
            KRUNNER_DEPRECATED_EXPORT
            "KRUNNER_DEPRECATED_VERSION(x, y, t)"
            "KRUNNER_DEPRECATED_VERSION_BELATED(x, y, xt, yt, t)"
        PREDEFINED_MACROS
            "KSERVICE_BUILD_DEPRECATED_SINCE(x, y)=1"
            "PLASMA_ENABLE_DEPRECATED_SINCE(x, y)=1"
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KRunner LIB_NAME KF5Runner DEPS "core" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF}/KRunner)
install(FILES ${PRI_FILENAME}
        DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

install(FILES
   "data/org.kde.krunner1.xml"
   DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
   RENAME kf5_org.kde.krunner1.xml)
