# SPDX-FileCopyrightText: Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause

add_library(krunnermodel_STATIC STATIC)
target_sources(krunnermodel_STATIC PRIVATE resultsmodel.cpp runnerresultsmodel.cpp qmlplugin.cpp)
target_link_libraries (krunnermodel_STATIC
    Qt6::Qml
    Qt6::Gui
    KF6::ItemModels
    KF6::Runner
)

if (KRUNNER_USE_QML)
    include(ECMQmlModule)
    find_package(Qt6Qml ${QT_MIN_VERSION})
    ecm_add_qml_module(krunnerquickplugin URI "org.kde.krunner" VERSION 1.0 CLASSNAME "KRunnerQuickPlugin")
    target_link_libraries(krunnerquickplugin PRIVATE krunnermodel_STATIC)
    ecm_finalize_qml_module(krunnerquickplugin DESTINATION ${KDE_INSTALL_QMLDIR})
endif()
