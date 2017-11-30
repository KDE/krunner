project(runnermodel)

set(runnermodel_SRCS
    runnermodel.cpp
    runnermodelplugin.cpp
    )
ecm_qt_declare_logging_category(runnermodel_SRCS HEADER krunner_debug.h IDENTIFIER KRUNNER CATEGORY_NAME org.kde.krunner)

add_library(runnermodelplugin SHARED ${runnermodel_SRCS})
target_link_libraries(runnermodelplugin
        Qt5::Quick
        Qt5::Qml
        Qt5::Widgets # QAction
        KF5::I18n
        KF5::Service
        KF5Runner
)

install(TARGETS runnermodelplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/runnermodel)
install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/runnermodel)

