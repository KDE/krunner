find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

include(ECMAddTests)

if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND
        EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.76.0)
    ecm_add_tests(
            runnercontexttest.cpp
            LINK_LIBRARIES Qt5::Test KF5::KIOCore KF5Runner
    )
endif()

ecm_add_tests(
   dbusrunnertest.cpp
   LINK_LIBRARIES Qt5::Test KF5::KIOCore KF5Runner Qt5::Widgets
)

set(demoapp_SRCS testremoterunner.cpp)
qt5_add_dbus_adaptor(demoapp_SRCS "../src/data/org.kde.krunner1.xml" testremoterunner.h TestRemoteRunner)
add_executable(testremoterunner ${demoapp_SRCS})
target_link_libraries(testremoterunner
    Qt5::DBus
    KF5::Runner
)
