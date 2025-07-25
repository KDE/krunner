# SPDX-FileCopyrightText: KDE Contributors
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

set(KF_VERSION "6.17.0") # handled by release scripts
set(KF_DEP_VERSION "6.16.0") # handled by release scripts
project(KRunner VERSION ${KF_VERSION})

# ECM setup
include(FeatureSummary)
find_package(ECM 6.16.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(ECMGenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(KDEPackageAppTemplates)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupQtPluginMacroNames)
include(ECMDeprecationSettings)
include(KDEGitCommitHooks)
include(ECMGenerateQDoc)

set(EXCLUDE_DEPRECATED_BEFORE_AND_AT 0 CACHE STRING "Control the range of deprecated API excluded from the build [default=0].")

set(krunner_version_header "${CMAKE_CURRENT_BINARY_DIR}/src/krunner_version.h")
ecm_setup_version(PROJECT
    VARIABLE_PREFIX KRUNNER
    VERSION_HEADER "${krunner_version_header}"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF6RunnerConfigVersion.cmake"
    SOVERSION 6
)

# Dependencies
set(REQUIRED_QT_VERSION 6.7.0)

find_package(Qt6 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Gui)

ecm_set_disabled_deprecation_versions(
    QT 6.10.0
    KF 6.16.0
)

find_package(KF6Config ${KF_DEP_VERSION} REQUIRED)
find_package(KF6CoreAddons ${KF_DEP_VERSION} REQUIRED)
find_package(KF6I18n ${KF_DEP_VERSION} REQUIRED)
find_package(KF6ItemModels ${KF_DEP_VERSION})
find_package(KF6WindowSystem ${KF_DEP_VERSION} REQUIRED)

# Subdirectories
add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
add_subdirectory(templates)

# Create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF6Runner")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KF6RunnerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KF6RunnerConfig.cmake"
    INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/KF6RunnerConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/KF6RunnerConfigVersion.cmake"
            "${CMAKE_CURRENT_SOURCE_DIR}/KF6KRunnerMacros.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)

install(EXPORT KF6RunnerTargets
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE KF6RunnerTargets.cmake
        NAMESPACE KF6::)

install(FILES "${krunner_version_header}"
        DESTINATION "${KDE_INSTALL_INCLUDEDIR_KF}/KRunner"
        COMPONENT Devel)

include(ECMFeatureSummary)
ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
