# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

include(ECMAddTests)

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

ecm_add_tests(
   dbusrunnertest.cpp
   LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF6Runner Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::DBus KF6::ConfigCore
)
ecm_add_tests(
        runnermatchmethodstest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF6Runner Qt${QT_MAJOR_VERSION}::Widgets KF6::ConfigCore
)


kcoreaddons_add_plugin(fakerunnerplugin SOURCES fakerunnerplugin.cpp INSTALL_NAMESPACE "krunnertest" STATIC)
target_link_libraries(fakerunnerplugin  KF6Runner)
kcoreaddons_desktop_to_json(fakerunnerplugin metadatafile1.desktop)
ecm_add_tests(
    runnermanagerhistorytest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF6Runner Qt${QT_MAJOR_VERSION}::Widgets KF6::ConfigCore
)
kcoreaddons_target_static_plugins(runnermanagerhistorytest krunnertest)

ecm_add_tests(
    runnermanagersinglerunnermodetest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF6Runner Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::DBus KF6::ConfigCore
)

ecm_add_tests(
    runnermanagertest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF6Runner Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::DBus KF6::ConfigCore
)
kcoreaddons_target_static_plugins(runnermanagertest krunnertest)
ecm_add_tests(
    testmetadataconversion.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF6Runner Qt${QT_MAJOR_VERSION}::Widgets Qt${QT_MAJOR_VERSION}::DBus KF6::ConfigCore
)

add_executable(testremoterunner)
qt_add_dbus_adaptor(demoapp_dbus_adaptor_SRCS "../src/data/org.kde.krunner1.xml" testremoterunner.h TestRemoteRunner)
target_sources(testremoterunner PRIVATE testremoterunner.cpp ${demoapp_dbus_adaptor_SRCS})
target_link_libraries(testremoterunner
    Qt${QT_MAJOR_VERSION}::DBus
    Qt${QT_MAJOR_VERSION}::Gui
    KF6::Runner
)

include(../KF6KRunnerMacros.cmake)
configure_krunner_test(dbusrunnertest testremoterunner DESKTOP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dbusrunnertest.desktop")
configure_krunner_test(runnermanagersinglerunnermodetest testremoterunner DESKTOP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dbusrunnertest.desktop")
configure_krunner_test(runnermanagertest testremoterunner DESKTOP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dbusrunnertest.desktop")
