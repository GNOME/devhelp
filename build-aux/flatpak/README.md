Flatpak for Devhelp
===================

 - `org.gnome.Devhelp.json`: the Flatpak-builder manifest for Devhelp
 - `run-flatpak.sh`: a utility script for building, running, and uninstalling
   the Devhelp Flatpak

If you want to build a Flatpak version of Devhelp:

 1. make sure to have `flatpak` and `flatpak-builder` installed
 2. `./run-flatpak build --app-id=org.gnome.Devhelp` to build and install
 3. `./run-flatpak run --app-id=org.gnome.Devhelp` to run
 4. `./run-flatpak clean --app-id=org.gnome.Devhelp` to uninstall
