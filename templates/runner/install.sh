#!/bin/bash

# Exit if something fails
set -e

mkdir -p build
cd build

cmake -DKDE_INSTALL_PLUGINDIR=`kf5-config --qt-plugins` -DCMAKE_BUILD_TYPE=Release  ..
make -j$(nproc)
sudo make install

# KRunner needs to be restarted for the changes to be applied
# we can just kill it and it will be started when the shortcut is invoked
kquitapp5 krunner
