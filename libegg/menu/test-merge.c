#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libegg/menu/egg-menu-merge.h>
#include <libegg/menu/egg-toggle-action.h>

#ifndef _
#  define _(String) (String)
#  define N_(String) (String)
#endif

static gchar *node_type_names[] = {
  "UNDECIDED",
  "ROOT",
  "MENUBAR",
  "MENU",
  "TOOLBAR",
  "MENU_PLACEHOLDER",
  "TOOLBAR_PLACEHOLDER",
  "POPUPS",
  "MENUITEM",
  "TOOLITEM",
  "SEPARATOR"
};

struct { const gchar *filename; guint merge_id; } merge_ids[] = {
  { "merge-1.ui", 0 },
  { "merge-2.ui", 0 },
  { "merge-3.ui", 0 }
};

static void
print_node(EggMenuMerge *merge, GNode *node, gint indent_level)
{
  gint i;
  EggMenuMergeNode *mnode;
  GNode *child;
  gchar *action_label = NULL;

  mnode = node->data;

  if (mnode->action)
    g_object_get (mnode->action, "label", &action_label, NULL);

  for (i = 0; i < indent_level; i++)
    printf("  ");
  printf("%s (%s): action_name=%s action_label=%s\n", mnode->name,
	 node_type_names[mnode->type],
	 g_quark_to_string(mnode->action_name), action_label);

  g_free(action_label);

  for (child = node->children; child != NULL; child = child->next) {
    print_node(merge, child, indent_level + 1);
  }
}

static void
activate_action (EggAction *action)
{
  const gchar *name = action->name;
  const gchar *typename = G_OBJECT_TYPE_NAME (action);

  g_message ("Action %s (type=%s) activated", name, typename);
}

static void
toggle_action (EggAction *action)
{
  const gchar *name = action->name;
  const gchar *typename = G_OBJECT_TYPE_NAME (action);

  g_message ("Action %s (type=%s) activated (active=%d)", name, typename,
	     (gint) EGG_TOGGLE_ACTION (action)->active);
}


static EggActionGroupEntry entries[] = {
  { "StockFileMenuAction", N_("_File"), NULL, NULL, NULL, NULL, NULL },
  { "StockEditMenuAction", N_("_Edit"), NULL, NULL, NULL, NULL, NULL },
  { "StockHelpMenuAction", N_("_Help"), NULL, NULL, NULL, NULL, NULL },
  { "Test", N_("Test"), NULL, NULL, NULL, NULL, NULL },

  { "NewAction", NULL, GTK_STOCK_NEW, "<control>n", NULL,
    G_CALLBACK (activate_action), NULL },
  { "New2Action", NULL, GTK_STOCK_NEW, "<control>m", NULL,
    G_CALLBACK (activate_action), NULL },
  { "OpenAction", NULL, GTK_STOCK_OPEN, "<control>o", NULL,
    G_CALLBACK (activate_action), NULL },
  { "QuitAction", NULL, GTK_STOCK_QUIT, "<control>q", NULL,
    G_CALLBACK (gtk_main_quit), NULL },
  { "CutAction", NULL, GTK_STOCK_CUT, "<control>x", NULL,
    G_CALLBACK (activate_action), NULL },
  { "CopyAction", NULL, GTK_STOCK_COPY, "<control>c", NULL,
    G_CALLBACK (activate_action), NULL },
  { "PasteAction", NULL, GTK_STOCK_PASTE, "<control>v", NULL,
    G_CALLBACK (activate_action), NULL },
  { "justify-left", NULL, GTK_STOCK_JUSTIFY_LEFT, "<control>L",
    N_("Left justify the text"),
    G_CALLBACK (toggle_action), NULL, RADIO_ACTION },
  { "justify-center", NULL, GTK_STOCK_JUSTIFY_CENTER, "<control>E",
    N_("Center justify the text"),
    G_CALLBACK (toggle_action), NULL, RADIO_ACTION, "justify-left" },
  { "justify-right", NULL, GTK_STOCK_JUSTIFY_RIGHT, "<control>R",
    N_("Right justify the text"),
    G_CALLBACK (toggle_action), NULL, RADIO_ACTION, "justify-left" },
  { "justify-fill", NULL, GTK_STOCK_JUSTIFY_FILL, "<control>J",
    N_("Fill justify the text"),
    G_CALLBACK (toggle_action), NULL, RADIO_ACTION, "justify-left" },
  { "AboutAction", N_("_About"), NULL, NULL, NULL,
    G_CALLBACK (activate_action), NULL },
};
static guint n_entries = G_N_ELEMENTS (entries);

static void
add_widget (EggMenuMerge *merge, GtkWidget *widget, GtkBox *box)
{
  gtk_box_pack_start (box, widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
}

static void
dump_tree (GtkWidget *button, EggMenuMerge *merge)
{
  egg_menu_merge_ensure_update (merge);
  
  print_node (merge, merge->root_node, 0);
}

static void
toggle_merge (GtkWidget *button, EggMenuMerge *merge)
{
  gint mergenum;

  mergenum = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "mergenum"));

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
    {
      GError *err = NULL;

      g_message("merging %s", merge_ids[mergenum].filename);
      merge_ids[mergenum].merge_id =
	egg_menu_merge_add_ui_from_file(merge, merge_ids[mergenum].filename,
					&err);
      if (err != NULL)
	{
	  GtkWidget *dialog;

	  dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(button)),
					  0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					  "could not merge %s: %s", merge_ids[mergenum].filename,
					  err->message);

	  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_object_destroy), NULL);
	  gtk_widget_show(dialog);

	  g_clear_error(&err);
	}
    }
  else
    {
      g_message("unmerging %s (merge_id=%u)", merge_ids[mergenum].filename,
		merge_ids[mergenum].merge_id);
      egg_menu_merge_remove_ui(merge, merge_ids[mergenum].merge_id);
    }
}

static void  
set_name_func (GtkTreeViewColumn *tree_column,
	       GtkCellRenderer   *cell,
	       GtkTreeModel      *tree_model,
	       GtkTreeIter       *iter,
	       gpointer           data)
{
  EggAction *action;
  char *name;
  
  gtk_tree_model_get (tree_model, iter,
		      0, &action,
		      -1);
  g_object_get (G_OBJECT (action),
		"name", &name,
		NULL);
  g_object_set (G_OBJECT (cell),
		"text", name,
		NULL);
  g_free (name);
  g_object_unref (action);
}

static void
set_sensitive_func(GtkTreeViewColumn *tree_column,
		   GtkCellRenderer   *cell,
		   GtkTreeModel      *tree_model,
		   GtkTreeIter       *iter,
		   gpointer           data)
{
  EggAction *action;
  gboolean sensitive;
  
  gtk_tree_model_get (tree_model, iter,
		      0, &action,
		      -1);
  g_object_get (G_OBJECT (action),
		"sensitive", &sensitive,
		NULL);
  g_object_set (G_OBJECT (cell),
		"active", sensitive,
		NULL);
  g_object_unref (action);
}


static void
set_visible_func(GtkTreeViewColumn *tree_column,
		 GtkCellRenderer   *cell,
		 GtkTreeModel      *tree_model,
		 GtkTreeIter       *iter,
		 gpointer           data)
{
  EggAction *action;
  gboolean visible;
  
  gtk_tree_model_get (tree_model, iter,
		      0, &action,
		      -1);
  g_object_get (G_OBJECT (action),
		"visible", &visible,
		NULL);
  g_object_set (G_OBJECT (cell),
		"active", visible,
		NULL);
  g_object_unref (action);
}

static void
sensitivity_toggled (GtkCellRendererToggle *cell, const gchar *path_str, GtkTreeModel *model)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EggAction *action;
  gboolean sensitive;

  path = gtk_tree_path_new_from_string (path_str);
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter,
		      0, &action,
		      -1);
  g_object_get (G_OBJECT (action),
		"sensitive", &sensitive,
		NULL);
  g_object_set (G_OBJECT (action),
		"sensitive", !sensitive,
		NULL);
  gtk_tree_model_row_changed (model, path, &iter);
  gtk_tree_path_free (path);
}

static void
visibility_toggled (GtkCellRendererToggle *cell, const gchar *path_str, GtkTreeModel *model)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EggAction *action;
  gboolean visible;

  path = gtk_tree_path_new_from_string (path_str);
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter,
		      0, &action,
		      -1);
  g_object_get (G_OBJECT (action),
		"visible", &visible,
		NULL);
  g_object_set (G_OBJECT (action),
		"visible", !visible,
		NULL);
  gtk_tree_model_row_changed (model, path, &iter);
  gtk_tree_path_free (path);
}

static gint
iter_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
		   gpointer user_data)
{
  GValue a_value = { 0, }, b_value = { 0, };
  EggAction *a_action, *b_action;
  gint retval = 0;

  gtk_tree_model_get_value (model, a, 0, &a_value);
  gtk_tree_model_get_value (model, b, 0, &b_value);
  a_action = EGG_ACTION( g_value_get_object (&a_value));
  b_action = EGG_ACTION( g_value_get_object (&b_value));

  if (a_action->name == NULL && b_action->name == NULL) retval = 0;
  else if (a_action->name == NULL) retval = -1;
  else if (b_action->name == NULL) retval = 1;
  else retval = strcmp(a_action->name, b_action->name);

  g_value_unset (&b_value);
  g_value_unset (&a_value);

  return retval;
}

static GtkWidget *
create_tree_view (EggMenuMerge *merge)
{
  GtkWidget *tree_view, *sw;
  GtkListStore *store;
  GList *p;
  GtkCellRenderer *cell;
  
  store = gtk_list_store_new (1, EGG_TYPE_ACTION);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(store), 0,
				   iter_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 0,
				       GTK_SORT_ASCENDING);

  for (p = merge->action_groups; p; p = p->next)
    {
      GList *actions, *l;

      actions = egg_action_group_list_actions (p->data);

      for (l = actions; l; l = l->next)
	{
	  GtkTreeIter iter;

	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, l->data,
			      -1);
	}
    }
  
  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree_view),
					      -1, "Action",
					      gtk_cell_renderer_text_new (),
					      set_name_func, NULL, NULL);

  gtk_tree_view_column_set_sort_column_id (gtk_tree_view_get_column (GTK_TREE_VIEW (tree_view), 0), 0);

  cell = gtk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (sensitivity_toggled), store);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree_view),
					      -1, "Sensitive",
					      cell,
					      set_sensitive_func, NULL, NULL);

  cell = gtk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (visibility_toggled), store);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree_view),
					      -1, "Visible",
					      cell,
					      set_visible_func, NULL, NULL);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (sw), tree_view);
  
  return sw;
}

int
main(int argc, char **argv)
{
  EggActionGroup *action_group;
  EggMenuMerge *merge;
  GtkWidget *window, *table, *frame, *menu_box, *vbox, *view;
  GtkWidget *button;
  gint i;
  
  gtk_init(&argc, &argv);

  action_group = egg_action_group_new ("TestActions");
  egg_action_group_add_actions (action_group, entries, n_entries);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), -1, 400);
  g_signal_connect (window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  table = gtk_table_new(2, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 2);
  gtk_container_set_border_width(GTK_CONTAINER(table), 2);
  gtk_container_add (GTK_CONTAINER (window), table);

  frame = gtk_frame_new ("Menus and Toolbars");
  gtk_table_attach(GTK_TABLE(table), frame, 0,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  
  menu_box = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(menu_box), 2);
  gtk_container_add(GTK_CONTAINER(frame), menu_box);
  
  merge = egg_menu_merge_new();
  egg_menu_merge_insert_action_group (merge, action_group, 0);
  g_signal_connect (merge, "add_widget", G_CALLBACK (add_widget), menu_box);

  gtk_window_add_accel_group (GTK_WINDOW (window), merge->accel_group);
  
  frame = gtk_frame_new("UI Files");
  gtk_table_attach(GTK_TABLE(table), frame, 0,1, 0,1,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);

  vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  for (i = 0; i < G_N_ELEMENTS(merge_ids); i++)
    {
      button = gtk_check_button_new_with_label (merge_ids[i].filename);
      g_object_set_data(G_OBJECT(button), "mergenum", GINT_TO_POINTER(i));
      g_signal_connect (button, "toggled",
			G_CALLBACK (toggle_merge), merge);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    }

  button = gtk_button_new_with_mnemonic ("_Dump Tree");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (dump_tree), merge);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  view = create_tree_view (merge);
  gtk_table_attach(GTK_TABLE(table), view, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  
  gtk_widget_show_all (window);
  gtk_main ();


  return 0;
}
