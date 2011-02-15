# -*- coding: utf-8 py-indent-offset: 4 -*-
#
#    Gedit devhelp plugin
#    Copyright (C) 2006 Imendio AB
#    Copyright (C) 2011 Red Hat, Inc.
#
#    Author: Richard Hult <richard@imendio.com>
#    Author: Dan Williams <dcbw@redhat.com>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

from gi.repository import GObject, Gtk, Gedit
import os
import gettext

ui_str = """
<ui>
  <menubar name="MenuBar">
    <menu name="ToolsMenu" action="Tools">
      <placeholder name="ToolsOps_5">
        <menuitem name="Devhelp" action="Devhelp"/>
      </placeholder>
    </menu>
  </menubar>
</ui>
"""

class DevhelpPlugin(GObject.Object, Gedit.WindowActivatable):
    __gtype_name__ = "DevhelpPlugin"

    window = GObject.property(type=GObject.GObject)

    def __init__(self):
        GObject.Object.__init__(self)

    def do_activate(self):
        self._insert_menu()

    def do_deactivate(self):
        self._remove_menu()

    def _remove_menu(self):
        manager = self.window.get_ui_manager()
        manager.remove_ui(self._ui_id)
        manager.remove_action_group(self._action_group)
        self._action_group = None
        manager.ensure_update()

    def _insert_menu(self):
        manager = self.window.get_ui_manager()

	# Translate actions below, hardcoding domain here to avoid complications now
	_ = lambda s: gettext.dgettext('devhelp', s);

        self._action_group = Gtk.ActionGroup(name="GeditDevhelpPluginActions")
	self._action_group.add_actions([('Devhelp', None,
	                                 _('Show API Documentation'),
	                                 'F2',
	                                 _('Show API Documentation for the word at the cursor'),
	                                 lambda a, w: self.do_devhelp(w.get_active_document()))],
	                                 self.window)
        manager.insert_action_group(self._action_group, -1)
        self._ui_id = manager.add_ui_from_string(ui_str)

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
