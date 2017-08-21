find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

include(ECMAddTests)

ecm_add_tests(
   runnercontexttest.cpp
   LINK_LIBRARIES Qt5::Test KF5::KIOCore KF5Runner
)

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
