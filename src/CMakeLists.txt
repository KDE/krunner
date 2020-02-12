add_subdirectory(declarative)

set (KF5Runner_SRCS
    abstractrunner.cpp
    dbusrunner.cpp
    runnerjobs.cpp
    querymatch.cpp
    runnercontext.cpp
    runnermanager.cpp
    runnersyntax.cpp)
ecm_qt_declare_logging_category(KF5Runner_SRCS
    HEADER krunner_debug.h
    IDENTIFIER KRUNNER
    CATEGORY_NAME org.kde.krunner
    DESCRIPTION "KRunner"
    EXPORT KRUNNER
)
set_property(SOURCE "data/org.kde.krunner1.xml" PROPERTY INCLUDE dbusutils_p.h)

add_library(KF5Runner
    ${KF5Runner_SRCS})
add_library(KF5::Runner ALIAS KF5Runner)
ecm_generate_export_header(KF5Runner
    BASE_NAME KRunner
    GROUP_BASE_NAME KF
    VERSION ${KF5_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 5.28
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)

set(KRunner_BUILD_INCLUDE_DIRS ${KRunner_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(KF5Runner INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/KRunner>")
target_include_directories(KF5Runner PUBLIC "$<BUILD_INTERFACE:${KRunner_BUILD_INCLUDE_DIRS}>")


target_link_libraries(KF5Runner
    PUBLIC
        Qt5::Core
        KF5::Plasma # Must be public because abstractrunner.h needs plasma/version.h
    PRIVATE
        Qt5::DBus
        Qt5::Gui
        Qt5::Widgets
        KF5::ConfigCore
        KF5::Service
        KF5::I18n
        KF5::ThreadWeaver
        KF5::CoreAddons #KShell
        KF5::KIOCore #KProtocolInfo
)

set_target_properties(KF5Runner
    PROPERTIES VERSION ${KRUNNER_VERSION_STRING}
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

    PREFIX KRunner
    REQUIRED_HEADERS KRunner_HEADERS
)

# Install files

install(TARGETS KF5Runner
        EXPORT KF5RunnerTargets
        ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${KRunner_CamelCase_HEADERS}
        DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KRunner/KRunner
        COMPONENT Devel)

install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/krunner_export.h
            ${KRunner_HEADERS}
        DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KRunner/krunner
        COMPONENT Devel)

install(FILES
   data/servicetypes/plasma-runner.desktop
   DESTINATION ${SERVICETYPES_INSTALL_DIR})

add_custom_target(copy_servicetypes)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/kservicetypes5)
add_custom_command(TARGET copy_servicetypes PRE_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/data/servicetypes/plasma-runner.desktop
    ${CMAKE_BINARY_DIR}/bin/data/kservicetypes5)
add_dependencies(KF5Runner copy_servicetypes)

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
        VERSION ${KF5_VERSION}
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
        PREDEFINED_MACROS
            "PLASMA_ENABLE_DEPRECATED_SINCE(x, y)=1"
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KRunner LIB_NAME KF5Runner DEPS "core" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KF5_INCLUDE_INSTALL_DIR}/KRunner)
install(FILES ${PRI_FILENAME}
        DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

install(FILES
   "data/org.kde.krunner1.xml"
   DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
   RENAME kf5_org.kde.krunner1.xml)
