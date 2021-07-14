### %{APPNAME}

This plugin provides a simple template for a KRunner plugin using dbus.

The install script copies the Krunner config file and a dbus activation service file
to their appropriate locations.
This way the python script gets executed when KRunner
requests matches and it does not need to be autostarted.

If you want to run the plugin manually to debug it you can do the following:  
```bash
mkdir -p ~/.local/share/kservices5/krunner/dbusplugins/
cp plasma-runner-%{APPNAMELC}.desktop ~/.local/share/kservices5/krunner/dbusplugins/
kquitapp5 krunner
python3 %{APPNAMELC}.py
```

After that you should see your runner when typing `hello` in KRunner.

More information can be found here:  
https://cgit.kde.org/krunner.git/plain/src/data/org.kde.krunner1.xml  
https://techbase.kde.org/Development/Tutorials/D-Bus/Introduction


If you feel confident about your runner you can upload it to the KDE Store
https://store.kde.org/browse/cat/628/order/latest/.
