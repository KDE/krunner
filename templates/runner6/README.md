Runner Template
----------------------

### Build instructions

```
cd /where/your/runner/is/installed
mkdir build
cd build
cmake -DKDE_INSTALL_PLUGINDIR=`kf5-config --qt-plugins` ..
make
make install
kquitapp5 krunner
```

After this you should see your runner in the system settings:  
`systemsettings5 kcm_plasmasearch`

You can also launch KRunner via Alt-F2 or Alt-Space and you will find your runner.

If you feel confident about your runner you can upload it to the KDE Store
https://store.kde.org/browse/cat/628/order/latest/.
