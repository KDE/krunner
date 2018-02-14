add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_org.kde.%{APPNAMELC}\")

set(%{APPNAMELC}_SRCS %{APPNAMELC}.cpp)

add_library(krunner_%{APPNAMELC} MODULE ${%{APPNAMELC}_SRCS})
target_link_libraries(krunner_%{APPNAMELC} KF5::Runner KF5::I18n)

install(TARGETS krunner_%{APPNAMELC} DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES plasma-runner-%{APPNAMELC}.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
