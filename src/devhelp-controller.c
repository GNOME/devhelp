/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Mikael Hallendal <micke@codefactory.se>
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
#include "bookshelf.h"
#include "function-database.h"
#include "book-index.h"
#include "books-dialog.h"
#include "devhelp-search.h"
#include "devhelp-controller.h"

#define d(x) x

#define DEVHELP_CONTROLLER_FACTORY_OAFIID "OAFIID:GNOME_DevHelp_Controller_Factory"

static void devhelp_controller_class_init  (DevHelpControllerClass *klass);
static void devhelp_controller_init        (DevHelpController      *index);
 
static void devhelp_controller_destroy     (GtkObject              *object);

static void devhelp_controller_uri_cb      (DevHelpController      *controller,
					    const GnomeVFSURI      *uri,
					    gpointer                user_data);

static void 
devhelp_controller_books_dialog_destroy_cb (GtkObject              *object,
					    DevHelpController      *controller);
static void cmd_book_manager_cb            (BonoboUIComponent      *component,
					    gpointer                data,
					    const gchar            *cname);

static void devhelp_controller_book_added_cb (DevHelpController    *controller,
					      Book                 *book,
					      gpointer              user_data);

static void devhelp_controller_book_removed_cb (DevHelpController  *controller,
						Book               *book,
						gpointer            user_data);


#define PARENT_TYPE BONOBO_X_OBJECT_TYPE
static BonoboXObjectClass *parent_class;

struct _DevHelpControllerPriv {
        Bookshelf           *bookshelf;
        FunctionDatabase    *fd;
        
        BookIndex           *index;
	DevHelpSearch       *search;
	
	BonoboEventSource   *event_source;
        BonoboUIComponent   *ui_component;

	BookNode            *current_node;
};

static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("CmdBookManager",    cmd_book_manager_cb),
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
	
	w = devhelp_search_get_entry_widget (priv->search);
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
	
	w = devhelp_search_get_result_widget (priv->search);
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
	BonoboUIComponent       *ui_component;

	controller = DEVHELP_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;

	ui_component = bonobo_ui_component_new ("DevHelpController");
	
	bonobo_ui_component_set_container (ui_component, ui_container);
	bonobo_ui_component_add_verb_list_with_data (ui_component,
						     verbs,
						     controller);
	bonobo_ui_util_set_ui (ui_component, DATA_DIR,
			       "GNOME_DevHelp_Controller.ui",
			       "devhelp");
}

static void
impl_DevHelp_Controller_openURI (PortableServer_Servant    servant,
				 const CORBA_char         *str_uri,
				 CORBA_Environment        *ev)
{
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;
	Document                *document;
	BookNode                *node;
	gchar                   *anchor;
	GnomeVFSURI             *uri;
	
	controller = DEVHELP_CONTROLLER (bonobo_x_object (servant));
	priv       = controller->priv;
	
	document = bookshelf_find_document (priv->bookshelf, str_uri, &anchor);

	if (document) { 
		node = bookshelf_find_node (priv->bookshelf,
					    document, anchor);

		if (node) {
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

			devhelp_controller_uri_cb (controller, uri, NULL);
		}
	}
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
	
	devhelp_search_set_search_string (priv->search, str);
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
        gchar                   *local_dir;
	
        priv = g_new0 (DevHelpControllerPriv, 1);

        priv->fd  = function_database_new ();
        local_dir = g_strdup_printf ("%s/.devhelp", getenv ("HOME"));

        function_database_freeze (priv->fd);
        priv->bookshelf = bookshelf_new (DATA_DIR "/devhelp/specs", priv->fd);
        bookshelf_add_directory (priv->bookshelf, local_dir);
        function_database_thaw (priv->fd);

        g_free (local_dir);

        priv->index  = BOOK_INDEX (book_index_new (priv->bookshelf));

	gtk_signal_connect_object (GTK_OBJECT (priv->bookshelf),
				   "book_added",
				   GTK_SIGNAL_FUNC (devhelp_controller_book_added_cb),
				   GTK_OBJECT (controller));
	
	gtk_signal_connect_object (GTK_OBJECT (priv->bookshelf),
				   "book_removed",
				   GTK_SIGNAL_FUNC (devhelp_controller_book_removed_cb),
				   GTK_OBJECT (controller));
	
	
	gtk_signal_connect_object (GTK_OBJECT (priv->index),
				   "uri_selected",
				   GTK_SIGNAL_FUNC (devhelp_controller_uri_cb),
				   GTK_OBJECT (controller));
        
        priv->search = devhelp_search_new (priv->bookshelf);

	gtk_signal_connect_object (GTK_OBJECT (priv->search),
				   "uri_selected",
				   GTK_SIGNAL_FUNC (devhelp_controller_uri_cb),
				   GTK_OBJECT (controller));

	priv->event_source = bonobo_event_source_new ();


	bonobo_object_add_interface (BONOBO_OBJECT (controller),
				     BONOBO_OBJECT (priv->event_source));

        controller->priv = priv;
}

static void
devhelp_controller_destroy (GtkObject *object)
{
}

static void
cmd_book_manager_cb (BonoboUIComponent   *component,
		     gpointer             data,
		     const gchar         *cname)
{
	DevHelpController       *controller;
	DevHelpControllerPriv   *priv;
	GtkWidget               *widget;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (data));
	
	controller = DEVHELP_CONTROLLER (data);
	priv       = controller->priv;

	widget = books_dialog_new (priv->bookshelf);
	gtk_widget_show_all (widget);
	
	gtk_main ();
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
	
	g_return_if_fail (controller != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (controller));
	g_return_if_fail (book != NULL);
	g_return_if_fail (IS_BOOK (book));
	
	priv = controller->priv;
	
	/* FIX: Not impl. yet */
}

DevHelpController *
devhelp_controller_new ()
{
        DevHelpController   *controller;
        
        controller = gtk_type_new (TYPE_DEVHELP_CONTROLLER);
        
        return controller;
}

static void
devhelp_controller_uri_cb (DevHelpController   *controller,
			   const GnomeVFSURI   *uri,
			   gpointer             ignored)
{
	DevHelpControllerPriv   *priv;
	CORBA_any               *any;
	BonoboArg               *arg;
	gchar                   *str_uri;
		
	g_return_if_fail (controller != NULL);
	g_return_if_fail (IS_DEVHELP_CONTROLLER (controller));
	g_return_if_fail (uri != NULL);
	
	priv    = controller->priv;
	str_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
	
	any         = CORBA_any__alloc ();
	any->_type  = TC_CORBA_string;
	any->_value = CORBA_string_dup (str_uri);
	CORBA_any_set_release (any, CORBA_TRUE);

	bonobo_event_source_notify_listeners (priv->event_source,
					      "GNOME/DevHelp:URI:changed",
					      any,
					      NULL);

	g_free (str_uri);
	CORBA_free (any);
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
