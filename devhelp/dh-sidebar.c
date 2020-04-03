/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2001-2003 CodeFactory AB
 * SPDX-FileCopyrightText: 2001-2003 Mikael Hallendal <micke@imendio.com>
 * SPDX-FileCopyrightText: 2005-2008 Imendio AB
 * SPDX-FileCopyrightText: 2010 Lanedo GmbH
 * SPDX-FileCopyrightText: 2013 Aleksander Morgado <aleksander@gnu.org>
 * SPDX-FileCopyrightText: 2015, 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dh-sidebar.h"
#include "dh-book.h"
#include "dh-book-tree.h"
#include "dh-keyword-model.h"

/**
 * SECTION:dh-sidebar
 * @Title: DhSidebar
 * @Short_description: The sidebar
 *
 * In the Devhelp application, there is one #DhSidebar per main window,
 * displayed in the left side panel.
 *
 * A #DhSidebar contains:
 * - a #GtkSearchEntry at the top;
 * - a #DhBookTree (a subclass of #GtkTreeView);
 * - another #GtkTreeView (displaying a list, not a tree) with a #DhKeywordModel
 *   as its model.
 *
 * When the #GtkSearchEntry is empty, the #DhBookTree is shown. When the
 * #GtkSearchEntry is not empty, it shows the search results in the other
 * #GtkTreeView. The two #GtkTreeView's cannot be both visible at the same time,
 * it's either one or the other.
 *
 * #DhSidebar emits the #DhSidebar::link-selected signal. When that happens, the
 * Devhelp application opens the #DhLink in a #WebKitWebView shown at the right
 * side of the main window.
 */

typedef struct {
        DhProfile *profile;

        /* A GtkSearchEntry. */
        GtkEntry *entry;

        DhBookTree *book_tree;
        GtkScrolledWindow *sw_book_tree;

        DhKeywordModel *hitlist_model;
        GtkTreeView *hitlist_view;
        GtkScrolledWindow *sw_hitlist;

        guint idle_complete_id;
        guint idle_search_id;
} DhSidebarPrivate;

enum {
        SIGNAL_LINK_SELECTED,
        N_SIGNALS
};

enum {
        PROP_0,
        PROP_PROFILE,
        N_PROPERTIES
};

static guint signals[N_SIGNALS] = { 0 };
static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (DhSidebar, dh_sidebar, GTK_TYPE_GRID)

static void
set_profile (DhSidebar *sidebar,
             DhProfile *profile)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        g_return_if_fail (profile == NULL || DH_IS_PROFILE (profile));

        g_assert (priv->profile == NULL);
        g_set_object (&priv->profile, profile);
}

static void
dh_sidebar_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
        DhSidebar *sidebar = DH_SIDEBAR (object);

        switch (prop_id) {
                case PROP_PROFILE:
                        g_value_set_object (value, dh_sidebar_get_profile (sidebar));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_sidebar_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
        DhSidebar *sidebar = DH_SIDEBAR (object);

        switch (prop_id) {
                case PROP_PROFILE:
                        set_profile (sidebar, g_value_get_object (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

/******************************************************************************/

static gboolean
search_idle_cb (gpointer user_data)
{
        DhSidebar *sidebar = DH_SIDEBAR (user_data);
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        const gchar *search_text;
        const gchar *book_id;
        DhLink *selected_link;
        DhLink *exact_link;

        priv->idle_search_id = 0;

        search_text = gtk_entry_get_text (priv->entry);

        selected_link = dh_book_tree_get_selected_link (priv->book_tree);
        book_id = selected_link != NULL ? dh_link_get_book_id (selected_link) : NULL;

        /* Disconnect the model, see the doc of dh_keyword_model_filter(). */
        gtk_tree_view_set_model (priv->hitlist_view, NULL);

        exact_link = dh_keyword_model_filter (priv->hitlist_model,
                                              search_text,
                                              book_id,
                                              priv->profile);

        gtk_tree_view_set_model (priv->hitlist_view,
                                 GTK_TREE_MODEL (priv->hitlist_model));

        if (exact_link != NULL)
                g_signal_emit (sidebar, signals[SIGNAL_LINK_SELECTED], 0, exact_link);

        if (selected_link != NULL)
                dh_link_unref (selected_link);

        return G_SOURCE_REMOVE;
}

static void
setup_search_idle (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        if (priv->idle_search_id == 0)
                priv->idle_search_id = g_idle_add (search_idle_cb, sidebar);
}

/******************************************************************************/

/* Create DhCompletion objects, because if all the DhCompletion objects need to
 * be created (synchronously) at the time of the first completion, it can make
 * the GUI not responsive (measured time was for example 40ms to create the
 * DhCompletion's for 17 books, which is not a lot of books). On application
 * startup it is less a problem.
 */
static void
create_completion_objects (DhBookList *book_list)
{
        GList *books;
        GList *l;

        books = dh_book_list_get_books (book_list);

        for (l = books; l != NULL; l = l->next) {
                DhBook *cur_book = DH_BOOK (l->data);
                dh_book_get_completion (cur_book);
        }
}

static void
add_book_cb (DhBookList *book_list,
             DhBook     *book,
             DhSidebar  *sidebar)
{
        /* See comment of create_completion_objects(). */
        dh_book_get_completion (book);

        /* Update current search if any. */
        setup_search_idle (sidebar);
}

static void
remove_book_cb (DhBookList *book_list,
                DhBook     *book,
                DhSidebar  *sidebar)
{
        /* Update current search if any. */
        setup_search_idle (sidebar);
}

/******************************************************************************/

/* Returns: (transfer full) (nullable): */
static DhLink *
hitlist_get_selected_link (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        GtkTreeSelection *selection;
        GtkTreeModel *model;
        GtkTreeIter iter;
        DhLink *link;

        selection = gtk_tree_view_get_selection (priv->hitlist_view);
        if (!gtk_tree_selection_get_selected (selection, &model, &iter))
                return NULL;

        gtk_tree_model_get (model, &iter,
                            DH_KEYWORD_MODEL_COL_LINK, &link,
                            -1);

        return link;
}

static void
hitlist_selection_changed_cb (GtkTreeSelection *selection,
                              DhSidebar        *sidebar)
{
        DhLink *link;

        link = hitlist_get_selected_link (sidebar);

        if (link != NULL) {
                g_signal_emit (sidebar, signals[SIGNAL_LINK_SELECTED], 0, link);
                dh_link_unref (link);
        }
}

static gboolean
entry_key_press_event_cb (GtkEntry    *entry,
                          GdkEventKey *event,
                          DhSidebar   *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        if (event->keyval == GDK_KEY_Tab) {
                if (event->state & GDK_CONTROL_MASK) {
                        if (gtk_widget_is_visible (GTK_WIDGET (priv->hitlist_view)))
                                gtk_widget_grab_focus (GTK_WIDGET (priv->hitlist_view));
                } else {
                        gtk_editable_select_region (GTK_EDITABLE (entry), 0, 0);
                        gtk_editable_set_position (GTK_EDITABLE (entry), -1);
                }

                return GDK_EVENT_STOP;
        }

        return GDK_EVENT_PROPAGATE;
}

static void
entry_changed_cb (GtkEntry  *entry,
                  DhSidebar *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        const gchar *search_text;

        search_text = gtk_entry_get_text (entry);

        /* We don't want a delay when the search text becomes empty, to show the
         * book tree. So do it here and not in entry_search_changed_cb().
         */
        if (search_text == NULL || search_text[0] == '\0') {
                gtk_widget_hide (GTK_WIDGET (priv->sw_hitlist));
                gtk_widget_show (GTK_WIDGET (priv->sw_book_tree));
        }
}

static void
entry_search_changed_cb (GtkSearchEntry *search_entry,
                         DhSidebar      *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        const gchar *search_text;

        search_text = gtk_entry_get_text (GTK_ENTRY (search_entry));

        if (search_text != NULL && search_text[0] != '\0') {
                gtk_widget_hide (GTK_WIDGET (priv->sw_book_tree));
                gtk_widget_show (GTK_WIDGET (priv->sw_hitlist));
                setup_search_idle (sidebar);
        }
}

static gboolean
complete_idle_cb (gpointer user_data)
{
        DhSidebar *sidebar = DH_SIDEBAR (user_data);
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        GList *books;
        GList *l;
        GList *completion_objects = NULL;
        const gchar *search_text;
        gchar *completed;

        books = dh_book_list_get_books (dh_profile_get_book_list (priv->profile));
        for (l = books; l != NULL; l = l->next) {
                DhBook *cur_book = DH_BOOK (l->data);
                DhCompletion *completion;

                completion = dh_book_get_completion (cur_book);
                completion_objects = g_list_prepend (completion_objects, completion);
        }

        search_text = gtk_entry_get_text (priv->entry);
        completed = dh_completion_aggregate_complete (completion_objects, search_text);

        if (completed != NULL) {
                guint16 n_chars_before;

                n_chars_before = gtk_entry_get_text_length (priv->entry);

                gtk_entry_set_text (priv->entry, completed);
                gtk_editable_set_position (GTK_EDITABLE (priv->entry), n_chars_before);
                gtk_editable_select_region (GTK_EDITABLE (priv->entry),
                                            n_chars_before, -1);
        }

        g_list_free (completion_objects);
        g_free (completed);

        priv->idle_complete_id = 0;
        return G_SOURCE_REMOVE;
}

static void
entry_insert_text_cb (GtkEntry    *entry,
                      const gchar *text,
                      gint         length,
                      gint        *position,
                      DhSidebar   *sidebar)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);

        if (priv->idle_complete_id == 0)
                priv->idle_complete_id = g_idle_add (complete_idle_cb, sidebar);
}

static void
entry_stop_search_cb (GtkSearchEntry *entry,
                      gpointer        user_data)
{
        gtk_entry_set_text (GTK_ENTRY (entry), "");
}

static void
hitlist_cell_data_func (GtkTreeViewColumn *tree_column,
                        GtkCellRenderer   *cell,
                        GtkTreeModel      *hitlist_model,
                        GtkTreeIter       *iter,
                        gpointer           data)
{
        DhLink *link;
        DhLinkType link_type;
        PangoStyle style;
        PangoWeight weight;
        gboolean current_book_flag;
        gchar *name;

        gtk_tree_model_get (hitlist_model, iter,
                            DH_KEYWORD_MODEL_COL_LINK, &link,
                            DH_KEYWORD_MODEL_COL_CURRENT_BOOK_FLAG, &current_book_flag,
                            -1);

        if (dh_link_get_flags (link) & DH_LINK_FLAGS_DEPRECATED)
                style = PANGO_STYLE_ITALIC;
        else
                style = PANGO_STYLE_NORMAL;

        /* Matches on the current book are given in bold. Note that we check the
         * current book as it was given to the DhKeywordModel. Do *not* rely on
         * the current book as given by the DhSidebar, as that will change
         * whenever a hit is clicked.
         */
        if (current_book_flag)
                weight = PANGO_WEIGHT_BOLD;
        else
                weight = PANGO_WEIGHT_NORMAL;

        link_type = dh_link_get_link_type (link);

        if (link_type == DH_LINK_TYPE_STRUCT ||
            link_type == DH_LINK_TYPE_PROPERTY ||
            link_type == DH_LINK_TYPE_SIGNAL) {
                name = g_markup_printf_escaped ("%s <i><small><span weight=\"normal\">(%s)</span></small></i>",
                                                dh_link_get_name (link),
                                                dh_link_type_to_string (link_type));
        } else {
                name = g_markup_printf_escaped ("%s", dh_link_get_name (link));
        }

        g_object_set (cell,
                      "markup", name,
                      "style", style,
                      "weight", weight,
                      NULL);

        dh_link_unref (link);
        g_free (name);
}

static void
book_tree_link_selected_cb (DhBookTree *book_tree,
                            DhLink     *link,
                            DhSidebar  *sidebar)
{
        g_signal_emit (sidebar, signals[SIGNAL_LINK_SELECTED], 0, link);
}

static void
dh_sidebar_constructed (GObject *object)
{
        DhSidebar *sidebar = DH_SIDEBAR (object);
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (sidebar);
        GtkTreeSelection *selection;
        GtkCellRenderer *cell;
        DhBookList *book_list;

        if (G_OBJECT_CLASS (dh_sidebar_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (dh_sidebar_parent_class)->constructed (object);

        if (priv->profile == NULL)
                priv->profile = g_object_ref (dh_profile_get_default ());

        /* Setup the search entry */
        priv->entry = GTK_ENTRY (gtk_search_entry_new ());
        gtk_widget_set_hexpand (GTK_WIDGET (priv->entry), TRUE);
        g_object_set (priv->entry,
                      "margin", 6,
                      NULL);
        gtk_container_add (GTK_CONTAINER (sidebar), GTK_WIDGET (priv->entry));

        g_signal_connect (priv->entry,
                          "key-press-event",
                          G_CALLBACK (entry_key_press_event_cb),
                          sidebar);

        g_signal_connect (priv->entry,
                          "changed",
                          G_CALLBACK (entry_changed_cb),
                          sidebar);

        g_signal_connect (priv->entry,
                          "search-changed",
                          G_CALLBACK (entry_search_changed_cb),
                          sidebar);

        g_signal_connect (priv->entry,
                          "insert-text",
                          G_CALLBACK (entry_insert_text_cb),
                          sidebar);

        g_signal_connect (priv->entry,
                          "stop-search",
                          G_CALLBACK (entry_stop_search_cb),
                          NULL);

        /* Setup hitlist */
        priv->hitlist_model = dh_keyword_model_new ();
        priv->hitlist_view = GTK_TREE_VIEW (gtk_tree_view_new ());
        gtk_tree_view_set_model (priv->hitlist_view, GTK_TREE_MODEL (priv->hitlist_model));
        gtk_tree_view_set_headers_visible (priv->hitlist_view, FALSE);
        gtk_tree_view_set_enable_search (priv->hitlist_view, FALSE);
        gtk_widget_show (GTK_WIDGET (priv->hitlist_view));

        selection = gtk_tree_view_get_selection (priv->hitlist_view);

        /* Set BROWSE mode. When clicking again on the same (already selected)
         * row, it re-emits the ::changed signal, which is convenient to come
         * back to that symbol when the HTML view has been scrolled away.
         */
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

        g_signal_connect (selection,
                          "changed",
                          G_CALLBACK (hitlist_selection_changed_cb),
                          sidebar);

        cell = gtk_cell_renderer_text_new ();
        g_object_set (cell,
                      "ellipsize", PANGO_ELLIPSIZE_END,
                      NULL);
        gtk_tree_view_insert_column_with_data_func (priv->hitlist_view,
                                                    -1,
                                                    NULL,
                                                    cell,
                                                    hitlist_cell_data_func,
                                                    sidebar,
                                                    NULL);

        /* Hitlist packing */
        priv->sw_hitlist = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
        gtk_widget_set_no_show_all (GTK_WIDGET (priv->sw_hitlist), TRUE);
        gtk_scrolled_window_set_policy (priv->sw_hitlist,
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_container_add (GTK_CONTAINER (priv->sw_hitlist),
                           GTK_WIDGET (priv->hitlist_view));
        gtk_widget_set_hexpand (GTK_WIDGET (priv->sw_hitlist), TRUE);
        gtk_widget_set_vexpand (GTK_WIDGET (priv->sw_hitlist), TRUE);
        gtk_container_add (GTK_CONTAINER (sidebar), GTK_WIDGET (priv->sw_hitlist));

        /* DhBookList */
        book_list = dh_profile_get_book_list (priv->profile);
        create_completion_objects (book_list);

        g_signal_connect_object (book_list,
                                 "add-book",
                                 G_CALLBACK (add_book_cb),
                                 sidebar,
                                 G_CONNECT_AFTER);

        g_signal_connect_object (book_list,
                                 "remove-book",
                                 G_CALLBACK (remove_book_cb),
                                 sidebar,
                                 G_CONNECT_AFTER);

        /* Setup the book tree */
        priv->sw_book_tree = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
        gtk_widget_show (GTK_WIDGET (priv->sw_book_tree));
        gtk_widget_set_no_show_all (GTK_WIDGET (priv->sw_book_tree), TRUE);
        gtk_scrolled_window_set_policy (priv->sw_book_tree,
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);

        priv->book_tree = dh_book_tree_new (priv->profile);
        gtk_widget_show (GTK_WIDGET (priv->book_tree));
        g_signal_connect (priv->book_tree,
                          "link-selected",
                          G_CALLBACK (book_tree_link_selected_cb),
                          sidebar);
        gtk_container_add (GTK_CONTAINER (priv->sw_book_tree), GTK_WIDGET (priv->book_tree));
        gtk_widget_set_hexpand (GTK_WIDGET (priv->sw_book_tree), TRUE);
        gtk_widget_set_vexpand (GTK_WIDGET (priv->sw_book_tree), TRUE);
        gtk_container_add (GTK_CONTAINER (sidebar), GTK_WIDGET (priv->sw_book_tree));

        gtk_widget_show_all (GTK_WIDGET (sidebar));
}

static void
dh_sidebar_dispose (GObject *object)
{
        DhSidebarPrivate *priv = dh_sidebar_get_instance_private (DH_SIDEBAR (object));

        g_clear_object (&priv->profile);
        g_clear_object (&priv->hitlist_model);

        if (priv->idle_complete_id != 0) {
                g_source_remove (priv->idle_complete_id);
                priv->idle_complete_id = 0;
        }

        if (priv->idle_search_id != 0) {
                g_source_remove (priv->idle_search_id);
                priv->idle_search_id = 0;
        }

        G_OBJECT_CLASS (dh_sidebar_parent_class)->dispose (object);
}

static void
dh_sidebar_class_init (DhSidebarClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = dh_sidebar_get_property;
        object_class->set_property = dh_sidebar_set_property;
        object_class->constructed = dh_sidebar_constructed;
        object_class->dispose = dh_sidebar_dispose;

        /**
         * DhSidebar::link-selected:
         * @sidebar: a #DhSidebar.
         * @link: the selected #DhLink.
         *
         * The ::link-selected signal is emitted when:
         * 1. One row in one of the #GtkTreeView's is selected and contains a
         *    #DhLink (i.e. when the row is not a language group);
         * 2. Or if there is an exact match returned by
         *    dh_keyword_model_filter() when a search occurs.
         *
         * Note that dh_sidebar_get_selected_link() takes into account only the
         * former, not the latter. So the last @link emitted with this signal is
         * not necessarily the same as the current return value of
         * dh_sidebar_get_selected_link().
         */
        signals[SIGNAL_LINK_SELECTED] =
                g_signal_new ("link-selected",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DhSidebarClass, link_selected),
                              NULL, NULL, NULL,
                              G_TYPE_NONE,
                              1, DH_TYPE_LINK);

        /**
         * DhSidebar:profile:
         *
         * The #DhProfile. If set to %NULL, the default profile as returned by
         * dh_profile_get_default() is used.
         *
         * Since: 3.30
         */
        properties[PROP_PROFILE] =
                g_param_spec_object ("profile",
                                     "Profile",
                                     "",
                                     DH_TYPE_PROFILE,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
dh_sidebar_init (DhSidebar *sidebar)
{
        gtk_orientable_set_orientation (GTK_ORIENTABLE (sidebar),
                                        GTK_ORIENTATION_VERTICAL);

        gtk_widget_set_hexpand (GTK_WIDGET (sidebar), TRUE);
        gtk_widget_set_vexpand (GTK_WIDGET (sidebar), TRUE);
}

/**
 * dh_sidebar_new:
 * @book_manager: (nullable): a #DhBookManager. This parameter is deprecated,
 * you should just pass %NULL.
 *
 * Returns: (transfer floating): a new #DhSidebar widget.
 * Deprecated: 3.30: Use dh_sidebar_new2() instead.
 */
GtkWidget *
dh_sidebar_new (DhBookManager *book_manager)
{
        return g_object_new (DH_TYPE_SIDEBAR, NULL);
}

/**
 * dh_sidebar_new2:
 * @profile: (nullable): a #DhProfile, or %NULL for the default profile.
 *
 * Returns: (transfer floating): a new #DhSidebar widget.
 * Since: 3.30
 */
DhSidebar *
dh_sidebar_new2 (DhProfile *profile)
{
        g_return_val_if_fail (profile == NULL || DH_IS_PROFILE (profile), NULL);

        return g_object_new (DH_TYPE_SIDEBAR,
                             "profile", profile,
                             NULL);
}

/**
 * dh_sidebar_get_profile:
 * @sidebar: a #DhSidebar.
 *
 * Returns: (transfer none): the #DhProfile of @sidebar.
 * Since: 3.30
 */
DhProfile *
dh_sidebar_get_profile (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv;

        g_return_val_if_fail (DH_IS_SIDEBAR (sidebar), NULL);

        priv = dh_sidebar_get_instance_private (sidebar);
        return priv->profile;
}

/**
 * dh_sidebar_get_selected_link:
 * @sidebar: a #DhSidebar.
 *
 * Note: the return value of this function is not necessarily the same as the
 * last #DhLink emitted by the #DhSidebar::link-selected signal. See the
 * documentation of #DhSidebar::link-selected.
 *
 * Returns: (transfer full) (nullable): the currently selected #DhLink in the
 * visible #GtkTreeView of @sidebar, or %NULL if the selection is empty or if a
 * language group row is selected. Unref with dh_link_unref() when no longer
 * needed.
 * Since: 3.30
 */
DhLink *
dh_sidebar_get_selected_link (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv;
        gboolean book_tree_visible;
        gboolean hitlist_visible;

        g_return_val_if_fail (DH_IS_SIDEBAR (sidebar), NULL);

        priv = dh_sidebar_get_instance_private (sidebar);

        book_tree_visible = gtk_widget_get_visible (GTK_WIDGET (priv->sw_book_tree));
        hitlist_visible = gtk_widget_get_visible (GTK_WIDGET (priv->sw_hitlist));

        g_return_val_if_fail ((book_tree_visible || hitlist_visible) &&
                              !(book_tree_visible && hitlist_visible), NULL);

        if (book_tree_visible)
                return dh_book_tree_get_selected_link (priv->book_tree);

        return hitlist_get_selected_link (sidebar);
}

/**
 * dh_sidebar_select_uri:
 * @sidebar: a #DhSidebar.
 * @uri: the URI to select.
 *
 * Calls dh_book_tree_select_uri().
 */
void
dh_sidebar_select_uri (DhSidebar   *sidebar,
                       const gchar *uri)
{
        DhSidebarPrivate *priv;

        g_return_if_fail (DH_IS_SIDEBAR (sidebar));
        g_return_if_fail (uri != NULL);

        priv = dh_sidebar_get_instance_private (sidebar);

        dh_book_tree_select_uri (priv->book_tree, uri);
}

/**
 * dh_sidebar_set_search_string:
 * @sidebar: a #DhSidebar.
 * @str: the string to search.
 */
void
dh_sidebar_set_search_string (DhSidebar   *sidebar,
                              const gchar *str)
{
        DhSidebarPrivate *priv;

        g_return_if_fail (DH_IS_SIDEBAR (sidebar));
        g_return_if_fail (str != NULL);

        priv = dh_sidebar_get_instance_private (sidebar);

        gtk_entry_set_text (priv->entry, str);
        gtk_editable_select_region (GTK_EDITABLE (priv->entry), 0, 0);
        gtk_editable_set_position (GTK_EDITABLE (priv->entry), -1);

        /* If the GtkEntry text was already equal to @str, the
         * GtkEditable::changed signal was not emitted, so force to emit it to
         * call entry_changed_cb() and entry_search_changed_cb(), forcing a new
         * search. If an exact match is found, the DhSidebar::link-selected
         * signal will be emitted, to re-jump to that symbol (even if the
         * GtkEntry text was equal, it doesn't mean that the WebKitWebView was
         * showing the exact match).
         * https://bugzilla.gnome.org/show_bug.cgi?id=776596
         */
        g_signal_emit_by_name (priv->entry, "changed");
}

/**
 * dh_sidebar_set_search_focus:
 * @sidebar: a #DhSidebar.
 *
 * Gives the focus to the search entry.
 */
void
dh_sidebar_set_search_focus (DhSidebar *sidebar)
{
        DhSidebarPrivate *priv;

        g_return_if_fail (DH_IS_SIDEBAR (sidebar));

        priv = dh_sidebar_get_instance_private (sidebar);

        gtk_widget_grab_focus (GTK_WIDGET (priv->entry));
}
