#!/bin/bash

# SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>
# SPDX-License-Identifier: LGPL-2.1-or-later

# Exit if something fails
set -e

if [[ -z "$XDG_DATA_HOME" ]]; then
    prefix=~/.local/share
else
    prefix="$XDG_DATA_HOME"
fi

rm $prefix/kservices5/krunner/dbusplugins/plasma-runner-%{APPNAMELC}.desktop
rm $prefix/dbus-1/services/org.kde.%{APPNAMELC}.service
kquitapp5 krunner

