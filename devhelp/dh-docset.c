/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * SPDX-FileCopyrightText: 2020 Ayman Bagabas <ayman.bagabas@gmail.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"
#include "dh-docset.h"
#include <libxml/parser.h>
#include <sqlite3.h>

/**
 * SECTION: dh-docset
 * @title: DhDocset
 * @short_description: TODO
 *
 * TODO
 */

/**
 * DhDocset:
 *
 * Data structure representing a #DhDocset.
 *
 * Since: UNRELEASED
 */

/**
 * DhDocsetClass:
 *
 * The class of a #DhDocset.
 *
 * Since: UNRELEASED
 */

#define DOCSET_FILE_DB_PATH "/Contents/Resources/docSet.dsidx"
#define DOCSET_FILE_PLIST_PATH "/Contents/Info.plist"

enum {
        PROP_0,
        N_PROPS
};

enum {
        SIGNAL_0,
        N_SIGNALS
};

struct _DhDocsetPrivate {
        GFile *root_dir;
        GHashTable *props;
        sqlite3 *db;
        sqlite3_stmt *stmt;
};

G_DEFINE_TYPE_WITH_PRIVATE(DhDocset, dh_docset, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPS] = {NULL};

static guint signals[N_SIGNALS] = {0};

DhLinkType
dh_docset_link_type(DhDocsetType type)
{
    switch (type) {
    }
}

GList *
dh_docset_find (const gchar *pattern)
{
}

gint
xml_node_name_cmp (const xmlChar *left,
                   const gchar *right)
{
    return xmlStrcmp(left, (const xmlChar *)right);
}

xmlNode *
parse_props_get_dict_node (xmlNode *tree)
{
    xmlNode *node;
    if (tree && xml_node_name_cmp(tree->name, "plist") == 0) {
        for (node = tree->children; node; node = node->next) {
            if (node->type == XML_ELEMENT_NODE) {
                if (xml_node_name_cmp(node->name, "dict") == 0) {
                    return node->children;
                }
            }
        }
    }

    return NULL;
}

GHashTable *
parse_props_file (const gchar *root_path)
{
    g_return_val_if_fail (root_path != NULL, NULL);

    GHashTable *props;
    xmlDocPtr doc = NULL;
    xmlNode *root = NULL;
    xmlNode *dict = NULL;
    gchar *file_path = NULL;
    gchar *key = NULL;
    gchar *value = NULL;

    file_path = g_strconcat (root_path, "/Contents/Info.plist", NULL);
    props = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    doc = xmlParseFile(file_path);
    if (!doc) {
        g_warning("Failed to parse build props tree.\n");
        g_object_unref (props);
        props = NULL;
        goto clean;
    }

    root = xmlDocGetRootElement(doc);
    if (!root) {
        g_warning("Failed to get props document root element.\n");
        g_object_unref (props);
        props = NULL;
        goto clean;
    }

    xmlChar *content;
    for (dict = parse_props_get_dict_node(root); dict; dict = dict->next) {
        if (dict->type == XML_ELEMENT_NODE) {
            if (xml_node_name_cmp(dict->name, "key") == 0) {
                content = xmlNodeGetContent(dict);
                key = g_strdup((gchar *)content);
                xmlFree (content);
            }

            if (xml_node_name_cmp(dict->name, "string") == 0) {
                content = xmlNodeGetContent(dict);
                value = g_strdup((gchar *)content);
                xmlFree (content);
            }

            if (xml_node_name_cmp(dict->name, "true") == 0 ||
                xml_node_name_cmp(dict->name, "false") == 0) {
                value = g_strdup_printf("%d",
                        xml_node_name_cmp(dict->name, "true") == 0);
            }

            if (key && value) {
                if (!g_hash_table_insert(props, key, value))
                    g_warning("Duplicate key while parsing docset props: %s.", key);
                key = value = NULL;
            } else {
                g_free (key);
                g_free (value);
            }
        }
    }

clean:
    xmlFreeDoc (doc);
    xmlCleanupParser ();
    g_free (file_path);
    return props;
}

sqlite3 *
open_docset_db (const gchar *root_dir)
{
    g_return_val_if_fail(root_dir != NULL, NULL);

    sqlite3 *db;
    gint err = 0;
    gchar *file_path = NULL;

    file_path = g_strconcat(root_dir,  "/Contents/Resources/docSet.dsidx", NULL);
    err = sqlite3_open_v2 (file_path, &db, SQLITE_OPEN_READONLY, NULL);
    if (err != SQLITE_OK) {
        g_warning ("Failed to open sqlite3 database: %s\n",
                   sqlite3_errstr (err));
        db = NULL;
        goto exit;
    }

exit:
    g_free (file_path);
    return db;
}

DhDocset *dh_docset_new(const gchar *path) {
        g_return_val_if_fail (path != NULL, NULL);

        DhDocset *self;
        DhDocsetPrivate *priv;
        GFile *root_dir = NULL;
        gchar *root_dir_path = NULL;
        GHashTable *props = NULL;
        sqlite3 *db = NULL;

        self = g_object_new (DH_TYPE_DOCSET, NULL);
        priv = dh_docset_get_instance_private (self);
        root_dir = g_file_new_for_path (path);
        root_dir_path = g_file_get_path (root_dir);
        props = parse_props_file(root_dir_path);
        if (props == NULL) {
            g_object_unref (self);
            self = NULL;
            goto exit;
        }

        db = open_docset_db (root_dir_path);
        if (db == NULL) {
            g_object_unref (self);
            self = NULL;
            goto exit;
        }

        priv->root_dir = g_object_ref (root_dir);
        priv->props = g_object_ref (props);
        priv->db = db;
        priv->stmt = NULL;

exit:
        g_object_unref(props);
        g_free(root_dir_path);
        g_object_unref(root_dir);
        return self;
}

static void dh_docset_init(DhDocset *self) {
        self->priv = dh_docset_get_instance_private(self);
}

static void dh_docset_get_property(GObject *object,
                                   guint property_id,
                                   GValue *value,
                                   GParamSpec *pspec) {
        DhDocset *self;
        DhDocsetPrivate *priv;

        self = DH_DOCSET(object);
        priv = dh_docset_get_instance_private(DH_DOCSET(object));

        switch (property_id) {
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
                        break;
        }
}

static void dh_docset_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec) {
        DhDocset *self;
        DhDocsetPrivate *priv;

        self = DH_DOCSET(object);
        priv = dh_docset_get_instance_private(DH_DOCSET(object));

        switch (property_id) {
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
                        break;
        }
}

static void dh_docset_dispose(GObject *object) {
        DhDocset *self;
        DhDocsetPrivate *priv;
        gint err = 0;

        self = DH_DOCSET(object);
        priv = dh_docset_get_instance_private(DH_DOCSET(object));

        g_clear_object (&priv->root_dir);
        g_hash_table_remove_all(priv->props);
        g_clear_object(&priv->props);

        err = sqlite3_finalize(priv->stmt);
        if (err != SQLITE_OK)
            g_warning ("Clearing database statement failed %d %s\n",
                    err,
                    sqlite3_errstr (err));

        err = sqlite3_close_v2(priv->db);
        if (err != SQLITE_OK)
            g_warning ("Closing database failed %d %s\n",
                    err,
                    sqlite3_errstr (err));

        G_OBJECT_CLASS(dh_docset_parent_class)->dispose(object);
}

static void dh_docset_finalize(GObject *object) {
        DhDocset *self;
        DhDocsetPrivate *priv;

        self = DH_DOCSET(object);
        priv = dh_docset_get_instance_private(DH_DOCSET(object));

        G_OBJECT_CLASS(dh_docset_parent_class)->finalize(object);
}

static void dh_docset_constructed(GObject *object) {
        DhDocset *self;
        DhDocsetPrivate *priv;

        self = DH_DOCSET(object);
        priv = dh_docset_get_instance_private(DH_DOCSET(object));

        G_OBJECT_CLASS(dh_docset_parent_class)->constructed(object);
}

static void dh_docset_class_init(DhDocsetClass *klass) {
        GObjectClass *object_class = G_OBJECT_CLASS(klass);

        object_class->get_property = dh_docset_get_property;
        object_class->set_property = dh_docset_set_property;
        object_class->constructed = dh_docset_constructed;
        object_class->dispose = dh_docset_dispose;
        object_class->finalize = dh_docset_finalize;

        g_object_class_install_properties(object_class, N_PROPS, obj_properties);
}
