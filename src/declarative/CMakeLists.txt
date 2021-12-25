# SPDX-FileCopyrightText: KDE contributors
# SPDX-License-Identifier: BSD-2-Clause
project(runnermodel)

add_library(runnermodelplugin SHARED)

target_sources(runnermodelplugin PRIVATE
    runnermodel.cpp
    runnermodel.h
    runnermodelplugin.cpp
    runnermodelplugin.h
)

ecm_qt_declare_logging_category(runnermodelplugin
    HEADER krunner_debug.h
    IDENTIFIER KRUNNER
    CATEGORY_NAME kf.runner.quick
)

target_link_libraries(runnermodelplugin
        Qt${QT_MAJOR_VERSION}::Quick
        Qt${QT_MAJOR_VERSION}::Qml
        Qt${QT_MAJOR_VERSION}::Widgets # QAction
        KF5::I18n
        KF5::Service
        KF5Runner
)

install(TARGETS runnermodelplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/runnermodel)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/runnermodel)

