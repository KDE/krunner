cmake_minimum_required(VERSION 3.0)

set(KF5_VERSION "5.42.0") # handled by release scripts
set(KF5_DEP_VERSION "5.42.0") # handled by release scripts
project(KRunner VERSION ${KF5_VERSION})

# ECM setup
include(FeatureSummary)
find_package(ECM 5.42.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})


include(GenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMAddQch)
include(CMakePackageConfigHelpers)
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(KDEPackageAppTemplates)
include(ECMQtDeclareLoggingCategory)

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

ecm_setup_version(PROJECT
    VARIABLE_PREFIX KRUNNER
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/krunner_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5RunnerConfigVersion.cmake"
)

# Dependencies
set(REQUIRED_QT_VERSION 5.7.0)

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

# Subdirectories
add_subdirectory(src)
add_subdirectory(autotests)
add_subdirectory(tests)
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

    # contains list of debug categories, for kdebugsettings
install(FILES krunner.categories DESTINATION ${KDE_INSTALL_CONFDIR})


feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
