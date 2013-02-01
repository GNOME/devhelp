<?xml version="1.0" encoding="UTF-8"?>

<!--
  Copyright (C) 2010 Imendio AB
  Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the licence, or (at
  your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02110-1301
  USA
-->

<interface>
  <requires lib="gtk+" version="3.0"/>

  <menu id="app-menu">
    <section>
      <item>
        <attribute name="label" translatable="yes">New window</attribute>
        <attribute name="action">app.new-window</attribute>
        <attribute name="accel">&lt;Primary&gt;n</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name="label" translatable="yes">Preferences</attribute>
        <attribute name="action">app.preferences</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name="label" translatable="yes">About Devhelp</attribute>
        <attribute name="action">app.about</attribute>
      </item>
      <item>
        <attribute name="label" translatable="yes">Quit</attribute>
        <attribute name="action">app.quit</attribute>
        <attribute name="accel">&lt;Primary&gt;q</attribute>
      </item>
    </section>
  </menu>

  <menu id="window-menu">
    <submenu>
      <attribute name="label" translatable="yes">_Window</attribute>
      <section>
        <item>
          <attribute name="label" translatable="yes">New _Tab</attribute>
          <attribute name="action">win.new-tab</attribute>
          <attribute name="accel">&lt;Primary&gt;t</attribute>
          <attribute name="use-underline">True</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name="label" translatable="yes">_Print</attribute>
          <attribute name="action">win.print</attribute>
          <attribute name="accel">&lt;Primary&gt;p</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name="label" translatable="yes">_Close</attribute>
          <attribute name="action">win.close</attribute>
          <attribute name="accel">&lt;Primary&gt;w</attribute>
        </item>
      </section>
    </submenu>
    <submenu>
      <attribute name="label" translatable="yes">_Edit</attribute>
      <section>
        <item>
          <attribute name="label" translatable="yes">_Copy</attribute>
          <attribute name="action">win.copy</attribute>
          <attribute name="accel">&lt;Primary&gt;c</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name="label" translatable="yes">_Find</attribute>
          <attribute name="action">win.find</attribute>
          <attribute name="accel">&lt;Primary&gt;f</attribute>
        </item>
        <item>
          <attribute name="label" translatable="yes">Find _Next</attribute>
          <attribute name="action">win.find-next</attribute>
          <attribute name="accel">&lt;Primary&gt;g</attribute>
        </item>
        <item>
          <attribute name="label" translatable="yes">Find _Previous</attribute>
          <attribute name="action">win.find-previous</attribute>
          <attribute name="accel">&lt;Primary&gt;&lt;Alt&gt;g</attribute>
        </item>
      </section>
    </submenu>
    <submenu>
      <attribute name="label" translatable="yes">_View</attribute>
      <section>
        <item>
          <attribute name="label" translatable="yes">_Larger text</attribute>
          <attribute name="action">win.zoom-in</attribute>
          <attribute name="accel">&lt;Primary&gt;plus</attribute>
        </item>
        <item>
          <attribute name="label" translatable="yes">S_maller text</attribute>
          <attribute name="action">win.zoom-out</attribute>
          <attribute name="accel">&lt;Primary&gt;minus</attribute>
        </item>
        <item>
          <attribute name="label" translatable="yes">_Normal size</attribute>
          <attribute name="action">win.zoom-default</attribute>
          <attribute name="accel">&lt;Primary&gt;0</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name="label" translatable="yes">Fullscreen</attribute>
          <attribute name="action">win.fullscreen</attribute>
          <attribute name="accel">F11</attribute>
        </item>
      </section>
    </submenu>
    <submenu>
      <attribute name="label" translatable="yes">_Go</attribute>
      <section>
        <item>
          <attribute name="label" translatable="yes">_Back</attribute>
          <attribute name="action">win.go-back</attribute>
          <attribute name="accel">&lt;Alt&gt;Left</attribute>
        </item>
        <item>
          <attribute name="label" translatable="yes">_Forward</attribute>
          <attribute name="action">win.go-forward</attribute>
          <attribute name="accel">&lt;Alt&gt;Right</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name="label" translatable="yes">_Search Tab</attribute>
          <attribute name="action">win.go-search-tab</attribute>
          <attribute name="accel">&lt;Primary&gt;s</attribute>
        </item>
        <item>
          <attribute name="label" translatable="yes">_Contents Tab</attribute>
          <attribute name="action">win.go-contents-tab</attribute>
          <attribute name="accel">&lt;Primary&gt;b</attribute>
        </item>
      </section>
    </submenu>
  </menu>

  <object class="GtkToolbar" id="toolbar">
    <property name="visible">True</property>
    <property name="expand">False</property>
    <child>
      <object class="GtkToolButton" id="back-button">
	<property name="is-important">True</property>
        <property name="stock_id">gtk-go-back</property>
        <property name="action_name">win.go-back</property>
        <property name="use_action_appearance">True</property>
        <property name="tooltip_text" translatable="yes">Go to the previous page</property>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="homogeneous">True</property>
      </packing>
    </child>
    <child>
      <object class="GtkToolButton" id="forward-button">
	<property name="is-important">True</property>
        <property name="stock_id">gtk-go-forward</property>
        <property name="action_name">win.go-forward</property>
        <property name="use_action_appearance">True</property>
        <property name="tooltip_text" translatable="yes">Go to the next page</property>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="homogeneous">True</property>
      </packing>
    </child>
    <child>
      <object class="GtkToolButton" id="zoom-out-button">
        <property name="stock_id">gtk-zoom-out</property>
        <property name="action_name">win.zoom-out</property>
        <property name="use_action_appearance">True</property>
        <property name="tooltip_text" translatable="yes">Decrease the text size</property>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="homogeneous">True</property>
      </packing>
    </child>
    <child>
      <object class="GtkToolButton" id="zoom-in-button">
        <property name="stock_id">gtk-zoom-in</property>
        <property name="action_name">win.zoom-in</property>
        <property name="use_action_appearance">True</property>
        <property name="tooltip_text" translatable="yes">Increase the text size</property>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="homogeneous">True</property>
      </packing>
    </child>
  </object>

  <object class="GtkToolbar" id="fullscreen-toolbar">
    <property name="visible">True</property>
    <property name="expand">False</property>
    <child>
      <object class="GtkToolButton" id="fullscreen-back-button">
	<property name="is-important">True</property>
        <property name="stock_id">gtk-go-back</property>
        <property name="action_name">win.go-back</property>
        <property name="use_action_appearance">True</property>
        <property name="tooltip_text" translatable="yes">Go to the previous page</property>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="homogeneous">True</property>
      </packing>
    </child>
    <child>
      <object class="GtkToolButton" id="fullscreen-forward-button">
	<property name="is-important">True</property>
        <property name="stock_id">gtk-go-forward</property>
        <property name="action_name">win.go-forward</property>
        <property name="use_action_appearance">True</property>
        <property name="tooltip_text" translatable="yes">Go to the next page</property>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="homogeneous">True</property>
      </packing>
    </child>
    <child>
      <object class="GtkSeparatorToolItem" id="separator1" />
      <packing>
        <property name="expand">False</property>
      </packing>
    </child>
    <child>
      <object class="GtkToolButton" id="fullscreen-zoom-out-button">
        <property name="stock_id">gtk-zoom-out</property>
        <property name="action_name">win.zoom-out</property>
        <property name="use_action_appearance">True</property>
        <property name="tooltip_text" translatable="yes">Decrease the text size</property>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="homogeneous">True</property>
      </packing>
    </child>
    <child>
      <object class="GtkToolButton" id="fullscreen-zoom-in-button">
        <property name="stock_id">gtk-zoom-in</property>
        <property name="action_name">win.zoom-in</property>
        <property name="use_action_appearance">True</property>
        <property name="tooltip_text" translatable="yes">Increase the text size</property>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="homogeneous">True</property>
      </packing>
    </child>
    <child>
      <object class="GtkSeparatorToolItem" id="separator2">
        <property name="draw">False</property>
      </object>
      <packing>
        <property name="expand">True</property>
      </packing>
    </child>
    <child>
      <object class="GtkToolButton" id="fullscreen-leave-fullscreen-button">
	<property name="is-important">True</property>
        <property name="stock_id">gtk-leave-fullscreen</property>
	<property name="visible">True</property>
        <property name="action_name">win.leave-fullscreen</property>
        <property name="use_action_appearance">True</property>
      </object>
      <packing>
        <property name="expand">False</property>
        <property name="homogeneous">True</property>
      </packing>
    </child>
  </object>

  <object class="GtkListStore" id="bookshelf_store">
    <columns>
      <!-- column-name enabled -->
      <column type="gboolean"/>
      <!-- column-name title -->
      <column type="gchararray"/>
      <!-- column-name book -->
      <column type="GObject"/>
      <!-- column-name weight -->
      <column type="gint"/>
      <!-- column-name inconsistent -->
      <column type="gboolean"/>
    </columns>
  </object>

  <object class="GtkDialog" id="preferences_dialog">
    <property name="border_width">5</property>
    <property name="title" translatable="yes">Preferences</property>
    <property name="default_width">500</property>
    <property name="default_height">400</property>
    <property name="type_hint">dialog</property>
    <child internal-child="vbox">
      <object class="GtkVBox" id="dialog-vbox1">
        <property name="visible">True</property>
        <property name="spacing">2</property>
        <child>
          <object class="GtkAlignment" id="alignment1">
            <property name="visible">True</property>
            <property name="right_padding">6</property>
            <child>
              <object class="GtkNotebook" id="notebook1">
                <property name="visible">True</property>
                <property name="can_focus">True</property>
                <child>
                  <object class="GtkVBox" id="outer_vbox1">
                    <property name="visible">True</property>
                    <child>
                      <object class="GtkAlignment" id="alignment3">
                        <property name="visible">True</property>
                        <property name="top_padding">8</property>
                        <property name="bottom_padding">8</property>
                        <property name="left_padding">8</property>
                        <property name="right_padding">8</property>
                        <child>
                          <object class="GtkVBox" id="vbox1">
                            <property name="visible">True</property>
                            <property name="spacing">6</property>
                            <child>
                              <object class="GtkCheckButton" id="bookshelf_group_by_language_button">
                                <property name="label" translatable="yes">_Group by language</property>
                                <property name="visible">True</property>
                                <property name="can_focus">True</property>
                                <property name="receives_default">False</property>
                                <property name="use_underline">True</property>
                                <property name="draw_indicator">True</property>
                              </object>
                              <packing>
                                <property name="expand">False</property>
                                <property name="fill">False</property>
                                <property name="position">0</property>
                              </packing>
                            </child>
                            <child>
                              <object class="GtkScrolledWindow" id="scrolledwindow1">
                                <property name="visible">True</property>
                                <property name="can_focus">True</property>
                                <property name="hscrollbar_policy">automatic</property>
                                <property name="vscrollbar_policy">automatic</property>
                                <property name="shadow-type">in</property>
                                <child>
                                  <object class="GtkTreeView" id="bookshelf_treeview">
                                    <property name="visible">True</property>
                                    <property name="can_focus">True</property>
                                    <property name="model">bookshelf_store</property>
                                    <property name="headers_clickable">False</property>
                                    <property name="search_column">0</property>
                                    <child>
                                      <object class="GtkTreeViewColumn" id="treeviewcolumn1">
                                        <property name="min_width">60</property>
                                        <property name="title" translatable="yes">Enabled</property>
                                        <property name="expand">True</property>
                                        <child>
                                          <object class="GtkCellRendererToggle" id="bookshelf_enabled_toggle">
                                            <property name="width">60</property>
                                          </object>
                                          <attributes>
                                            <attribute name="active">0</attribute>
                                            <attribute name="inconsistent">4</attribute>
                                          </attributes>
                                        </child>
                                      </object>
                                    </child>
                                    <child>
                                      <object class="GtkTreeViewColumn" id="treeviewcolumn2">
                                        <property name="title" translatable="yes">Title</property>
                                        <property name="expand">True</property>
                                        <child>
                                          <object class="GtkCellRendererText" id="bookshelf_title_text"/>
                                          <attributes>
                                            <attribute name="text">1</attribute>
                                            <attribute name="weight">3</attribute>
                                          </attributes>
                                        </child>
                                      </object>
                                    </child>
                                  </object>
                                </child>
                              </object>
                              <packing>
                                <property name="position">2</property>
                              </packing>
                            </child>
                          </object>
                        </child>
                      </object>
                      <packing>
                        <property name="position">0</property>
                      </packing>
                    </child>
                  </object>
                </child>
                <child type="tab">
                  <object class="GtkLabel" id="label_bookshelf">
                    <property name="visible">True</property>
                    <property name="label" translatable="yes">Book Shelf</property>
                  </object>
                  <packing>
                    <property name="tab_fill">False</property>
                  </packing>
                </child>
                <child>
                  <object class="GtkVBox" id="outer_vbox2">
                    <property name="visible">True</property>
                    <child>
                      <object class="GtkAlignment" id="alignment2">
                        <property name="visible">True</property>
                        <property name="top_padding">8</property>
                        <property name="bottom_padding">8</property>
                        <property name="left_padding">8</property>
                        <property name="right_padding">8</property>
                        <child>
                          <object class="GtkVBox" id="vbox4">
                            <property name="visible">True</property>
                            <property name="spacing">6</property>
                            <child>
                              <object class="GtkCheckButton" id="system_fonts_button">
                                <property name="label" translatable="yes">_Use system fonts</property>
                                <property name="visible">True</property>
                                <property name="can_focus">True</property>
                                <property name="receives_default">False</property>
                                <property name="use_underline">True</property>
                                <property name="active">True</property>
                                <property name="draw_indicator">True</property>
                              </object>
                              <packing>
                                <property name="expand">False</property>
                                <property name="fill">False</property>
                                <property name="position">0</property>
                              </packing>
                            </child>
                            <child>
                              <object class="GtkTable" id="fonts_table">
                                <property name="visible">True</property>
                                <property name="sensitive">False</property>
                                <property name="n_rows">2</property>
                                <property name="n_columns">2</property>
                                <property name="column_spacing">6</property>
                                <property name="row_spacing">6</property>
                                <child>
                                  <object class="GtkLabel" id="label4">
                                    <property name="visible">True</property>
                                    <property name="xalign">0</property>
                                    <property name="label" translatable="yes">_Variable width: </property>
                                    <property name="use_underline">True</property>
                                    <property name="mnemonic_widget">variable_font_button</property>
                                  </object>
                                  <packing>
                                    <property name="x_options"></property>
                                    <property name="y_options"></property>
                                  </packing>
                                </child>
                                <child>
                                  <object class="GtkLabel" id="label5">
                                    <property name="visible">True</property>
                                    <property name="xalign">0</property>
                                    <property name="label" translatable="yes">_Fixed width:</property>
                                    <property name="use_underline">True</property>
                                    <property name="mnemonic_widget">fixed_font_button</property>
                                  </object>
                                  <packing>
                                    <property name="top_attach">1</property>
                                    <property name="bottom_attach">2</property>
                                    <property name="x_options">GTK_FILL</property>
                                    <property name="y_options"></property>
                                  </packing>
                                </child>
                                <child>
                                  <object class="GtkFontButton" id="variable_font_button">
                                    <property name="visible">True</property>
                                    <property name="can_focus">True</property>
                                    <property name="receives_default">True</property>
                                    <property name="use_font">True</property>
                                  </object>
                                  <packing>
                                    <property name="left_attach">1</property>
                                    <property name="right_attach">2</property>
                                    <property name="x_options">GTK_FILL</property>
                                    <property name="y_options">GTK_FILL</property>
                                  </packing>
                                </child>
                                <child>
                                  <object class="GtkFontButton" id="fixed_font_button">
                                    <property name="visible">True</property>
                                    <property name="can_focus">True</property>
                                    <property name="receives_default">True</property>
                                    <property name="font_name">Monospace 12</property>
                                    <property name="use_font">True</property>
                                  </object>
                                  <packing>
                                    <property name="left_attach">1</property>
                                    <property name="right_attach">2</property>
                                    <property name="top_attach">1</property>
                                    <property name="bottom_attach">2</property>
                                    <property name="x_options"></property>
                                    <property name="y_options"></property>
                                  </packing>
                                </child>
                              </object>
                              <packing>
                                <property name="expand">False</property>
                                <property name="fill">False</property>
                                <property name="position">1</property>
                              </packing>
                            </child>
                          </object>
                        </child>
                      </object>
                      <packing>
                        <property name="position">0</property>
                      </packing>
                    </child>
                  </object>
                  <packing>
                    <property name="position">1</property>
                  </packing>
                </child>
                <child type="tab">
                  <object class="GtkLabel" id="label_fonts">
                    <property name="visible">True</property>
                    <property name="label" translatable="yes">Fonts</property>
                  </object>
                  <packing>
                    <property name="position">1</property>
                    <property name="tab_fill">False</property>
                  </packing>
                </child>
                <child>
                  <placeholder/>
                </child>
                <child type="tab">
                  <placeholder/>
                </child>
              </object>
            </child>
          </object>
          <packing>
            <property name="position">1</property>
          </packing>
        </child>
        <child internal-child="action_area">
          <object class="GtkHButtonBox" id="dialog-action_area1">
            <property name="visible">True</property>
            <property name="layout_style">end</property>
            <child>
              <object class="GtkButton" id="preferences_close_button">
                <property name="label">gtk-close</property>
                <property name="visible">True</property>
                <property name="can_focus">True</property>
                <property name="can_default">True</property>
                <property name="receives_default">False</property>
                <property name="use_stock">True</property>
              </object>
              <packing>
                <property name="expand">False</property>
                <property name="fill">False</property>
                <property name="position">0</property>
              </packing>
            </child>
          </object>
          <packing>
            <property name="expand">False</property>
            <property name="pack_type">end</property>
            <property name="position">0</property>
          </packing>
        </child>
      </object>
    </child>
    <action-widgets>
      <action-widget response="-7">preferences_close_button</action-widget>
    </action-widgets>
  </object>

</interface>
