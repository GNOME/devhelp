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
#include "book-index.h"
#include "dh-search.h"
#include "dh-history.h"
#include "devhelp-controller.h"

#define d(x)

#define DEVHELP_CONTROLLER_FACTORY_OAFIID "OAFIID:GNOME_DevHelp_Controller_Factory"

static void devhelp_controller_class_init  (DevHelpControllerClass *klass);
static void devhelp_controller_init        (DevHelpController      *index);
 
static void devhelp_controller_destroy     (GtkObject              *object);

static void devhelp_controller_uri_cb      (GObject                *unused,
					    const GnomeVFSURI      *uri,
					    DevHelpController      *controller);

static gboolean devhelp_controller_open    (DevHelpController      *controller,
					    const gchar            *str_uri);

static void devhelp_controller_emit_uri    (DevHelpController      *controller,
					    const gchar            *str_uri);

static void cmd_back_cb                    (BonoboUIComponent      *component,
					    gpointer                data,
					    const gchar            *cname);

static void cmd_forward_cb                 (BonoboUIComponent      *component,
					    gpointer                data,
					    const gchar            *cname);

static void
devhelp_controller_book_added_cb           (DevHelpController      *controller,
					    Book                   *book,
					    gpointer                user_data);

static void
devhelp_controller_book_removed_cb         (DevHelpController      *controller,
					    Book                   *book,
					    gpointer                user_data);

static void
devhelp_controller_forward_exists_changed_cb (DhHistory              *history,
					      gboolean              exists,
					      DevHelpController    *controller);
static void
devhelp_controller_back_exists_changed_cb (DhHistory                 *history,
					   gboolean                 exists,
					   DevHelpController       *controller);

#define PARENT_TYPE BONOBO_X_OBJECT_TYPE
static BonoboXObjectClass *parent_class;

struct _DevHelpControllerPriv {
        DhBookshelf         *bookshelf;
        FunctionDatabase    *fd;
        
        BookIndex           *index;
	DhSearch            *search;

	DhHistory             *history;
	
	BonoboEventSource   *event_source;
        BonoboUIComponent   *ui_component;

	BookNode            *current_node;
};

static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("CmdBack",           cmd_back_cb),
	BONOBO_UI_VERB ("CmdForward",        cmd_forward_cb),
        BONOBO_UI_VERB_END
};

static Bonobo_Control
impl_DevHelp_Controller_getSearchEntry (PortableServer_Servant   servant,
					CORBA_Environment       *ev)
{
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;
	GtkWidget               *w;
	BonoboControl           *control;

	controller = DEVHELP_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;
	
	w = dh_search_get_entry_widget (priv->search);
	gtk_widget_show_all (w);
	control = bonobo_control_new (w);

	return bonobo_object_corba_objref (BONOBO_OBJECT (control));
}

static Bonobo_Control
impl_DevHelp_Controller_getSearchResultList (PortableServer_Servant    servant,
					     CORBA_Environment        *ev)
{
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;
	GtkWidget               *w;
	BonoboControl           *control;

	controller = DEVHELP_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;
	
	w = dh_search_get_result_widget (priv->search);
	gtk_widget_show_all (w);
	control = bonobo_control_new (w);

	return bonobo_object_corba_objref (BONOBO_OBJECT (control));
}

static Bonobo_Control
impl_DevHelp_Controller_getBookIndex (PortableServer_Servant    servant,
				      CORBA_Environment        *ev)
{
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;
	GtkWidget               *w;
	BonoboControl           *control;

	controller = DEVHELP_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;
	
	w = book_index_get_scrolled (priv->index);
	gtk_widget_show_all (w);
	control = bonobo_control_new (w);

	return bonobo_object_corba_objref (BONOBO_OBJECT (control));
}

static void
impl_DevHelp_Controller_addMenus (PortableServer_Servant    servant,
				  Bonobo_UIContainer        ui_container,
				  CORBA_Environment        *ev)
{
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;

	controller = DEVHELP_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;

	priv->ui_component = bonobo_ui_component_new ("DevHelpController");
	
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
impl_DevHelp_Controller_openURI (PortableServer_Servant    servant,
				 const CORBA_char         *str_uri,
				 CORBA_Environment        *ev)
{
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;
	Document                *document;
	gchar                   *anchor;
	gchar                   *str;
	
	controller = DEVHELP_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;

	str = g_strdup (str_uri);

	if (devhelp_controller_open (controller, str)) {
		dh_history_goto (priv->history, str);
	}

	g_free (str);
}

static void
impl_DevHelp_Controller_search (PortableServer_Servant    servant,
				const CORBA_char         *str,
				CORBA_Environment        *ev)
{
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;

	controller = DEVHELP_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;
	
	dh_search_set_search_string (priv->search, str);
}

static void
devhelp_controller_class_init (DevHelpControllerClass *klass)
{
	POA_GNOME_DevHelp_Controller__epv *epv = &klass->epv;

	parent_class = gtk_type_class (PARENT_TYPE);
	
	epv->getSearchEntry      = impl_DevHelp_Controller_getSearchEntry;
	epv->getSearchResultList = impl_DevHelp_Controller_getSearchResultList;
	epv->getBookIndex        = impl_DevHelp_Controller_getBookIndex;
	epv->addMenus            = impl_DevHelp_Controller_addMenus;
	epv->openURI             = impl_DevHelp_Controller_openURI;
	epv->search              = impl_DevHelp_Controller_search;
}

static void
devhelp_controller_init (DevHelpController *controller)
{
        DevHelpControllerPriv   *priv;
	
        priv = g_new0 (DevHelpControllerPriv, 1);

	priv->ui_component = NULL;
        priv->fd           = function_database_new ();
	priv->history      = dh_history_new ();
        priv->bookshelf    = dh_bookshelf_new (priv->fd);
        priv->index        = BOOK_INDEX (book_index_new (priv->bookshelf));

	g_signal_connect (priv->history,
			  "forward_exists_changed",
			  G_CALLBACK (devhelp_controller_forward_exists_changed_cb),
			  controller);
	
	g_signal_connect (priv->history,
			  "back_exists_changed",
			  G_CALLBACK (devhelp_controller_back_exists_changed_cb),
			  controller);

	/* TODO: Convert from connect_object to connect */
	g_signal_connect_object (G_OBJECT (priv->bookshelf),
				 "book_added",
				 G_CALLBACK (devhelp_controller_book_added_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->bookshelf),
				 "book_removed",
				 G_CALLBACK (devhelp_controller_book_removed_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->history),
				 "forward_exists_changed",
				 G_CALLBACK (devhelp_controller_forward_exists_changed_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->history),
				 "back_exists_changed",
				 G_CALLBACK (devhelp_controller_back_exists_changed_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);

	g_signal_connect_object (G_OBJECT (priv->bookshelf),
				 "book_added",
				 G_CALLBACK (devhelp_controller_book_added_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->bookshelf),
				 "book_removed",
				 G_CALLBACK (devhelp_controller_book_removed_cb),
				 G_OBJECT (controller),
				 G_CONNECT_AFTER);
	
	g_signal_connect_object (G_OBJECT (priv->index),
				 "uri_selected",
				 G_CALLBACK (devhelp_controller_uri_cb),
				 controller,
				 0);
        
        priv->search = dh_search_new (priv->bookshelf);

	g_signal_connect (G_OBJECT (priv->search),
			  "uri_selected",
			  G_CALLBACK (devhelp_controller_uri_cb),
			  controller);

	priv->event_source = bonobo_event_source_new ();

	bonobo_object_add_interface (BONOBO_OBJECT (controller),
				     BONOBO_OBJECT (priv->event_source));


        controller->priv = priv;
}

static void
devhelp_controller_destroy (GtkObject *object)
{
}

static gboolean
devhelp_controller_open (DevHelpController *controller, const gchar *url)
{
	DevHelpControllerPriv   *priv;
	BookNode                *node;
	GnomeVFSURI             *uri;
	Document                *doc;
	gchar                   *anchor;
	gchar                   *str_uri;
	
	g_return_val_if_fail (controller != NULL, FALSE);
	g_return_val_if_fail (IS_DEVHELP_CONTROLLER (controller), FALSE);
	g_return_val_if_fail (doc != NULL, FALSE);

	priv = controller->priv;
	
	doc = dh_bookshelf_find_document (priv->bookshelf, url, &anchor);
	
	if (doc) { 
		node = dh_bookshelf_find_node (priv->bookshelf, doc, anchor);

		if (node) {
			dh_bookshelf_open_document (priv->bookshelf, doc);
			priv->current_node = node;
			
			gtk_signal_handler_block_by_func 
				(GTK_OBJECT (priv->index),
				 GTK_SIGNAL_FUNC (devhelp_controller_uri_cb),
				 controller); 
			
			book_index_open_node (priv->index, node);
			
			gtk_signal_handler_unblock_by_func 
				(GTK_OBJECT (priv->index), 
				 GTK_SIGNAL_FUNC (devhelp_controller_uri_cb), 
				 controller);
		
			uri = book_node_get_uri (node, anchor);

			str_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
			
			devhelp_controller_emit_uri (controller, str_uri);
		
			g_free (str_uri);

			return TRUE;
		}
	}
	
	return FALSE;
}

static void
devhelp_controller_emit_uri (DevHelpController   *controller, 
			     const gchar         *str_uri)
{
	DevHelpControllerPriv   *priv;
	CORBA_any               *any;

	g_return_if_fail (controller != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (controller));
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
	
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;
	gchar                   *str_uri = NULL;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (data));
	
	controller = DEVHELP_CONTROLLER (data);
	priv       = controller->priv;
	
	if (dh_history_exist_back (priv->history)) {
		bonobo_ui_component_set_prop (component, "/commands/CmdBack",
					      "sensitive", "0", NULL);

		str_uri = dh_history_go_back (priv->history);

		if (str_uri) {
			devhelp_controller_open (controller, str_uri);
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
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;
	gchar                   *str_uri = NULL;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (data));
	
	controller = DEVHELP_CONTROLLER (data);
	priv       = controller->priv;

	if (dh_history_exist_forward (priv->history)) {
		bonobo_ui_component_set_prop (component,
					      "/commands/CmdForward",
					      "sensitive", "0", NULL);
		
		str_uri = dh_history_go_forward (priv->history);
		
		if (str_uri) {
			devhelp_controller_open (controller, str_uri);
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
devhelp_controller_book_added_cb (DevHelpController   *controller,
				  Book                *book,
				  gpointer             user_data)
{
	DevHelpControllerPriv   *priv;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (controller));
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));

	priv = controller->priv;

	book_index_add_book (priv->index, book);
}

static void
devhelp_controller_book_removed_cb (DevHelpController   *controller,
				    Book                *book,
				    gpointer             user_data)
{
	DevHelpControllerPriv   *priv;
	FunctionDatabase        *fd;
	GSList                  *functions;
	Function                *function;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (controller));
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
	
	book_index_remove_book (priv->index, book);
}

static void 
devhelp_controller_forward_exists_changed_cb (DhHistory           *history,
					      gboolean             exists,
					      DevHelpController   *controller)
{
	DevHelpControllerPriv   *priv;
	gchar                   *sensitive;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (controller));

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
devhelp_controller_back_exists_changed_cb (DhHistory           *history,
					   gboolean             exists,
					   DevHelpController   *controller)
{
	DevHelpControllerPriv   *priv;
	gchar                   *sensitive;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (controller));

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

DevHelpController *
devhelp_controller_new ()
{
        DevHelpController   *controller;
        
	controller = g_object_new (TYPE_DEVHELP_CONTROLLER, NULL);
        
        return controller;
}

static void
devhelp_controller_uri_cb (GObject             *unused,
			   const GnomeVFSURI   *uri,
			   DevHelpController   *controller)
{
	DevHelpControllerPriv   *priv;
	gchar                   *str_uri;
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (controller));
	g_return_if_fail (uri != NULL);
	
	priv    = controller->priv;

	str_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

  	dh_history_goto (priv->history, str_uri);
	devhelp_controller_emit_uri (controller, str_uri);

	g_free (str_uri);
}

BONOBO_X_TYPE_FUNC_FULL (DevHelpController,
			GNOME_DevHelp_Controller,
			PARENT_TYPE,
			devhelp_controller);


static BonoboObject *
devhelp_controller_factory (BonoboGenericFactory   *factory,
			    const gchar            *object_id,
			    void                   *data)
{
	static GSList           *controllers = NULL;
        DevHelpController       *controller  = NULL;

	controller = devhelp_controller_new ();
	
	controllers = g_slist_prepend (controllers, controller);
	
	return BONOBO_OBJECT (controller);
}


BONOBO_OAF_SHLIB_FACTORY_MULTI (DEVHELP_CONTROLLER_FACTORY_OAFIID,
                                "DevHelp controller factory",
                                devhelp_controller_factory,
                                NULL);
