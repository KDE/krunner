add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_org.kde.%{APPNAMELC}\")

add_library(%{APPNAMELC} MODULE)

target_sources(%{APPNAMELC} PRIVATE
    %{APPNAMELC}.cpp
)

target_link_libraries(%{APPNAMELC} KF6::Runner KF6::I18n)

install(TARGETS %{APPNAMELC} DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/krunner)
