#include <gtk/gtk.h>
#include "eggtoolbar.h"
#include "eggtoolitem.h"
#include "eggtoolbutton.h"
#include "eggtoggletoolbutton.h"
#include "eggseparatortoolitem.h"

static void
reload_clicked (GtkWidget *widget)
{
  static GdkAtom atom_rcfiles = GDK_NONE;

  GdkEventClient sev;
  int i;
  
  if (!atom_rcfiles)
    atom_rcfiles = gdk_atom_intern("_GTK_READ_RCFILES", FALSE);

  for(i = 0; i < 5; i++)
    sev.data.l[i] = 0;
  sev.data_format = 32;
  sev.message_type = atom_rcfiles;
  gdk_event_send_clientmessage_toall ((GdkEvent *) &sev);
}

static void
change_orientation (GtkWidget *button, GtkWidget *toolbar)
{
  GtkWidget *table;
  GtkOrientation orientation;

  table = gtk_widget_get_parent (toolbar);
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    orientation = GTK_ORIENTATION_VERTICAL;
  else
    orientation = GTK_ORIENTATION_HORIZONTAL;

  g_object_ref (toolbar);
  gtk_container_remove (GTK_CONTAINER (table), toolbar);
  egg_toolbar_set_orientation (EGG_TOOLBAR (toolbar), orientation);
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gtk_table_attach (GTK_TABLE (table), toolbar,
			0,2, 0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
    }
  else
    {
      gtk_table_attach (GTK_TABLE (table), toolbar,
			0,1, 0,3, GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
    }
  g_object_unref (toolbar);
}

static void
change_show_arrow (GtkWidget *button, GtkWidget *toolbar)
{
  egg_toolbar_set_show_arrow (EGG_TOOLBAR (toolbar),
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}

static void
change_toolbar_style (GtkWidget *option_menu, GtkWidget *toolbar)
{
  GtkToolbarStyle style;

  style = gtk_option_menu_get_history (GTK_OPTION_MENU (option_menu));
  egg_toolbar_set_style (EGG_TOOLBAR (toolbar), style);
}

static void
set_visible_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
		 GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  EggToolItem *tool_item;
  gboolean visible;

  gtk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_get (G_OBJECT (tool_item), "visible", &visible, NULL);
  g_object_set (G_OBJECT (cell), "active", visible, NULL);
  g_object_unref (tool_item);
}

static void
visibile_toggled(GtkCellRendererToggle *cell, const gchar *path_str,
		 GtkTreeModel *model)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EggToolItem *tool_item;
  gboolean visible;

  path = gtk_tree_path_new_from_string (path_str);
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter, 0, &tool_item, -1);
  g_object_get (G_OBJECT (tool_item), "visible", &visible, NULL);
  g_object_set (G_OBJECT (tool_item), "visible", !visible, NULL);
  g_object_unref (tool_item);

  gtk_tree_model_row_changed (model, path, &iter);
  gtk_tree_path_free (path);
}

static void
set_expandable_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
		    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  EggToolItem *tool_item;

  gtk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_set (G_OBJECT (cell), "active", tool_item->expandable, NULL);
  g_object_unref (tool_item);
}

static void
expandable_toggled(GtkCellRendererToggle *cell, const gchar *path_str,
		   GtkTreeModel *model)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EggToolItem *tool_item;

  path = gtk_tree_path_new_from_string (path_str);
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter, 0, &tool_item, -1);
  egg_tool_item_set_expandable (tool_item, !tool_item->expandable);
  g_object_unref (tool_item);

  gtk_tree_model_row_changed (model, path, &iter);
  gtk_tree_path_free (path);
}

static void
set_pack_end_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
		  GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  EggToolItem *tool_item;

  gtk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_set (G_OBJECT (cell), "active", tool_item->pack_end, NULL);
  g_object_unref (tool_item);
}

static void
pack_end_toggled(GtkCellRendererToggle *cell, const gchar *path_str,
		 GtkTreeModel *model)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EggToolItem *tool_item;

  path = gtk_tree_path_new_from_string (path_str);
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter, 0, &tool_item, -1);
  egg_tool_item_set_pack_end (tool_item, !tool_item->pack_end);
  g_object_unref (tool_item);

  gtk_tree_model_row_changed (model, path, &iter);
  gtk_tree_path_free (path);
}

static void
set_homogeneous_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
		     GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  EggToolItem *tool_item;

  gtk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_set (G_OBJECT (cell), "active", tool_item->homogeneous, NULL);
  g_object_unref (tool_item);
}

static void
homogeneous_toggled(GtkCellRendererToggle *cell, const gchar *path_str,
		    GtkTreeModel *model)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EggToolItem *tool_item;

  path = gtk_tree_path_new_from_string (path_str);
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter, 0, &tool_item, -1);
  egg_tool_item_set_homogeneous (tool_item, !tool_item->homogeneous);
  g_object_unref (tool_item);

  gtk_tree_model_row_changed (model, path, &iter);
  gtk_tree_path_free (path);
}

static GtkListStore *
create_items_list (GtkWidget **tree_view_p)
{
  GtkWidget *tree_view;
  GtkListStore *list_store;
  GtkCellRenderer *cell;
  
  list_store = gtk_list_store_new (2, EGG_TYPE_TOOL_ITEM, G_TYPE_STRING);
  
  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view),
					       -1, "Tool Item",
					       gtk_cell_renderer_text_new (),
					       "text", 1, NULL);

  cell = gtk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (visibile_toggled),
		    list_store);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree_view),
					      -1, "Visible",
					      cell,
					      set_visible_func, NULL, NULL);

  cell = gtk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (expandable_toggled),
		    list_store);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree_view),
					      -1, "Expandable",
					      cell,
					      set_expandable_func, NULL, NULL);

  cell = gtk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (pack_end_toggled),
		    list_store);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree_view),
					      -1, "Pack End",
					      cell,
					      set_pack_end_func, NULL, NULL);

  cell = gtk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (homogeneous_toggled),
		    list_store);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree_view),
					      -1, "Homogeneous",
					      cell,
					      set_homogeneous_func, NULL,NULL);

  g_object_unref (list_store);

  *tree_view_p = tree_view;

  return list_store;
}

static void
add_item_to_list (GtkListStore *store, EggToolItem *item, const gchar *text)
{
  GtkTreeIter iter;

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      0, item,
		      1, text,
		      -1);
  
}

static void
bold_toggled (EggToggleToolButton *button)
{
  g_message ("Bold toggled (active=%d)",
	     egg_toggle_tool_button_get_active (button));
}

gint
main (gint argc, gchar **argv)
{
  GtkWidget *window, *toolbar, *table, *treeview, *scrolled_window;
  GtkWidget *hbox, *checkbox, *option_menu, *menu;
  gint i;
  static const gchar *toolbar_styles[] = { "icons", "text", "both (vertical)",
					   "both (horizontal)" };
  EggToolItem *item;
  GtkListStore *store;
  GtkWidget *image;
  
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect (window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_add (GTK_CONTAINER (window), table);

  toolbar = egg_toolbar_new ();
  egg_toolbar_set_style (EGG_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH_HORIZ);
  gtk_table_attach (GTK_TABLE (table), toolbar,
		    0,2, 0,1, GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_table_attach (GTK_TABLE (table), hbox,
		    1,2, 1,2, GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);

  checkbox = gtk_check_button_new_with_mnemonic("_Vertical");
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect (checkbox, "toggled",
		    G_CALLBACK (change_orientation), toolbar);

  checkbox = gtk_check_button_new_with_mnemonic("_Show Arrow");
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect (checkbox, "toggled",
		    G_CALLBACK (change_show_arrow), toolbar);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();
  for (i = 0; i < G_N_ELEMENTS (toolbar_styles); i++)
    {
      GtkWidget *menuitem;

      menuitem = gtk_menu_item_new_with_label (toolbar_styles[i]);
      gtk_container_add (GTK_CONTAINER (menu), menuitem);
      gtk_widget_show (menuitem);
    }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
			       EGG_TOOLBAR (toolbar)->style);
  gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
  g_signal_connect (option_menu, "changed",
		    G_CALLBACK (change_toolbar_style), toolbar);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_table_attach (GTK_TABLE (table), scrolled_window,
		    1,2, 2,3, GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

  store = create_items_list (&treeview);
  gtk_container_add (GTK_CONTAINER (scrolled_window), treeview);
  
  item = egg_tool_button_new_from_stock (GTK_STOCK_NEW);
  add_item_to_list (store, item, "New");
  egg_toolbar_insert_item (EGG_TOOLBAR (toolbar), item, -1);

  item = egg_tool_button_new_from_stock (GTK_STOCK_OPEN);
  add_item_to_list (store, item, "Open");
  egg_toolbar_insert_item (EGG_TOOLBAR (toolbar), item, -1);

  item = egg_separator_tool_item_new ();
  add_item_to_list (store, item, "-----");    
  egg_toolbar_insert_item (EGG_TOOLBAR (toolbar), item, -1);
  
  item = egg_tool_button_new_from_stock (GTK_STOCK_REFRESH);
  add_item_to_list (store, item, "Refresh");  
  g_signal_connect (item, "clicked", G_CALLBACK (reload_clicked), NULL);
  egg_toolbar_insert_item (EGG_TOOLBAR (toolbar), item, -1);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
  item = egg_tool_item_new ();
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (item), image);
  add_item_to_list (store, item, "(Custom Item)");    
  egg_toolbar_insert_item (EGG_TOOLBAR (toolbar), item, -1);

  
  item = egg_tool_button_new_from_stock (GTK_STOCK_GO_BACK);
  add_item_to_list (store, item, "Back");    
  egg_toolbar_insert_item (EGG_TOOLBAR (toolbar), item, -1);

  item = egg_separator_tool_item_new ();
  add_item_to_list (store, item, "-----");  
  egg_toolbar_insert_item (EGG_TOOLBAR (toolbar), item, -1);
  
  item = egg_tool_button_new_from_stock (GTK_STOCK_GO_FORWARD);
  add_item_to_list (store, item, "Forward");  
  egg_toolbar_insert_item (EGG_TOOLBAR (toolbar), item, -1);

  item = egg_toggle_tool_button_new_from_stock (GTK_STOCK_BOLD);
  g_signal_connect (item, "toggled", G_CALLBACK (bold_toggled), NULL);
  add_item_to_list (store, item, "Bold");  
  egg_toolbar_insert_item (EGG_TOOLBAR (toolbar), item, -1);

  gtk_widget_show_all (window);
  
  gtk_main ();
  
  return 0;
}
