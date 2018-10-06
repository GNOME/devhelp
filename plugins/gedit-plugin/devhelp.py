# -*- coding: utf-8 py-indent-offset: 4 -*-
#
#    Gedit devhelp plugin
#
#    Copyright (C) 2006 Imendio AB
#    Copyright (C) 2011 Red Hat, Inc.
#
#    Author: Richard Hult <richard@imendio.com>
#    Author: Dan Williams <dcbw@redhat.com>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, see <http://www.gnu.org/licenses/>.

from gi.repository import GObject, Gio, Gtk, Gedit, Gdk
import os
import gettext

class DevhelpAppActivatable(GObject.Object, Gedit.AppActivatable):

    app = GObject.Property(type=Gedit.App)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        self.app.add_accelerator("F2", "win.devhelp", None)
        self.app.add_accelerator("<Shift>F2", "win.codesearch", None)

        # Translate actions below, hardcoding domain here to avoid complications now
        _ = lambda s: gettext.dgettext('devhelp', s)

        self.menu_ext = self.extend_menu("tools-section")
        item = Gio.MenuItem.new(_("Show API Documentation"), "win.devhelp")
        self.menu_ext.prepend_menu_item(item)
        item = Gio.MenuItem.new(_("Find source on codesearch.debian.net"), "win.codesearch")
        self.menu_ext.prepend_menu_item(item)

    def do_deactivate(self):
        self.app.remove_accelerator("win.devhelp", None)
        self.app.remove_accelerator("win.codesearch", None)
        self.menu_ext = None

class DevhelpWindowActivatable(GObject.Object, Gedit.WindowActivatable):

    window = GObject.Property(type=Gedit.Window)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        action = Gio.SimpleAction(name="devhelp")
        action.connect('activate', lambda a, p: self.do_devhelp(self.window.get_active_document()))
        self.window.add_action(action)
        action = Gio.SimpleAction(name="codesearch")
        action.connect('activate', lambda a, p: self.do_devhelp(self.window.get_active_document(), True))
        self.window.add_action(action)

    def do_deactivate(self):
        self.window.remove_action("devhelp")
        self.window.remove_action("codesearch")

    def do_update_state(self):
        self.window.lookup_action("devhelp").set_enabled(self.window.get_active_document() is not None)
        self.window.lookup_action("codesearch").set_enabled(self.window.get_active_document() is not None)

    def _is_word_separator(self, c):
        return not (c.isalnum() or c == '_')

    def code_search(self, text):
        # Regex is (?mi:^(?:#define\s+)?text)\s*\((?m:([^;]+)$)
        # it matches both function and macro definitions
        uri = "https://codesearch.debian.net/search?q=%28%3Fmi%3A%5E%28%3F%3A%23define%5Cs%2B%29%3F"+text+"%29%5Cs*%5C%28%28%3Fm%3A%28%5B%5E%3B%5D%2B%29%24%29"
        Gtk.show_uri_on_window(self.window, uri, Gdk.CURRENT_TIME)

    def do_devhelp(self, document, is_codesearch = False):
        # Get the word at the cursor
        start = document.get_iter_at_mark(document.get_insert())
        end = start.copy()

        # If just after a word, move back into it
        c = start.get_char()
        if self._is_word_separator(c):
            start.backward_char()

        # Go backward
        while True:
            c = start.get_char()
            if not self._is_word_separator(c):
                if not start.backward_char():
                    break
            else:
                start.forward_char()
                break

        # Go forward
        while True:
            c = end.get_char()
            if not self._is_word_separator(c):
                if not end.forward_char():
                    break
            else:
                break

        if end.compare(start) > 0:
            text = document.get_text(start,end,False).strip()
            if text:
                if is_codesearch:
                    self.code_search(text)
                    return;
                # FIXME: We need a dbus interface for devhelp soon...
                os.spawnlp(os.P_NOWAIT, 'devhelp', 'devhelp', '-s', text)

# ex:ts=4:et:
