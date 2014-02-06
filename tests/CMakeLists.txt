add_executable(runnermodeltest runnermodeltest.cpp ../src/declarative/runnermodel.cpp)
target_link_libraries(runnermodeltest Qt5::Widgets KF5::Service KF5Runner)
ecm_mark_as_test(runnermodeltest)
