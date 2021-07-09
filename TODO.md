Possible things to do in Devhelp
================================

Normally all the codebase is in a good state, except `DhAssistant` because I
don't use it (and I think it's sometimes broken).

Some stuff that can be done:

Improve `DhBookList` API
------------------------

Make it easier to create a custom `DhBookList`. Maybe expose publicly
`DhBookListSimple` and be able to subclass it.

Have man-pages for the lib C
----------------------------

See [issue 38](https://gitlab.gnome.org/GNOME/devhelp/-/issues/38).

Improve `DhSettings`
--------------------

See the comment in [`dh-settings.c`](./devhelp/dh-settings.c).

Rename some classes
-------------------

Attention to not break apps like Anjuta and Builder, so renaming classes should
either be done in a backward-compatible way (deprecate the old classes, provide
the new ones in parallel and implement the old classes by using the new ones),
or it should be done for the next major version of the libdevhelp, with the
different major versions being parallel-installable (which means moving the
libdevhelp to its own git repo).

- `DhSidebar` -> `DhSidePanel`. A "bar" in GNOME is more for horizontal things,
  like `GtkInfoBar`.
- `DhKeywordModel` -> `DhSearchModel` ?
