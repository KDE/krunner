#!/bin/bash

# Exit if something fails
set -e


if [[ -z "$XDG_DATA_HOME" ]]; then
    prefix=~/.local/share
else
    prefix="$XDG_DATA_HOME"
fi

mkdir -p $prefix/kservices5/krunner/dbusplugins/
mkdir -p $prefix/dbus-1/services/

cp plasma-runner-%{APPNAMELC}.desktop $prefix/krunner/dbusplugins/
sed "s|%{PROJECTDIR}/%{APPNAMELC}.py|${PWD}/%{APPNAMELC}.py|" "org.kde.%{APPNAMELC}.service" > $prefix/dbus-1/services/org.kde.%{APPNAMELC}.service

kquitapp5 krunner

