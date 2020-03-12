cmake_minimum_required(VERSION 3.5)

set(KF5_VERSION "5.69.0") # handled by release scripts
set(KF5_DEP_VERSION "5.68.0") # handled by release scripts
project(KRunner VERSION ${KF5_VERSION})

# ECM setup
include(FeatureSummary)
find_package(ECM 5.68.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(ECMGenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMAddQch)
include(CMakePackageConfigHelpers)
include(KDEPackageAppTemplates)
include(ECMQtDeclareLoggingCategory)

set(EXCLUDE_DEPRECATED_BEFORE_AND_AT 0 CACHE STRING "Control the range of deprecated API excluded from the build [default=0].")

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

ecm_setup_version(PROJECT
    VARIABLE_PREFIX KRUNNER
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/krunner_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5RunnerConfigVersion.cmake"
)

# Dependencies
set(REQUIRED_QT_VERSION 5.12.0)

find_package(Qt5 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Gui Widgets Quick)

find_package(KF5Config ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5CoreAddons ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5I18n ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5KIO ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Service ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Plasma ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5ThreadWeaver ${KF5_DEP_VERSION} REQUIRED)

set(KRunner_AUTOMOC_MACRO_NAMES "K_EXPORT_PLASMA_RUNNER" "K_EXPORT_RUNNER_CONFIG")
if(NOT CMAKE_VERSION VERSION_LESS "3.10.0")
    # CMake 3.9+ warns about automoc on files without Q_OBJECT, and doesn't know about other macros.
    # 3.10+ lets us provide more macro names that require automoc.
    list(APPEND CMAKE_AUTOMOC_MACRO_NAMES ${KRunner_AUTOMOC_MACRO_NAMES})
endif()
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050d00)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054300)
add_definitions(-DPLASMA_DISABLE_DEPRECATED_BEFORE_AND_AT=0x050500) # needed because we use Plasma::Package in the API
add_definitions(-DQT_NO_FOREACH)

# Subdirectories
add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()
add_subdirectory(templates)

# Create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KF5Runner")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KF5Runner_QCH
        FILE KF5RunnerQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KF5RunnerQchTargets.cmake\")")
endif()

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
        DESTINATION "${KF5_INCLUDE_INSTALL_DIR}"
        COMPONENT Devel)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
