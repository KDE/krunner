add_executable(runnermodeltest runnermodeltest.cpp ../src/declarative/runnermodel.cpp)
target_link_libraries(runnermodeltest Qt5::Widgets KF5::Service KF5Runner)
ecm_mark_as_test(runnermodeltest)

add_executable(runnertest runnertest.cpp)
target_link_libraries(runnertest Qt5::Widgets KF5::Runner)
ecm_mark_as_test(runnertest)
