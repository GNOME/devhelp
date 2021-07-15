Title: API Breaks
Slug: api-breaks

Devhelp is a quite old piece of software (created in 2001), and as every
software, the code evolves during its lifetime. So there are sometimes API
breaks. We try to not break applications depending on the Devhelp API. But
if we see that a certain API is used nowhere else than the Devhelp
application itself (or is dead code), we don't hesitate to break the API to
improve the code.

Currently we try to not break [Anjuta](https://wiki.gnome.org/Apps/Anjuta)
and [GNOME Builder](https://wiki.gnome.org/Apps/Builder). If your
application is not listed and depends on the Devhelp API and is Free
Software, please contact the Devhelp maintainers.

Changes between 3.29.1 and 3.29.2
---------------------------------

- `DhBookManager` is now completely empty and entirely deprecated, the
  minimum amount of API is kept to not break Anjuta. It has been replaced by
  a more flexible infrastructure: [`class@Profile`], [`class@Settings`] and
  [`class@BookList`]. If your application uses only the default libdevhelp
  widgets it will still work fine – even if you don't adapt the code of the
  application – because in that case it will use the default `DhProfile`
  which contains the same content as how `DhBookManager` was implemented.

- Whether a DhBook is enabled/selected is now decoupled from DhBook:
  `dh_book_get_enabled()` and `dh_book_set_enabled()` have been removed, as
  well as the `DhBook::enabled` and `DhBook::disabled` signals. For a book
  to be enabled it now needs to be part of a [`class@BookList`], which is more
  flexible because there can be several different `DhBookList`'s in parallel.

- The last parameter of `dh_keyword_model_filter()` has been changed, it is
  no longer the language (a string, that parameter was deprecated), it is a
  nullable [`class@Profile`].

- `dh_book_tree_get_selected_book()` has been replaced by
  [`method@BookTree.get_selected_link`].

Changes between 3.28.0 and 3.29.1
---------------------------------

- The `DhBookManager:group-by-language` property has been replaced by the
  [`property@Settings:group-books-by-language`] one.

- [`ctor@BookTree.new`] now takes a [`class@Profile`] parameter.

Changes between 3.27.1 and 3.27.2
---------------------------------

- `dh_book_cmp_by_path()` has been removed (dead code).

- The `DhBookManager::language-enabled` and
  `DhBookManager::language-disabled` signals have been removed (dead code).

- [`class@Sidebar`] is now a subclass of [`class@Gtk.Grid`], not of
  [`class@Gtk.Box`].

- `dh_sidebar_get_selected_book()` has been removed (it was used only inside
  `DhSidebar`).

- `dh_book_get_completions()` has been replaced by
  [`method@Book.get_completion`].

Changes between 3.26.0 and 3.27.1
---------------------------------

- [`ctor@Link.new`] has been split in two, with [`ctor@Link.new_book`] to
  create a [`struct@Link`] of `DH_LINK_TYPE_BOOK`.

- The `dh_link_get_file_name()` function has been removed.

- The `dh_book_get_path()` function has been replaced by
  [`method@Book.get_index_file`].

- The [`ctor@Book.new`] constructor now takes a `GFile` argument instead of
  a string path.

- `dh_book_get_name()` has been renamed to [`method@Book.get_id`].

- `dh_book_cmp_by_name()` has been renamed to [`method@Book.cmp_by_id`].

- `dh_link_get_book_name()` has been renamed to
  [`method@Link.get_book_title`].

- `dh_book_get_keywords()` has been renamed to [`method@Book.get_links`].

- The ownership transfer of the return values of
  `dh_book_tree_get_selected_book()` and `dh_sidebar_get_selected_book()`
  have been changed from `(transfer none)` to `(transfer full)`.

Changes between 3.25.1 and 3.25.2
---------------------------------

- The `page` parameter of [`ctor@Link.new`] has been removed because it was
  broken in `dh-parser.c`.  The `book` parameter has also been moved, to
  group related parameters together.

- The `dh_link_get_page_name()` function has been removed because it was
  broken and used nowhere.

- The `dh_link_get_type_as_string()` function (which took a
  [`struct@Link`] parameter) has been removed, and it has been replaced by [`func@LinkType.to_string`]
  which takes a [`enum@LinkType`] parameter.

Changes between 3.24.0 and 3.25.1
---------------------------------

- All deprecated APIs have been removed.

- `dh-error.h` is now private.

- The `DhApp`, `DhAssistant` and `DhWindow` classes are now private. `DhApp`
  is a subclass of `GtkApplication`, and an application can have only one
  `GtkApplication` instance, so as-is `DhApp` didn't make sense in the
  library (what if two different libraries have both a subclass of
  `GtkApplication`?). Since `DhAssistant` and `DhWindow` depend on `DhApp`,
  they are now also private.

- The `DhLanguage` class is now private, it's currently used
  only internally by [`class@BookManager`].

- Due to [`class@BookManager`] being now a singleton, there has been the
  following API changes:

  - `dh_assistant_view_set_book_manager()` has been
   removed.

  - `dh_keyword_model_set_words()` has been removed.

  - The `DhBookTree:book-manager` property has been removed.

  - API break for [`ctor@BookTree.new`]/

  - The `DhSidebar:book-manager` property has been removed.

  - The `book_manager` parameter of [`ctor@Sidebar.new`] is no deprecated.
