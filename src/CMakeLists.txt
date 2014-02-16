add_subdirectory(declarative)

add_library(KF5Runner
    abstractrunner.cpp
#     scripting/runnerscript.cpp
    private/runnerjobs.cpp
    querymatch.cpp
    runnercontext.cpp
    runnermanager.cpp
    runnersyntax.cpp)

generate_export_header(KF5Runner BASE_NAME KRunner)
add_library(KF5::Runner ALIAS KF5Runner)

target_include_directories(KF5Runner INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/KRunner>")
target_include_directories(KF5Runner PUBLIC "$<BUILD_INTERFACE:${KRunner_BINARY_DIR};${CMAKE_CURRENT_BINARY_DIR}>")


target_link_libraries(KF5Runner
    PUBLIC
        Qt5::Core
        KF5::Plasma # Must be public because abstractrunner.h needs plasma/version.h
    PRIVATE
        Qt5::Gui
        KF5::ConfigCore
        KF5::Service
        KF5::I18n
        KF5::Completion
        KF5::ThreadWeaver
        KF5::Solid
        KF5::CoreAddons #KShell
        KF5::KIOCore #KProtocolInfo
)

set_target_properties(KF5Runner
    PROPERTIES VERSION ${KRUNNER_VERSION_STRING}
    SOVERSION ${KRUNNER_SOVERSION}
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
        ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES ${KRunner_CamelCase_HEADERS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/KRunner/KRunner
        COMPONENT Devel)

install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/krunner_export.h
            ${KRunner_HEADERS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/KRunner/krunner
        COMPONENT Devel)

install(FILES
   data/servicetypes/plasma-runner.desktop
   DESTINATION ${SERVICETYPES_INSTALL_DIR})

include(ECMGeneratePriFile)
ecm_generate_pri_file(
    BASE_NAME KRunner
    LIB_NAME KF5Runner
    DEPS "core"
    FILENAME_VAR PRI_FILENAME
)
install(FILES ${PRI_FILENAME}
        DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
