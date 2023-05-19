add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_org.kde.%{APPNAMELC}\")

kcoreaddons_add_plugin(%{APPNAMELC} INSTALL_NAMESPACE "kf6/krunner")

target_sources(%{APPNAMELC} PRIVATE
    %{APPNAMELC}.cpp
)

target_link_libraries(%{APPNAMELC} KF6::Runner KF6::I18n)
