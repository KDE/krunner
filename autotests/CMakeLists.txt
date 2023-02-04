# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

include(ECMAddTests)

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

ecm_add_tests(
   dbusrunnertest.cpp
   LINK_LIBRARIES Qt6::Gui Qt6::Test KF6Runner Qt6::DBus KF6::ConfigCore
)
ecm_add_tests(
        runnermatchmethodstest.cpp
    LINK_LIBRARIES Qt6::Gui Qt6::Test KF6Runner KF6::ConfigCore
)


kcoreaddons_add_plugin(fakerunnerplugin SOURCES fakerunnerplugin.cpp INSTALL_NAMESPACE "krunnertest" STATIC)
target_link_libraries(fakerunnerplugin  KF6Runner)
ecm_add_tests(
    runnermanagerhistorytest.cpp
    LINK_LIBRARIES Qt6::Gui Qt6::Test KF6Runner KF6::ConfigCore
)
kcoreaddons_target_static_plugins(runnermanagerhistorytest krunnertest)

ecm_add_tests(
    runnermanagersinglerunnermodetest.cpp
    LINK_LIBRARIES Qt6::Gui Qt6::Test KF6Runner Qt6::DBus KF6::ConfigCore
)

ecm_add_tests(
    runnermanagertest.cpp
    LINK_LIBRARIES Qt6::Gui Qt6::Test KF6Runner Qt6::DBus KF6::ConfigCore
)
kcoreaddons_target_static_plugins(runnermanagertest krunnertest)
ecm_add_tests(
    testmetadataconversion.cpp
    LINK_LIBRARIES Qt6::Test KF6Runner Qt6::DBus KF6::ConfigCore
)

add_executable(testremoterunner)
qt_add_dbus_adaptor(demoapp_dbus_adaptor_SRCS "../src/data/org.kde.krunner1.xml" testremoterunner.h TestRemoteRunner)
target_sources(testremoterunner PRIVATE testremoterunner.cpp ${demoapp_dbus_adaptor_SRCS})
target_link_libraries(testremoterunner
    Qt6::DBus
    Qt6::Gui
    KF6::Runner
)

include(../KF6KRunnerMacros.cmake)
krunner_configure_test(dbusrunnertest testremoterunner DESKTOP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dbusrunnertest.desktop")
krunner_configure_test(runnermanagersinglerunnermodetest testremoterunner DESKTOP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dbusrunnertest.desktop")
krunner_configure_test(runnermanagertest testremoterunner DESKTOP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dbusrunnertest.desktop")
