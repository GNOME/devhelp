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

from gi.repository import GObject, Gio, Gtk, Gedit
import os
import gettext

class DevhelpAppActivatable(GObject.Object, Gedit.AppActivatable):

    app = GObject.Property(type=Gedit.App)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        self.app.add_accelerator("F2", "win.devhelp", None)

        # Translate actions below, hardcoding domain here to avoid complications now
        _ = lambda s: gettext.dgettext('devhelp', s)

        self.menu_ext = self.extend_menu("tools-section")
        item = Gio.MenuItem.new(_("Show API Documentation"), "win.devhelp")
        self.menu_ext.prepend_menu_item(item)

    def do_deactivate(self):
        self.app.remove_accelerator("win.devhelp", None)
        self.menu_ext = None

class DevhelpWindowActivatable(GObject.Object, Gedit.WindowActivatable):

    window = GObject.Property(type=Gedit.Window)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        action = Gio.SimpleAction(name="devhelp")
        action.connect('activate', lambda a, p: self.do_devhelp(self.window.get_active_document()))
        self.window.add_action(action)

    def do_deactivate(self):
        self.window.remove_action("devhelp")

    def do_update_state(self):
        self.window.lookup_action("devhelp").set_enabled(self.window.get_active_document() is not None)

    def _is_word_separator(self, c):
        return not (c.isalnum() or c == '_')

    def do_devhelp(self, document):
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
                # FIXME: We need a dbus interface for devhelp soon...
                os.spawnlp(os.P_NOWAIT, 'devhelp', 'devhelp', '-s', text)

# ex:ts=4:et:
