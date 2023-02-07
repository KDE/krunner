# SPDX-FileCopyrightText: Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_qml_module(krunnerquickplugin URI "org.kde.krunner" VERSION 1.0 CLASSNAME "KRunnerQuickPlugin")

target_sources(krunnerquickplugin PRIVATE resultsmodel.cpp runnerresultsmodel.cpp qmlplugin.cpp)

find_package(KF6ItemModels ${KF_DEP_VERSION} REQUIRED)
target_link_libraries (krunnerquickplugin
    Qt6::Qml
    Qt6::Quick
    Qt6::Gui
    KF6::ItemModels
    KF6::Runner
)

ecm_finalize_qml_module(krunnerquickplugin DESTINATION ${KDE_INSTALL_QMLDIR})
