project(runnermodel)

add_library(runnermodelplugin SHARED)

target_sources(runnermodelplugin PRIVATE
    runnermodel.cpp
    runnermodelplugin.cpp
)

ecm_qt_declare_logging_category(runnermodelplugin
    HEADER krunner_debug.h
    IDENTIFIER KRUNNER
    CATEGORY_NAME kf.runner.quick
)

target_link_libraries(runnermodelplugin
        Qt5::Quick
        Qt5::Qml
        Qt5::Widgets # QAction
        KF5::I18n
        KF5::Service
        KF5Runner
)

install(TARGETS runnermodelplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/runnermodel)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/runnermodel)

