#!/bin/bash

# Exit if something fails
set -e

prefix="${XDG_DATA_HOME:-$HOME/.local/share}"
krunner_dbusdir="$prefix/krunner/dbusplugins"
services_dir="$prefix/dbus-1/services/"

mkdir -p $krunner_dbusdir
mkdir -p $services_dir

cp plasma-runner-%{APPNAMELC}.desktop $krunner_dbusdir
sed "s|%{PROJECTDIR}/%{APPNAMELC}.py|${PWD}/%{APPNAMELC}.py|" "org.kde.%{APPNAMELC}.service" > $services_dir/org.kde.%{APPNAMELC}.service

kquitapp5 krunner

