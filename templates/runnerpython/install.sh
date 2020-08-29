#!/bin/bash

# Exit if something fails
set -e

mkdir -p ~/.local/share/kservices5/krunner/dbusplugins/
mkdir -p ~/.local/share/dbus-1/services/

cp plasma-runner-%{APPNAMELC}.desktop ~/.local/share/kservices5/krunner/dbusplugins/
sed "s|%{PROJECTDIR}/%{APPNAMELC}.py|${PWD}/%{APPNAMELC}.py|" "org.kde.%{APPNAMELC}.service" > ~/.local/share/dbus-1/services/org.kde.%{APPNAMELC}.service

kquitapp5 krunner

