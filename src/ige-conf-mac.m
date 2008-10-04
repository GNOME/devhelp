/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Imendio AB
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
 */

#include "config.h"
#import <Cocoa/Cocoa.h>
#include <string.h>
#include "ige-conf.h"

typedef struct {
        NSUserDefaults *defaults;
} IgeConfPriv;

typedef struct {
        IgeConf           *conf;
        IgeConfNotifyFunc  func;
        gpointer           user_data;
} IgeConfNotifyData;

#define POOL_ALLOC   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]
#define POOL_RELEASE [pool release]

G_DEFINE_TYPE (IgeConf, ige_conf, G_TYPE_OBJECT);

#define GET_PRIVATE(instance) G_TYPE_INSTANCE_GET_PRIVATE \
  (instance, IGE_TYPE_CONF, IgeConfPriv);

static IgeConf *global_conf = NULL;

static void
conf_finalize (GObject *object)
{
        IgeConfPriv *priv = GET_PRIVATE (object);

        [priv->defaults synchronize];

        G_OBJECT_CLASS (ige_conf_parent_class)->finalize (object);
}

static void
ige_conf_class_init (IgeConfClass *class)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (class);

        object_class->finalize = conf_finalize;

        g_type_class_add_private (object_class, sizeof (IgeConfPriv));
}

static void
ige_conf_init (IgeConf *conf)
{
        IgeConfPriv  *priv = GET_PRIVATE (conf);
        NSDictionary *dict;

        priv->defaults = [NSUserDefaults standardUserDefaults];

        POOL_ALLOC;

#define CONF_PATH                     "/apps/devhelp"
#define CONF_UI_MAIN_WINDOW_MAXIMIZED CONF_PATH "/ui/main_window_maximized"

        dict = [NSDictionary dictionaryWithObjectsAndKeys:
                @"NO", @CONF_UI_MAIN_WINDOW_MAXIMIZED,
                nil];

        /* Setup defaults. FIXME: Can we do this only when needed? */
        [priv->defaults registerDefaults: dict];

        POOL_RELEASE;
}

IgeConf *
ige_conf_get (void)
{
        if (!global_conf) {
                global_conf = g_object_new (IGE_TYPE_CONF, NULL);
        }

        return global_conf;
}

gboolean
ige_conf_set_int (IgeConf     *conf,
                  const gchar *key,
                  gint         value)
{
        IgeConfPriv *priv;
        NSString    *string;

        g_return_val_if_fail (IGE_IS_CONF (conf), FALSE);

        priv = GET_PRIVATE (conf);

        POOL_ALLOC;

        string = [NSString stringWithUTF8String: key];
        [priv->defaults setInteger: value forKey: string];

        POOL_RELEASE;

        return TRUE;
}

gboolean
ige_conf_get_int (IgeConf     *conf,
                  const gchar *key,
                  gint        *value)
{
        IgeConfPriv *priv;
        NSString    *string;

        g_return_val_if_fail (IGE_IS_CONF (conf), FALSE);

        priv = GET_PRIVATE (conf);

        POOL_ALLOC;

        string = [NSString stringWithUTF8String: key];
        *value = [priv->defaults integerForKey: string];

        [priv->defaults synchronize];

        POOL_RELEASE;

        return TRUE;
}

gboolean
ige_conf_set_bool (IgeConf     *conf,
                   const gchar *key,
                   gboolean     value)
{
        IgeConfPriv *priv;
        NSString    *string;

        g_return_val_if_fail (IGE_IS_CONF (conf), FALSE);

        priv = GET_PRIVATE (conf);

        POOL_ALLOC;

        string = [NSString stringWithUTF8String: key];
        [priv->defaults setBool: value forKey: string];

        [priv->defaults synchronize];

        POOL_RELEASE;

        return TRUE;
}

gboolean
ige_conf_get_bool (IgeConf     *conf,
                   const gchar *key,
                   gboolean    *value)
{
        IgeConfPriv *priv;
        NSString    *string;

        g_return_val_if_fail (IGE_IS_CONF (conf), FALSE);

        priv = GET_PRIVATE (conf);

        POOL_ALLOC;

        string = [NSString stringWithUTF8String: key];
        *value = [priv->defaults boolForKey: string];

        POOL_RELEASE;

        return TRUE;
}

gboolean
ige_conf_set_string (IgeConf     *conf,
                     const gchar *key,
                     const gchar *value)
{
        IgeConfPriv *priv;
        NSString    *string, *nsvalue;

        g_return_val_if_fail (IGE_IS_CONF (conf), FALSE);

        priv = GET_PRIVATE (conf);

        POOL_ALLOC;

        string = [NSString stringWithUTF8String: key];
        nsvalue = [NSString stringWithUTF8String: value];
        [priv->defaults setObject: nsvalue forKey: string];

        [priv->defaults synchronize];

        POOL_RELEASE;

        return TRUE;
}

gboolean
ige_conf_get_string (IgeConf      *conf,
                     const gchar  *key,
                     gchar       **value)
{
        IgeConfPriv *priv;
        NSString    *string, *nsvalue;

        g_return_val_if_fail (IGE_IS_CONF (conf), FALSE);

        priv = GET_PRIVATE (conf);

        POOL_ALLOC;

        string = [NSString stringWithUTF8String: key];
        nsvalue = [priv->defaults stringForKey: string];

        *value = g_strdup ([nsvalue UTF8String]);

        POOL_RELEASE;

        return TRUE;
}

gboolean
ige_conf_set_string_list (IgeConf     *conf,
                          const gchar *key,
                          GSList      *value)
{
        IgeConfPriv *priv;

        g_return_val_if_fail (IGE_IS_CONF (conf), FALSE);

        priv = GET_PRIVATE (conf);

        return TRUE; /*gconf_client_set_string_list (priv->gconf_client,
                                             key,
                                             value,
                                             NULL);
                     */
}

gboolean
ige_conf_get_string_list (IgeConf      *conf,
                          const gchar  *key,
                          GSList      **value)
{
        IgeConfPriv *priv;

        g_return_val_if_fail (IGE_IS_CONF (conf), FALSE);

        priv = GET_PRIVATE (conf);

        *value = NULL; /*gconf_client_get_string_list (priv->gconf_client,
                                               key,
                                               &error);
                       */
        return TRUE;
}

/*
static void
conf_notify_data_free (IgeConfNotifyData *data)
{
        g_object_unref (data->conf);
        g_slice_free (IgeConfNotifyData, data);
}

static void
conf_notify_func (GConfClient *client,
                  guint        id,
                  GConfEntry  *entry,
                  gpointer     user_data)
{
        IgeConfNotifyData *data;

        data = user_data;

        data->func (data->conf,
                    gconf_entry_get_key (entry),
                    data->user_data);
}
*/

guint
ige_conf_notify_add (IgeConf           *conf,
                     const gchar       *key,
                     IgeConfNotifyFunc  func,
                     gpointer           user_data)
{
        IgeConfPriv       *priv;
        guint              id;
        IgeConfNotifyData *data;

        g_return_val_if_fail (IGE_IS_CONF (conf), 0);

        priv = GET_PRIVATE (conf);

        data = g_slice_new (IgeConfNotifyData);
        data->func = func;
        data->user_data = user_data;
        data->conf = g_object_ref (conf);

        id = 0; /*gconf_client_notify_add (priv->gconf_client,
                                      key,
                                      conf_notify_func,
                                      data,
                                      (GFreeFunc) conf_notify_data_free,
                                      NULL);
        */
        return id;
}

gboolean
ige_conf_notify_remove (IgeConf *conf,
                        guint    id)
{
        IgeConfPriv *priv;

        g_return_val_if_fail (IGE_IS_CONF (conf), FALSE);

        priv = GET_PRIVATE (conf);

        /*gconf_client_notify_remove (priv->gconf_client, id);*/

        return TRUE;
}
