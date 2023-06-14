#!/bin/bash

# Exit if something fails
set -e

if [[ -z "$XDG_DATA_HOME" ]]; then
    prefix=~/.local/share
else
    prefix="$XDG_DATA_HOME"
fi

rm $prefix/kservices6/krunner/dbusplugins/plasma-runner-%{APPNAMELC}.desktop
rm $prefix/dbus-1/services/org.kde.%{APPNAMELC}.service
kquitapp6 krunner

