Devhelp
=======

The Devhelp web page:

https://wiki.gnome.org/Apps/Devhelp

Professional services
---------------------

See the file: [docs/professional-services.md](docs/professional-services.md)

Installation of the Devhelp Flatpak
-----------------------------------

- [Devhelp on Flathub](https://flathub.org/apps/details/org.gnome.Devhelp)

How to contribute
-----------------

See the [HACKING](HACKING) file.

Dependencies
------------

- GLib
- GTK
- WebKitGTK
- [Amtk](https://wiki.gnome.org/Projects/Amtk)
- gsettings-desktop-schemas

Description
-----------

Devhelp is a developer tool for browsing and searching API documentation.
It provides an easy way to navigate through libraries and to search by
function, struct, or macro.

The documentation must be installed locally, so an internet connection is
not needed to use Devhelp.

Devhelp works natively with GTK-Doc, so the GTK and GNOME libraries are
well supported. But other development platforms can be supported as well,
as long as the API documentation is available in HTML and a `*.devhelp2`
index file is generated.

Devhelp integrates with other applications such as Glade, Builder or
Anjuta, and plugins are available for different text editors (gedit, Vim,
Emacs, Geany, …).

Integration with other developer tools
--------------------------------------

Devhelp provides some command line options, such as `--search` and
`--search-assistant`. A text editor plugin can for example launch the command
`devhelp --search function_name` when a keyboard shortcut is pressed, with the
`function_name` under the cursor.

Devhelp also provides a shared library, to integrate the GTK widgets inside an
IDE. It is used for example by Builder and Anjuta.

For the `--search` command line option, see the class description of
DhKeywordModel, the search string supports additional features useful for IDEs
or other developer tools.

Other documentation
-------------------

- There is user documentation written in the Mallard format in the `help/C/`
  directory. You can open that documentation with the `yelp help/C/` command,
  or pressing F1 in the Devhelp application.

- There is an API reference manual for the libdevhelp that can be built with
  GTK-Doc, see the `gtk_doc` build option.
