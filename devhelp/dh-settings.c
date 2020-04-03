/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* SPDX-FileCopyrightText: 2012 Thomas Bechtold <toabctl@gnome.org>
 * SPDX-FileCopyrightText: 2017, 2018 Sébastien Wilmet <swilmet@gnome.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"
#include "dh-settings.h"
#include "dh-settings-builder.h"

/**
 * SECTION:dh-settings
 * @Title: DhSettings
 * @Short_description: libdevhelp settings
 *
 * #DhSettings represents the libdevhelp settings. The libdevhelp provides a
 * #GSettings schema, but for some settings the use of #GSettings is optional:
 * it is instead possible to set the #DhSettings property and do not bind it to
 * the #GSettings key, to force a certain value and to not provide a user
 * configuration for it in the application.
 *
 * To have the documentation about the available #GSettings keys, their types,
 * the default values, etc, read the `*.gschema.xml` file.
 *
 * Note that #DhSettings do not expose the #GSettings objects, you should use
 * the #DhSettings wrapper API instead. So instead of using g_settings_bind(),
 * you should use g_object_bind_property() with a #DhSettings property as the
 * source. This has the small drawback that the writability of the #GSettings
 * keys are not exposed, it is assumed that all keys are writable; if knowing
 * the writability of the #GSettings keys is really wanted, the #DhSettings API
 * can be changed to expose publicly the #GSettings objects (for that purpose
 * only).
 *
 * # Different default value
 *
 * If you want to save a #DhSettings setting with #GSettings, but with a
 * different default value than the one provided by the libdevhelp #GSettings
 * schema, the recommended thing to do is to *not* call the bind function
 * provided by #DhSettings for that property, provide your own #GSettings schema
 * with the same key but with a different default value, and call yourself
 * g_settings_bind().
 *
 * # GSettings keys without corresponding properties
 *
 * For the following #GSettings keys, the use of #GSettings is mandatory, there
 * are no corresponding #DhSettings properties (and thus no bind function).
 *
 * - `"books-disabled"`: if the use of #GSettings is not wanted, a custom
 *   #DhBookList can be created instead.
 */

/* API design:
 *
 * There is no dh_settings_set_default() function. Because what if the default
 * instance is changed when the app is already initialized? There is no signal
 * to be notified. So there is only one default DhSettings possible, and it is
 * defined by the libdevhelp (not the app).
 */

/* TODO Possible DhSettings improvements:
 *
 * GSettings is usually used only in an application, not in a library. The
 * purpose of DhSettings is to have the minimum amount of code or work to do in
 * the applications, with the goal to create a software product line for
 * Devhelp (create several similar products based on the libdevhelp).
 *
 * But I'm not entirely satisfied by the current DhSettings API (it is the first
 * implementation iteration with the GSettings schema being in the library).
 * Here are some thoughts to make it better.
 *
 * More generally, there needs to be a reflection on how best to use GSettings
 * in a library. And maybe there is a very simple solution that I didn't see.
 *
 * -----
 *
 * Idea: leverage other GSettings backends instead of using the default one? For
 * example the memory GSettings backend, see g_memory_settings_backend_new().
 * Have both backends, the default GSettings backend plus the memory backend,
 * and optionally bind the keys between the two?
 *
 * -----
 *
 * Have access to all GSettings features, be able to choose a different backend
 * etc.
 *
 * -----
 *
 * Thing not to forget: it's also possible to create _interfaces_, of course, in
 * addition to classes.
 *
 * Maybe having one or two interfaces would help: an app can have a custom
 * implementation. But the purpose of DhSettings is that it's used by the other
 * libdevhelp classes, and it would be nice for the other libdevhelp classes to
 * have access to all GSettings features. So providing an interface, with the
 * need to implement a custom implementation in order to access all GSettings
 * features, may not help in all cases for the other libdevhelp classes to
 * access those features (the other libdevhelp classes would depend only on the
 * DhSettingsSomethingInterface, not the custom app implementation, of course).
 *
 * But maybe an interface would help for DhSettingsBuilder, to have more control
 * when creating the GSettings objects.
 *
 * -----
 *
 * To continue in the same direction as the current DhSettings* implementation:
 *
 * To make it better, it would require lots of boilerplate code, so I was maybe
 * thinking about a code generation tool that reads the GSettings XML schema and
 * creates a GObject class with some properties and functions for each key. Then
 * DhSettings would come on top of that generated class (either as a subclass or
 * using it by composition) to add more specific functions like
 * is_book_enabled(), set_book_enabled(), etc.
 *
 * The code generation tool would do something along those lines:
 * - Add a property of type GVariant for each GSettings key.
 * - Add getters/setters for the GVariant properties (bonus if for simple types
 *   like integers, strings, booleans, the getters/setters use the appropriate
 *   GLib type, not GVariant. Or find a way to make it convenient to use GVariant
 *   to get/set the properties).
 * - Add bind_to_key() functions, to bind a GVariant property to its corresponding
 *   GSettings key.
 * - For each key, add another property of type boolean to know whether the
 *   GVariant property has been bound to the GSettings key. Same name as the
 *   GVariant property, but with the -bound suffix.
 * - Add one bind() wrapper function with an API like g_settings_bind(), but calls
 *   g_object_bind_property() if -bound property is FALSE, and calls
 *   g_settings_bind() if -bound property is TRUE, to take advantage of the
 *   writability of the GSettings key.
 * - If more flexibility is needed, add getters to get the GSettings objects,
 *   those can be used for the keys where the -bound property is TRUE.
 *
 * That code generation tool would be useful outside Devhelp, for any project
 * that wants to use GSettings in a library or software product line. But it can
 * be prototyped in the libdevhelp.
 *
 * -----
 *
 * Another solution is to improve GSettings itself by contributing to GIO,
 * instead of trying to find a solution outside with the existing GSettings API.
 * Yes, it's always possible to externally add another layer of indirection, but
 * it would be a bit stupid to end up duplicating the whole GSettings API.
 */

/* libdevhelp GSettings schema IDs */
#define LIBDEVHELP_GSCHEMA_PREFIX       "org.gnome.libdevhelp-" LIBDEVHELP_API_VERSION
#define SETTINGS_SCHEMA_ID_CONTENTS     LIBDEVHELP_GSCHEMA_PREFIX ".contents"
#define SETTINGS_SCHEMA_ID_FONTS        LIBDEVHELP_GSCHEMA_PREFIX ".fonts"

/* Provided by the gsettings-desktop-schemas module. */
#define SETTINGS_SCHEMA_ID_DESKTOP_INTERFACE    "org.gnome.desktop.interface"
#define SYSTEM_FIXED_FONT_KEY                   "monospace-font-name"
#define SYSTEM_VARIABLE_FONT_KEY                "font-name"

struct _DhSettingsPrivate {
        GSettings *gsettings_contents;
        GSettings *gsettings_fonts;
        GSettings *gsettings_desktop_interface;

        /* List of book IDs (gchar*) currently disabled. */
        GList *books_disabled;

        gchar *variable_font;
        gchar *fixed_font;

        guint group_books_by_language : 1;
        guint use_system_fonts : 1;
};

enum {
        PROP_0,
        PROP_GROUP_BOOKS_BY_LANGUAGE,
        PROP_USE_SYSTEM_FONTS,
        PROP_VARIABLE_FONT,
        PROP_FIXED_FONT,
        N_PROPERTIES
};

enum {
        SIGNAL_BOOKS_DISABLED_CHANGED,
        SIGNAL_FONTS_CHANGED,
        N_SIGNALS
};

static GParamSpec *properties[N_PROPERTIES];
static guint signals[N_SIGNALS] = { 0 };
static DhSettings *default_instance = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (DhSettings, dh_settings, G_TYPE_OBJECT);

static void
load_books_disabled (DhSettings *settings)
{
        gchar **books_disabled_strv;
        gint i;

        g_list_free_full (settings->priv->books_disabled, g_free);
        settings->priv->books_disabled = NULL;

        books_disabled_strv = g_settings_get_strv (settings->priv->gsettings_contents,
                                                   "books-disabled");

        if (books_disabled_strv == NULL)
                return;

        for (i = 0; books_disabled_strv[i] != NULL; i++) {
                gchar *book_id = books_disabled_strv[i];
                settings->priv->books_disabled = g_list_prepend (settings->priv->books_disabled, book_id);
        }

        settings->priv->books_disabled = g_list_reverse (settings->priv->books_disabled);

        g_free (books_disabled_strv);
}

static void
store_books_disabled (DhSettings *settings)
{
        GVariantBuilder *builder;
        GVariant *variant;
        GList *l;

        builder = g_variant_builder_new (G_VARIANT_TYPE_STRING_ARRAY);

        for (l = settings->priv->books_disabled; l != NULL; l = l->next) {
                const gchar *book_id = l->data;
                g_variant_builder_add (builder, "s", book_id);
        }

        variant = g_variant_builder_end (builder);
        g_variant_builder_unref (builder);

        g_settings_set_value (settings->priv->gsettings_contents, "books-disabled", variant);
}

static GList *
find_in_books_disabled (DhSettings  *settings,
                        const gchar *book_id)
{
        GList *node;

        for (node = settings->priv->books_disabled; node != NULL; node = node->next) {
                const gchar *cur_book_id = node->data;

                if (g_strcmp0 (book_id, cur_book_id) == 0)
                        return node;
        }

        return NULL;
}

static void
enable_book (DhSettings  *settings,
             const gchar *book_id)
{
        GList *node;

        node = find_in_books_disabled (settings, book_id);

        /* Already enabled. */
        if (node == NULL)
                return;

        g_free (node->data);
        settings->priv->books_disabled = g_list_delete_link (settings->priv->books_disabled, node);

        store_books_disabled (settings);
}

static void
disable_book (DhSettings  *settings,
              const gchar *book_id)
{
        GList *node;

        node = find_in_books_disabled (settings, book_id);

        /* Already disabled. */
        if (node != NULL)
                return;

        settings->priv->books_disabled = g_list_append (settings->priv->books_disabled,
                                                        g_strdup (book_id));
        store_books_disabled (settings);
}

static void
dh_settings_books_disabled_changed_default (DhSettings *settings)
{
        load_books_disabled (settings);
}

static void
books_disabled_changed_cb (GSettings  *gsettings_contents,
                           gchar      *key,
                           DhSettings *settings)
{
        g_signal_emit (settings, signals[SIGNAL_BOOKS_DISABLED_CHANGED], 0);
}

static void
dh_settings_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
        DhSettings *settings = DH_SETTINGS (object);

        switch (prop_id) {
                case PROP_GROUP_BOOKS_BY_LANGUAGE:
                        g_value_set_boolean (value, dh_settings_get_group_books_by_language (settings));
                        break;

                case PROP_USE_SYSTEM_FONTS:
                        g_value_set_boolean (value, dh_settings_get_use_system_fonts (settings));
                        break;

                case PROP_VARIABLE_FONT:
                        g_value_set_string (value, dh_settings_get_variable_font (settings));
                        break;

                case PROP_FIXED_FONT:
                        g_value_set_string (value, dh_settings_get_fixed_font (settings));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_settings_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
        DhSettings *settings = DH_SETTINGS (object);

        switch (prop_id) {
                case PROP_GROUP_BOOKS_BY_LANGUAGE:
                        dh_settings_set_group_books_by_language (settings, g_value_get_boolean (value));
                        break;

                case PROP_USE_SYSTEM_FONTS:
                        dh_settings_set_use_system_fonts (settings, g_value_get_boolean (value));
                        break;

                case PROP_VARIABLE_FONT:
                        dh_settings_set_variable_font (settings, g_value_get_string (value));
                        break;

                case PROP_FIXED_FONT:
                        dh_settings_set_fixed_font (settings, g_value_get_string (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                        break;
        }
}

static void
dh_settings_dispose (GObject *object)
{
        DhSettings *settings = DH_SETTINGS (object);

        g_clear_object (&settings->priv->gsettings_contents);
        g_clear_object (&settings->priv->gsettings_fonts);
        g_clear_object (&settings->priv->gsettings_desktop_interface);

        G_OBJECT_CLASS (dh_settings_parent_class)->dispose (object);
}

static void
dh_settings_finalize (GObject *object)
{
        DhSettings *settings = DH_SETTINGS (object);

        g_list_free_full (settings->priv->books_disabled, g_free);
        g_free (settings->priv->variable_font);
        g_free (settings->priv->fixed_font);

        if (default_instance == settings)
                default_instance = NULL;

        G_OBJECT_CLASS (dh_settings_parent_class)->finalize (object);
}

static void
dh_settings_class_init (DhSettingsClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        klass->books_disabled_changed = dh_settings_books_disabled_changed_default;

        object_class->get_property = dh_settings_get_property;
        object_class->set_property = dh_settings_set_property;
        object_class->dispose = dh_settings_dispose;
        object_class->finalize = dh_settings_finalize;

        /**
         * DhSettings:group-books-by-language:
         *
         * Whether books should be grouped by programming language in the UI.
         *
         * Since: 3.30
         */
        properties[PROP_GROUP_BOOKS_BY_LANGUAGE] =
                g_param_spec_boolean ("group-books-by-language",
                                      "Group books by language",
                                      "",
                                      FALSE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT |
                                      G_PARAM_STATIC_STRINGS);

        /**
         * DhSettings:use-system-fonts:
         *
         * Whether to use the system default fonts.
         *
         * Since: 3.30
         */
        properties[PROP_USE_SYSTEM_FONTS] =
                g_param_spec_boolean ("use-system-fonts",
                                      "use-system-fonts",
                                      "",
                                      TRUE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT |
                                      G_PARAM_STATIC_STRINGS);

        /**
         * DhSettings:variable-font:
         *
         * Font for text with variable width.
         *
         * This property is independent of #DhSettings:use-system-fonts.
         *
         * Since: 3.30
         */
        properties[PROP_VARIABLE_FONT] =
                g_param_spec_string ("variable-font",
                                     "variable-font",
                                     "",
                                     "Sans 12",
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_STATIC_STRINGS);

        /**
         * DhSettings:fixed-font:
         *
         * Font for text with fixed width, such as code examples.
         *
         * This property is independent of #DhSettings:use-system-fonts.
         *
         * Since: 3.30
         */
        properties[PROP_FIXED_FONT] =
                g_param_spec_string ("fixed-font",
                                     "fixed-font",
                                     "",
                                     "Monospace 12",
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);

        /**
         * DhSettings::books-disabled-changed:
         * @settings: the #DhSettings emitting the signal.
         *
         * The ::books-disabled-changed signal is emitted when the
         * "books-disabled" #GSettings key changes.
         *
         * Since: 3.30
         */
        signals[SIGNAL_BOOKS_DISABLED_CHANGED] =
                g_signal_new ("books-disabled-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (DhSettingsClass, books_disabled_changed),
                              NULL, NULL, NULL,
                              G_TYPE_NONE, 0);

        /**
         * DhSettings::fonts-changed:
         * @settings: the #DhSettings emitting the signal.
         *
         * The ::fonts-changed signal is emitted when the return values of
         * dh_settings_get_selected_fonts() have potentially changed.
         *
         * Since: 3.30
         */
        signals[SIGNAL_FONTS_CHANGED] =
                g_signal_new ("fonts-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (DhSettingsClass, fonts_changed),
                              NULL, NULL, NULL,
                              G_TYPE_NONE, 0);
}

static void
system_font_changed_cb (GSettings  *gsettings,
                        gchar      *key,
                        DhSettings *settings)
{
        if (settings->priv->use_system_fonts)
                g_signal_emit (settings, signals[SIGNAL_FONTS_CHANGED], 0);
}

static void
dh_settings_init (DhSettings *settings)
{
        settings->priv = dh_settings_get_instance_private (settings);

        settings->priv->gsettings_desktop_interface = g_settings_new (SETTINGS_SCHEMA_ID_DESKTOP_INTERFACE);

        g_signal_connect_object (settings->priv->gsettings_desktop_interface,
                                 "changed::" SYSTEM_FIXED_FONT_KEY,
                                 G_CALLBACK (system_font_changed_cb),
                                 settings,
                                 0);

        g_signal_connect_object (settings->priv->gsettings_desktop_interface,
                                 "changed::" SYSTEM_VARIABLE_FONT_KEY,
                                 G_CALLBACK (system_font_changed_cb),
                                 settings,
                                 0);
}

DhSettings *
_dh_settings_new (const gchar *contents_path,
                  const gchar *fonts_path)
{
        DhSettings *settings;

        g_return_val_if_fail (contents_path != NULL, NULL);

        settings = g_object_new (DH_TYPE_SETTINGS, NULL);

        settings->priv->gsettings_contents = g_settings_new_with_path (SETTINGS_SCHEMA_ID_CONTENTS,
                                                                       contents_path);
        settings->priv->gsettings_fonts = g_settings_new_with_path (SETTINGS_SCHEMA_ID_FONTS,
                                                                    fonts_path);

        g_signal_connect_object (settings->priv->gsettings_contents,
                                 "changed::books-disabled",
                                 G_CALLBACK (books_disabled_changed_cb),
                                 settings,
                                 0);

        load_books_disabled (settings);

        return settings;
}

/**
 * dh_settings_get_default:
 *
 * Gets the default #DhSettings object. It has the default #GSettings paths (see
 * #DhSettingsBuilder) and dh_settings_bind_all() has been called.
 *
 * Returns: (transfer none): the default #DhSettings object.
 * Since: 3.30
 */
DhSettings *
dh_settings_get_default (void)
{
        if (default_instance == NULL) {
                DhSettingsBuilder *builder;

                builder = dh_settings_builder_new ();
                default_instance = dh_settings_builder_create_object (builder);
                dh_settings_bind_all (default_instance);
                g_object_unref (builder);
        }

        return default_instance;
}

void
_dh_settings_unref_default (void)
{
        if (default_instance != NULL)
                g_object_unref (default_instance);

        /* default_instance is not set to NULL here, it is set to NULL in
         * dh_settings_finalize() (i.e. when we are sure that the ref count
         * reaches 0).
         */
}

/**
 * dh_settings_bind_all:
 * @settings: a #DhSettings.
 *
 * Binds all the #DhSettings properties to their corresponding #GSettings keys.
 *
 * Since: 3.30
 */
void
dh_settings_bind_all (DhSettings *settings)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        dh_settings_bind_group_books_by_language (settings);
        dh_settings_bind_fonts (settings);
}

/**
 * dh_settings_get_group_books_by_language:
 * @settings: a #DhSettings.
 *
 * Returns: the value of the #DhSettings:group-books-by-language property.
 * Since: 3.30
 */
gboolean
dh_settings_get_group_books_by_language (DhSettings *settings)
{
        g_return_val_if_fail (DH_IS_SETTINGS (settings), FALSE);

        return settings->priv->group_books_by_language;
}

/**
 * dh_settings_set_group_books_by_language:
 * @settings: a #DhSettings.
 * @group_books_by_language: the new value.
 *
 * Sets the #DhSettings:group-books-by-language property.
 *
 * Since: 3.30
 */
void
dh_settings_set_group_books_by_language (DhSettings *settings,
                                         gboolean    group_books_by_language)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        group_books_by_language = group_books_by_language != FALSE;

        if (settings->priv->group_books_by_language != group_books_by_language) {
                settings->priv->group_books_by_language = group_books_by_language;
                g_object_notify_by_pspec (G_OBJECT (settings), properties[PROP_GROUP_BOOKS_BY_LANGUAGE]);
        }
}

/**
 * dh_settings_bind_group_books_by_language:
 * @settings: a #DhSettings.
 *
 * Binds the #DhSettings:group-books-by-language property to the corresponding
 * #GSettings key.
 *
 * Since: 3.30
 */
void
dh_settings_bind_group_books_by_language (DhSettings *settings)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        g_settings_bind (settings->priv->gsettings_contents, "group-books-by-language",
                         settings, "group-books-by-language",
                         G_SETTINGS_BIND_DEFAULT |
                         G_SETTINGS_BIND_NO_SENSITIVITY);
}

/**
 * dh_settings_is_book_enabled:
 * @settings: a #DhSettings.
 * @book: a #DhBook.
 *
 * Returns whether @book is enabled according to the "books-disabled" #GSettings
 * key. If the @book ID is present in "books-disabled", this function returns
 * %FALSE, otherwise %TRUE is returned.
 *
 * Returns: whether @book is enabled.
 * Since: 3.30
 */
gboolean
dh_settings_is_book_enabled (DhSettings *settings,
                             DhBook     *book)
{
        const gchar *book_id;

        g_return_val_if_fail (DH_IS_SETTINGS (settings), FALSE);
        g_return_val_if_fail (DH_IS_BOOK (book), FALSE);

        book_id = dh_book_get_id (book);
        return find_in_books_disabled (settings, book_id) == NULL;
}

/**
 * dh_settings_set_book_enabled:
 * @settings: a #DhSettings.
 * @book: a #DhBook.
 * @enabled: the new value.
 *
 * Modifies the "books-disabled" #GSettings key. It adds or removes the @book ID
 * from "books-disabled".
 *
 * Since: 3.30
 */
void
dh_settings_set_book_enabled (DhSettings *settings,
                              DhBook     *book,
                              gboolean    enabled)
{
        const gchar *book_id;

        g_return_if_fail (DH_IS_SETTINGS (settings));
        g_return_if_fail (DH_IS_BOOK (book));

        book_id = dh_book_get_id (book);

        if (enabled)
                enable_book (settings, book_id);
        else
                disable_book (settings, book_id);
}

/**
 * dh_settings_freeze_books_disabled_changed:
 * @settings: a #DhSettings.
 *
 * Tells @settings to not emit the #DhSettings::books-disabled-changed signal
 * until dh_settings_thaw_books_disabled_changed() is called.
 *
 * A bit like g_object_freeze_notify(), except that there is no freeze count.
 *
 * This function is useful if you call dh_settings_set_book_enabled() several
 * times in a row.
 *
 * Since: 3.30
 */
void
dh_settings_freeze_books_disabled_changed (DhSettings *settings)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        g_signal_handlers_block_by_func (settings->priv->gsettings_contents,
                                         books_disabled_changed_cb,
                                         settings);
}

/**
 * dh_settings_thaw_books_disabled_changed:
 * @settings: a #DhSettings.
 *
 * Stops the effect of dh_settings_freeze_books_disabled_changed(), and emits
 * the #DhSettings::books-disabled-changed signal.
 *
 * A bit like g_object_thaw_notify(), except that there is no freeze count.
 *
 * Since: 3.30
 */
void
dh_settings_thaw_books_disabled_changed (DhSettings *settings)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        g_signal_handlers_unblock_by_func (settings->priv->gsettings_contents,
                                           books_disabled_changed_cb,
                                           settings);

        /* Emit the signal in any case, the implementation is simpler and good
         * enough, it doesn't hurt to emit the signal even if the GSettings key
         * didn't change.
         */
        g_signal_emit (settings, signals[SIGNAL_BOOKS_DISABLED_CHANGED], 0);
}

/**
 * dh_settings_get_selected_fonts:
 * @settings: a #DhSettings.
 * @variable_font: (out): location to store the font name for text with variable
 *   width. Free with g_free().
 * @fixed_font: (out): location to store the font name for text with fixed
 *   width. Free with g_free().
 *
 * If #DhSettings:use-system-fonts is %TRUE, returns the system fonts. Otherwise
 * returns the values of the #DhSettings:variable-font and
 * #DhSettings:fixed-font properties.
 *
 * Since: 3.30
 */
void
dh_settings_get_selected_fonts (DhSettings  *settings,
                                gchar      **variable_font,
                                gchar      **fixed_font)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));
        g_return_if_fail (variable_font != NULL && *variable_font == NULL);
        g_return_if_fail (fixed_font != NULL && *fixed_font == NULL);

        if (settings->priv->use_system_fonts) {
                *variable_font = g_settings_get_string (settings->priv->gsettings_desktop_interface,
                                                        SYSTEM_VARIABLE_FONT_KEY);
                *fixed_font = g_settings_get_string (settings->priv->gsettings_desktop_interface,
                                                     SYSTEM_FIXED_FONT_KEY);
        } else {
                *variable_font = g_strdup (settings->priv->variable_font);
                *fixed_font = g_strdup (settings->priv->fixed_font);
        }
}

/**
 * dh_settings_get_use_system_fonts:
 * @settings: a #DhSettings.
 *
 * Returns: the value of the #DhSettings:use-system-fonts property.
 * Since: 3.30
 */
gboolean
dh_settings_get_use_system_fonts (DhSettings *settings)
{
        g_return_val_if_fail (DH_IS_SETTINGS (settings), FALSE);

        return settings->priv->use_system_fonts;
}

/**
 * dh_settings_set_use_system_fonts:
 * @settings: a #DhSettings.
 * @use_system_fonts: the new value.
 *
 * Sets the #DhSettings:use-system-fonts property.
 *
 * Since: 3.30
 */
void
dh_settings_set_use_system_fonts (DhSettings *settings,
                                  gboolean    use_system_fonts)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        use_system_fonts = use_system_fonts != FALSE;

        if (settings->priv->use_system_fonts != use_system_fonts) {
                settings->priv->use_system_fonts = use_system_fonts;
                g_object_notify_by_pspec (G_OBJECT (settings), properties[PROP_USE_SYSTEM_FONTS]);

                g_signal_emit (settings, signals[SIGNAL_FONTS_CHANGED], 0);
        }
}

/**
 * dh_settings_get_variable_font:
 * @settings: a #DhSettings.
 *
 * Warning: you probably want to use the dh_settings_get_selected_fonts()
 * function instead, to take into account the #DhSettings:use-system-fonts
 * property.
 *
 * Returns: the value of the #DhSettings:variable-font property.
 * Since: 3.30
 */
const gchar *
dh_settings_get_variable_font (DhSettings *settings)
{
        g_return_val_if_fail (DH_IS_SETTINGS (settings), NULL);

        return settings->priv->variable_font;
}

/**
 * dh_settings_set_variable_font:
 * @settings: a #DhSettings.
 * @variable_font: the new value.
 *
 * Sets the #DhSettings:variable-font property.
 *
 * Since: 3.30
 */
void
dh_settings_set_variable_font (DhSettings  *settings,
                               const gchar *variable_font)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));
        g_return_if_fail (variable_font != NULL);

        if (g_strcmp0 (settings->priv->variable_font, variable_font) != 0) {
                g_free (settings->priv->variable_font);
                settings->priv->variable_font = g_strdup (variable_font);
                g_object_notify_by_pspec (G_OBJECT (settings), properties[PROP_VARIABLE_FONT]);

                if (!settings->priv->use_system_fonts)
                        g_signal_emit (settings, signals[SIGNAL_FONTS_CHANGED], 0);
        }
}

/**
 * dh_settings_get_fixed_font:
 * @settings: a #DhSettings.
 *
 * Warning: you probably want to use the dh_settings_get_selected_fonts()
 * function instead, to take into account the #DhSettings:use-system-fonts
 * property.
 *
 * Returns: the value of the #DhSettings:fixed-font property.
 * Since: 3.30
 */
const gchar *
dh_settings_get_fixed_font (DhSettings *settings)
{
        g_return_val_if_fail (DH_IS_SETTINGS (settings), NULL);

        return settings->priv->fixed_font;
}

/**
 * dh_settings_set_fixed_font:
 * @settings: a #DhSettings.
 * @fixed_font: the new value.
 *
 * Sets the #DhSettings:fixed-font property.
 *
 * Since: 3.30
 */
void
dh_settings_set_fixed_font (DhSettings  *settings,
                            const gchar *fixed_font)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));
        g_return_if_fail (fixed_font != NULL);

        if (g_strcmp0 (settings->priv->fixed_font, fixed_font) != 0) {
                g_free (settings->priv->fixed_font);
                settings->priv->fixed_font = g_strdup (fixed_font);
                g_object_notify_by_pspec (G_OBJECT (settings), properties[PROP_FIXED_FONT]);

                if (!settings->priv->use_system_fonts)
                        g_signal_emit (settings, signals[SIGNAL_FONTS_CHANGED], 0);
        }
}

/**
 * dh_settings_bind_fonts:
 * @settings: a #DhSettings.
 *
 * Binds the #DhSettings:use-system-fonts, #DhSettings:variable-font and
 * #DhSettings:fixed-font properties to their corresponding #GSettings keys.
 *
 * Since: 3.30
 */
void
dh_settings_bind_fonts (DhSettings *settings)
{
        g_return_if_fail (DH_IS_SETTINGS (settings));

        g_settings_bind (settings->priv->gsettings_fonts, "use-system-fonts",
                         settings, "use-system-fonts",
                         G_SETTINGS_BIND_DEFAULT |
                         G_SETTINGS_BIND_NO_SENSITIVITY);

        g_settings_bind (settings->priv->gsettings_fonts, "variable-font",
                         settings, "variable-font",
                         G_SETTINGS_BIND_DEFAULT |
                         G_SETTINGS_BIND_NO_SENSITIVITY);

        g_settings_bind (settings->priv->gsettings_fonts, "fixed-font",
                         settings, "fixed-font",
                         G_SETTINGS_BIND_DEFAULT |
                         G_SETTINGS_BIND_NO_SENSITIVITY);
}
