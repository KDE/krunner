cmake_minimum_required(VERSION 2.8.12)

project(KRunner)

# ECM setup
find_package(ECM 0.0.9 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(FeatureSummary)
include(GenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)

set(KF5_VERSION "4.96.0")

ecm_setup_version(${KF5_VERSION}
    VARIABLE_PREFIX KRUNNER
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/krunner_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5RunnerConfigVersion.cmake"
)

# Dependencies
set(REQUIRED_QT_VERSION "5.2.0")

find_package(Qt5 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Gui Quick)
find_package(KF5 ${KF5_VERSION} NO_MODULE REQUIRED I18n ThreadWeaver Config Service Solid Completion Plasma CoreAddons KIO)

# find_package(KF5CoreAddons ${KF5_VERSION} REQUIRED)

# Subdirectories
add_subdirectory(src)
add_subdirectory(autotests)
add_subdirectory(tests)

# Create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KF5Runner")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KF5RunnerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KF5RunnerConfig.cmake"
    INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/KF5RunnerConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/KF5RunnerConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)

install(EXPORT KF5RunnerTargets
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE KF5RunnerTargets.cmake
        NAMESPACE KF5::)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/krunner_version.h"
        DESTINATION "${INCLUDE_INSTALL_DIR}"
        COMPONENT Devel)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
