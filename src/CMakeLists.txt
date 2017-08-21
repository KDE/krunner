add_subdirectory(declarative)

set (KF5Runner_SRCS
    abstractrunner.cpp
    dbusrunner.cpp
    runnerjobs.cpp
    querymatch.cpp
    runnercontext.cpp
    runnermanager.cpp
    runnersyntax.cpp
    krunner_debug.cpp)
set_property(SOURCE "data/org.kde.krunner1.xml" PROPERTY INCLUDE dbusutils_p.h)
qt5_add_dbus_interface(KF5Runner_SRCS "data/org.kde.krunner1.xml" krunner_iface)

add_library(KF5Runner
    ${KF5Runner_SRCS})

generate_export_header(KF5Runner BASE_NAME KRunner)
add_library(KF5::Runner ALIAS KF5Runner)

target_include_directories(KF5Runner INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/KRunner>")
target_include_directories(KF5Runner PUBLIC "$<BUILD_INTERFACE:${KRunner_BINARY_DIR};${CMAKE_CURRENT_BINARY_DIR}>")


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
        KF5::Solid
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
        BLANK_MACROS
            KRUNNER_EXPORT
            KRUNNER_DEPRECATED
            KRUNNER_DEPRECATED_EXPORT
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
