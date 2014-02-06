find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

include(ECMMarkAsTest)

macro(krunner_tests)
    foreach(_testname ${ARGN})
        add_executable(${_testname} ${_testname}.cpp)
        target_link_libraries(${_testname} Qt5::Test KF5::KIOCore KF5Runner)
        add_test(krunner-${_testname} ${_testname})
        ecm_mark_as_test(${_testname})
    endforeach(_testname)
endmacro()

krunner_tests(
   runnercontexttest
)
