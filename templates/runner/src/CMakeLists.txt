add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_org.kde.%{APPNAMELC}\")

set(%{APPNAMELC}_SRCS %{APPNAMELC}.cpp)

add_library(%{APPNAMELC} MODULE ${%{APPNAMELC}_SRCS})
kcoreaddons_desktop_to_json(%{APPNAMELC} plasma-runner-%{APPNAMELC}.desktop)
target_link_libraries(%{APPNAMELC} KF5::Runner KF5::I18n)

install(TARGETS %{APPNAMELC} DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/krunner)

install(FILES plasma-runner-%{APPNAMELC}.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
