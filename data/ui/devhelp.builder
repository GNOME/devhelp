<?xml version="1.0"?>
<interface>
  <!-- interface-requires gtk+ 2.12 -->
  <!-- interface-naming-policy toplevel-contextual -->
  <object class="GtkListStore" id="book_manager_store">
    <columns>
      <!-- column-name enabled -->
      <column type="gboolean"/>
      <!-- column-name title -->
      <column type="gchararray"/>
      <!-- column-name book -->
      <column type="gpointer"/>
    </columns>
  </object>
  <object class="GtkDialog" id="preferences_dialog">
    <property name="border_width">5</property>
    <property name="title" translatable="yes">Preferences</property>
    <property name="default_width">500</property>
    <property name="default_height">300</property>
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
                </child>
                <child type="tab">
                  <object class="GtkLabel" id="label1">
                    <property name="visible">True</property>
                    <property name="label" translatable="yes">Fonts</property>
                    <attributes>
                      <attribute name="weight" value="bold"/>
                    </attributes>
                  </object>
                  <packing>
                    <property name="tab_fill">False</property>
                  </packing>
                </child>
                <child>
                  <object class="GtkVBox" id="outer_vbox2">
                    <property name="visible">True</property>
                    <child>
                      <object class="GtkAlignment" id="alignment3">
                        <property name="visible">True</property>
                        <property name="top_padding">8</property>
                        <property name="bottom_padding">8</property>
                        <property name="left_padding">8</property>
                        <property name="right_padding">8</property>
                        <child>
                          <object class="GtkScrolledWindow" id="scrolledwindow1">
                            <property name="visible">True</property>
                            <property name="can_focus">True</property>
                            <property name="hscrollbar_policy">automatic</property>
                            <property name="vscrollbar_policy">automatic</property>
                            <child>
                              <object class="GtkTreeView" id="book_manager_treeview">
                                <property name="visible">True</property>
                                <property name="can_focus">True</property>
                                <property name="model">book_manager_store</property>
                                <property name="headers_clickable">False</property>
                                <property name="search_column">0</property>
                                <property name="enable_grid_lines">vertical</property>
                                <child>
                                  <object class="GtkTreeViewColumn" id="treeviewcolumn1">
                                    <property name="min_width">60</property>
                                    <property name="title" translatable="yes">Enabled</property>
                                    <property name="expand">True</property>
                                    <child>
                                      <object class="GtkCellRendererToggle" id="book_manager_toggle">
                                        <property name="width">60</property>
                                      </object>
                                      <attributes>
                                        <attribute name="active">0</attribute>
                                      </attributes>
                                    </child>
                                  </object>
                                </child>
                                <child>
                                  <object class="GtkTreeViewColumn" id="treeviewcolumn2">
                                    <property name="title" translatable="yes">Title</property>
                                    <property name="expand">True</property>
                                    <child>
                                      <object class="GtkCellRendererText" id="cellrenderertext1"/>
                                      <attributes>
                                        <attribute name="text">1</attribute>
                                      </attributes>
                                    </child>
                                  </object>
                                </child>
                              </object>
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
                  <object class="GtkLabel" id="label2">
                    <property name="visible">True</property>
                    <property name="label" translatable="yes">Book Shelf</property>
                    <attributes>
                      <attribute name="weight" value="bold"/>
                    </attributes>
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
