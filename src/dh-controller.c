/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@codefactory.se>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Mikael Hallendal <micke@codefactory.se>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <bonobo.h>
#include <bonobo/bonobo-shlib-factory.h>

#include "dh-bookshelf.h"
#include "function-database.h"
#include "dh-book-tree.h"
#include "dh-search.h"
#include "dh-history.h"
#include "dh-controller.h"

#define d(x)

#define DH_CONTROLLER_FACTORY_OAFIID "OAFIID:GNOME_DevHelp_Controller_Factory"

static void dh_controller_class_init  (DhControllerClass      *klass);
static void dh_controller_init        (DhController           *controller);
 
static void controller_destroy        (GtkObject              *object);

static void controller_uri_cb         (GObject                *unused,
				       const GnomeVFSURI      *uri,
				       DhController           *controller);

static gboolean controller_open       (DhController           *controller,
				       const gchar            *str_uri);

static void controller_emit_uri       (DhController           *controller,
				       const gchar            *str_uri);

static void cmd_back_cb               (BonoboUIComponent      *component,
				       gpointer                data,
				       const gchar            *cname);

static void cmd_forward_cb            (BonoboUIComponent      *component,
				       gpointer                data,
				       const gchar            *cname);

static void
controller_book_added_cb              (DhController           *controller,
				       Book                   *book,
				       gpointer                user_data);

static void
controller_book_removed_cb            (DhController           *controller,
				       Book                   *book,
				       gpointer                user_data);

static void
controller_forward_exists_changed_cb  (DhHistory              *history,
				       gboolean                exists,
				       DhController           *controller);
static void
controller_back_exists_changed_cb     (DhHistory              *history,
				       gboolean                exists,
				       DhController           *controller);

#define PARENT_TYPE BONOBO_X_OBJECT_TYPE
static BonoboXObjectClass *parent_class;

struct _DhControllerPriv {
        DhBookshelf       *bookshelf;
        FunctionDatabase  *fd;
        
        DhBookTree        *book_tree;
	DhSearch          *search;

	DhHistory         *history;
	
	BonoboEventSource *event_source;
        BonoboUIComponent *ui_component;

	BookNode          *current_node;
};

static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("CmdBack",           cmd_back_cb),
	BONOBO_UI_VERB ("CmdForward",        cmd_forward_cb),
        BONOBO_UI_VERB_END
};

static Bonobo_Control
impl_Dh_Controller_getSearchEntry (PortableServer_Servant   servant,
					CORBA_Environment       *ev)
{
	DhController       *controller;
	DhControllerPriv   *priv;
	GtkWidget               *w;
	BonoboControl           *control;

	controller = DH_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;
	
	w = dh_search_get_entry_widget (priv->search);
	gtk_widget_show_all (w);
	control = bonobo_control_new (w);

	return bonobo_object_corba_objref (BONOBO_OBJECT (control));
}

static Bonobo_Control
impl_Dh_Controller_getSearchResultList (PortableServer_Servant    servant,
					     CORBA_Environment        *ev)
{
	DhController       *controller;
	DhControllerPriv   *priv;
	GtkWidget               *w;
	BonoboControl           *control;

	controller = DH_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;
	
	w = dh_search_get_result_widget (priv->search);
	gtk_widget_show_all (w);
	control = bonobo_control_new (w);

	return bonobo_object_corba_objref (BONOBO_OBJECT (control));
}

static Bonobo_Control
impl_Dh_Controller_getBookTree (PortableServer_Servant    servant,
				CORBA_Environment        *ev)
{
	DhController       *controller;
	DhControllerPriv   *priv;
	GtkWidget               *w;
	BonoboControl           *control;

	controller = DH_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;
	
	w = dh_book_tree_get_scrolled (priv->book_tree);
	gtk_widget_show_all (w);
	control = bonobo_control_new (w);

	return bonobo_object_corba_objref (BONOBO_OBJECT (control));
}

static void
impl_Dh_Controller_addMenus (PortableServer_Servant    servant,
				  Bonobo_UIContainer        ui_container,
				  CORBA_Environment        *ev)
{
	DhController       *controller;
	DhControllerPriv   *priv;

	controller = DH_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;

	priv->ui_component = bonobo_ui_component_new ("DhController");
	
	bonobo_ui_component_set_container (priv->ui_component,
					   ui_container,
					   ev);
	bonobo_ui_component_add_verb_list_with_data (priv->ui_component,
						     verbs,
						     controller);
	bonobo_ui_util_set_ui (priv->ui_component, DATA_DIR,
			       "GNOME_DevHelp_Controller.ui",
			       "devhelp",
			       ev);
}

static void
impl_Dh_Controller_openURI (PortableServer_Servant    servant,
				 const CORBA_char         *str_uri,
				 CORBA_Environment        *ev)
{
	DhController       *controller;
	DhControllerPriv   *priv;
	Document                *document;
	gchar                   *anchor;
	gchar                   *str;
	
	controller = DH_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;

	str = g_strdup (str_uri);

	if (controller_open (controller, str)) {
		dh_history_goto (priv->history, str);
	}

	g_free (str);
}

static void
impl_Dh_Controller_search (PortableServer_Servant    servant,
				const CORBA_char         *str,
				CORBA_Environment        *ev)
{
	DhController       *controller;
	DhControllerPriv   *priv;

	controller = DH_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;
	
	dh_search_set_search_string (priv->search, str);
}

static void
dh_controller_class_init (DhControllerClass *klass)
{
	POA_GNOME_DevHelp_Controller__epv *epv = &klass->epv;

	parent_class = gtk_type_class (PARENT_TYPE);
	
	epv->getSearchEntry      = impl_Dh_Controller_getSearchEntry;
	epv->getSearchResultList = impl_Dh_Controller_getSearchResultList;
	epv->getBookTree         = impl_Dh_Controller_getBookTree;
	epv->addMenus            = impl_Dh_Controller_addMenus;
	epv->openURI             = impl_Dh_Controller_openURI;
	epv->search              = impl_Dh_Controller_search;
}

static void
dh_controller_init (DhController *controller)
{
        DhControllerPriv   *priv;
	
        priv = g_new0 (DhControllerPriv, 1);

	priv->ui_component = NULL;
        priv->fd           = function_database_new ();
	priv->history      = dh_history_new ();
        priv->bookshelf    = dh_bookshelf_new (priv->fd);
        priv->book_tree    = DH_BOOK_TREE (dh_book_tree_new (priv->bookshelf));

	g_signal_connect (priv->history,
			  "forward_exists_changed",
			  G_CALLBACK (controller_forward_exists_changed_cb),
			  controller);
	
	g_signal_connect (priv->history,
			  "back_exists_changed",
			  G_CALLBACK (controller_back_exists_changed_cb),
			  controller);

	/* TODO: Convert from connect_object to connect */
	g_signal_connect_object (G_OBJECT (priv->bookshelf),
				 "book_added",
				 G_CALLBACK (controller_book_added_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->bookshelf),
				 "book_removed",
				 G_CALLBACK (controller_book_removed_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->history),
				 "forward_exists_changed",
				 G_CALLBACK (controller_forward_exists_changed_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->history),
				 "back_exists_changed",
				 G_CALLBACK (controller_back_exists_changed_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);

	g_signal_connect_object (G_OBJECT (priv->bookshelf),
				 "book_added",
				 G_CALLBACK (controller_book_added_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->bookshelf),
				 "book_removed",
				 G_CALLBACK (controller_book_removed_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->book_tree),
				 "uri_selected",
				 G_CALLBACK (controller_uri_cb),
				 controller,
				 0);
        
        priv->search = dh_search_new (priv->bookshelf);

	g_signal_connect (G_OBJECT (priv->search),
			  "uri_selected",
			  G_CALLBACK (controller_uri_cb),
			  controller);

	priv->event_source = bonobo_event_source_new ();

	bonobo_object_add_interface (BONOBO_OBJECT (controller),
				     BONOBO_OBJECT (priv->event_source));


        controller->priv = priv;
}

static void
controller_destroy (GtkObject *object)
{
}

static gboolean
controller_open (DhController *controller, const gchar *url)
{
	DhControllerPriv   *priv;
	BookNode                *node;
	GnomeVFSURI             *uri;
	Document                *doc;
	gchar                   *anchor;
	gchar                   *str_uri;
	
	g_return_val_if_fail (controller != NULL, FALSE);
	g_return_val_if_fail (DH_IS_CONTROLLER (controller), FALSE);
	g_return_val_if_fail (doc != NULL, FALSE);

	priv = controller->priv;
	
	doc = dh_bookshelf_find_document (priv->bookshelf, url, &anchor);
	
	if (doc) { 
		node = dh_bookshelf_find_node (priv->bookshelf, doc, anchor);

		if (node) {
			dh_bookshelf_open_document (priv->bookshelf, doc);
			priv->current_node = node;
			
			gtk_signal_handler_block_by_func 
				(GTK_OBJECT (priv->book_tree),
				 GTK_SIGNAL_FUNC (controller_uri_cb),
				 controller); 
			
			dh_book_tree_open_node (priv->book_tree, node);
			
			gtk_signal_handler_unblock_by_func 
				(GTK_OBJECT (priv->book_tree), 
				 GTK_SIGNAL_FUNC (controller_uri_cb), 
				 controller);
		
			uri = book_node_get_uri (node, anchor);

			str_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
			
			controller_emit_uri (controller, str_uri);
		
			g_free (str_uri);

			return TRUE;
		}
	}
	
	return FALSE;
}

static void
controller_emit_uri (DhController   *controller, 
			     const gchar         *str_uri)
{
	DhControllerPriv   *priv;
	CORBA_any               *any;

	g_return_if_fail (controller != NULL);
	g_return_if_fail (DH_IS_CONTROLLER (controller));
	g_return_if_fail (str_uri != NULL);

	priv = controller->priv;
	
	any         = CORBA_any__alloc ();
	any->_type  = TC_CORBA_string;
	any->_value = CORBA_string_dup (str_uri);
	CORBA_any_set_release (any, CORBA_TRUE);

	bonobo_event_source_notify_listeners (priv->event_source,
					      "GNOME/DevHelp:URI:changed",
					      any,
					      NULL);
	
 	CORBA_free (any);
}

static void
cmd_back_cb (BonoboUIComponent *component, gpointer data, const gchar *cname)
{
	
	DhController       *controller;
	DhControllerPriv   *priv;
	gchar                   *str_uri = NULL;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (DH_IS_CONTROLLER (data));
	
	controller = DH_CONTROLLER (data);
	priv       = controller->priv;
	
	if (dh_history_exist_back (priv->history)) {
		bonobo_ui_component_set_prop (component, "/commands/CmdBack",
					      "sensitive", "0", NULL);

		str_uri = dh_history_go_back (priv->history);

		if (str_uri) {
			controller_open (controller, str_uri);
			g_free (str_uri);
		}
		
		if (dh_history_exist_back (priv->history)) {
			bonobo_ui_component_set_prop (component, "/commands/CmdBack",
						      "sensitive", "1", NULL);
		}
	}
}


static void
cmd_forward_cb (BonoboUIComponent   *component, 
		gpointer             data, 
		const gchar         *cname)
{
	DhController       *controller;
	DhControllerPriv   *priv;
	gchar                   *str_uri = NULL;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (DH_IS_CONTROLLER (data));
	
	controller = DH_CONTROLLER (data);
	priv       = controller->priv;

	if (dh_history_exist_forward (priv->history)) {
		bonobo_ui_component_set_prop (component,
					      "/commands/CmdForward",
					      "sensitive", "0", NULL);
		
		str_uri = dh_history_go_forward (priv->history);
		
		if (str_uri) {
			controller_open (controller, str_uri);
			g_free (str_uri);
		}
		
		if (dh_history_exist_forward (priv->history)) {
			bonobo_ui_component_set_prop (component,
						      "/commands/CmdForward",
						      "sensitive", "1", NULL);
		}
	}

}

static void
controller_book_added_cb (DhController   *controller,
				  Book                *book,
				  gpointer             user_data)
{
	DhControllerPriv   *priv;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (DH_IS_CONTROLLER (controller));
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));

	priv = controller->priv;

	dh_book_tree_add_book (priv->book_tree, book);
}

static void
controller_book_removed_cb (DhController   *controller,
				    Book                *book,
				    gpointer             user_data)
{
	DhControllerPriv   *priv;
	FunctionDatabase        *fd;
	GSList                  *functions;
	Function                *function;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (DH_IS_CONTROLLER (controller));
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = controller->priv;
	fd = priv->fd;
	
	functions = book_get_functions (book);
	while (functions) {
		function = (Function*)functions->data;
		function_database_remove_function (fd, function);
		
		functions = functions->next;
	}
	
	dh_book_tree_remove_book (priv->book_tree, book);
}

static void 
controller_forward_exists_changed_cb (DhHistory           *history,
					      gboolean             exists,
					      DhController   *controller)
{
	DhControllerPriv   *priv;
	gchar                   *sensitive;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (DH_IS_CONTROLLER (controller));

	priv = controller->priv;

	sensitive = exists ? "1" : "0";
	
	d(g_print ("Setting forward sensitivity to: %s\n", sensitive));

	if (priv->ui_component) {
		bonobo_ui_component_freeze (priv->ui_component, NULL);
		
		bonobo_ui_component_set_prop (priv->ui_component,
					      "/commands/CmdForward",
					      "sensitive", sensitive,
					      NULL);

		bonobo_ui_component_thaw (priv->ui_component, NULL);
	} else {
		g_warning ("No ui_component");
	}
}

static void 
controller_back_exists_changed_cb (DhHistory           *history,
					   gboolean             exists,
					   DhController   *controller)
{
	DhControllerPriv   *priv;
	gchar                   *sensitive;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (DH_IS_CONTROLLER (controller));

	priv = controller->priv;

	sensitive = exists ? "1" : "0";

	d(g_print ("Setting back sensitivity to: %s\n", sensitive));
	
	if (priv->ui_component) {
		bonobo_ui_component_freeze (priv->ui_component, NULL);
		
		bonobo_ui_component_set_prop (priv->ui_component,
					      "/commands/CmdBack",
					      "sensitive", sensitive,
					      NULL);

		bonobo_ui_component_thaw (priv->ui_component, NULL);
	} else {
		g_warning ("No ui_component");
	}
	
}

static void
controller_uri_cb (GObject             *unused,
			   const GnomeVFSURI   *uri,
			   DhController   *controller)
{
	DhControllerPriv   *priv;
	gchar                   *str_uri;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (DH_IS_CONTROLLER (controller));
	g_return_if_fail (uri != NULL);
	
	priv    = controller->priv;

	str_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

  	dh_history_goto (priv->history, str_uri);
	controller_emit_uri (controller, str_uri);

	g_free (str_uri);
}

DhController *
dh_controller_new ()
{
        DhController   *controller;
        
	controller = g_object_new (DH_TYPE_CONTROLLER, NULL);
        
        return controller;
}


BONOBO_X_TYPE_FUNC_FULL (DhController,
			 GNOME_DevHelp_Controller,
			 PARENT_TYPE,
			 dh_controller);


static BonoboObject *
controller_factory (BonoboGenericFactory   *factory,
			    const gchar            *object_id,
			    void                   *data)
{
	static GSList *controllers = NULL;
        DhController  *controller  = NULL;

	controller = dh_controller_new ();
	
	controllers = g_slist_prepend (controllers, controller);
	
	return BONOBO_OBJECT (controller);
}

BONOBO_OAF_SHLIB_FACTORY_MULTI (DH_CONTROLLER_FACTORY_OAFIID,
                                "Devhelp controller factory",
                                controller_factory,
                                NULL);
