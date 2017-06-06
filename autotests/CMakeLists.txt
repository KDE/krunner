find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

include(ECMAddTests)

ecm_add_tests(
   runnercontexttest.cpp
   LINK_LIBRARIES Qt5::Test KF5::KIOCore KF5Runner
)
