# Contribution guidelines

Thank you for considering contributing to the Devhelp project!

These guidelines are meant for new contributors, regardless of their level
of proficiency; following them allows the maintainers of the Devhelp project
to more effectively evaluate your contribution, and provide prompt feedback
to you. Additionally, by following these guidelines you clearly communicate
that you respect the time and effort that the people developing Devhelp put
into managing the project.

Devhelp would not exist without contributions from the free and open source
software community. There are many things that we value:

 - bug reporting and fixing
 - documentation and examples
 - tests
 - new features

Please, do not use the issue tracker for support questions. If you have
questions on how to use Devhelp effectively, you can use:

 - the [devhelp tag on the GNOME Discourse instance](https://discourse.gnome.org/tag/devhelp)


The issue tracker is meant to be used for actionable issues only.

## How to report bugs

### Security issues

If you wish to report a security-related issue, please follow the
instructions available on the [GNOME security
website](https://security.gnome.org).

### Bug reports

If you're reporting a bug make sure to list:

 0. which version of Devhelp are you using?
 0. which operating system are you using?
 0. the necessary steps to reproduce the issue
 0. the expected outcome
 0. a description of the behavior; screenshots are also welcome

If the issue includes a crash, you should also include:

 0. the eventual warnings printed on the terminal
 0. a backtrace, obtained with tools such as GDB or LLDB

It is fine to include screenshots of screen recordings to demonstrate
an issue that is best to understand visually, but please don't just
dump screen recordings without further details into issues. It is
essential that the problem is described in enough detail to reproduce
it without watching a video.

For small issues, such as:

 - spelling/grammar fixes in the documentation
 - typo correction
 - comment clean ups
 - changes to metadata files (CI, `.gitignore`)
 - build system changes
 - source tree clean ups and reorganizations

You should directly open a merge request instead of filing a new issue.

### Features and enhancements

Feature discussion can be open ended and require high bandwidth channels; if
you are proposing a new feature on the issue tracker, make sure to make
an actionable proposal, and list:

 0. what you're trying to achieve
 0. prior art, in other toolkits or applications
 0. design and theming changes

If you're proposing the integration of new features it helps to have
multiple applications using shared or similar code, especially if they have
iterated over it various times.

Each feature should also come fully documented, and with tests.

## Your first contribution

### Prerequisites

If you want to contribute to the Devhelp project, you will need to have the
development tools appropriate for your operating system, including:

 - Python 3.x
 - Meson
 - Ninja
 - Gettext (19.7 or newer)
 - a [C99 compatible compiler](https://wiki.gnome.org/Projects/GLib/CompilerRequirements)

Up-to-date instructions about developing GNOME applications and libraries
can be found on [the GNOME Developer Center](https://developer.gnome.org).

The Devhelp project uses GitLab for code hosting and for tracking issues. More
information about using GitLab can be found [on the GNOME wiki](https://wiki.gnome.org/GitLab).

### Dependencies

Devhelp depends on various libraries that are part of the GNOME application
development platform:

 - GTK
 - WebKitGTK

Additionally, you will need the following settings module installed:

 - gsettings-desktop-schemas

### Getting started

You should start by forking the Devhelp repository from the GitLab web UI, and
cloning from your fork:

```sh
$ git clone https://gitlab.gnome.org/yourusername/devhelp.git
```

We recommend using [GNOME Builder](https://wiki.gnome.org/Apps/Builder),
which will automatically use the Flatpak manifest and build Devhelp in a
containerized environment.

To compile the Git version of Devhelp on your system, you will need to
configure your build using Meson:

```sh
$ meson setup _builddir .
$ meson compile -C _builddir
```

Typically, you should work on your own branch:

```sh
$ git switch -C your-branch
```

Once you've finished working on the bug fix or feature, push the branch
to the Git repository and open a new merge request, to let the Devhelp
maintainers review your contribution.

### Code reviews

Each contribution is reviewed by the maintainers of the Devhelp project.

Just remember that the maintainers are volunteers just like you, so they
might take some time to review your contributions. Please, be patient.

### Commit messages

The expected format for git commit messages is as follows:

```plain
Short explanation of the commit

Longer explanation explaining exactly what's changed, whether any
external or private interfaces changed, what bugs were fixed (with bug
tracker reference if applicable) and so forth. Be concise but not too
brief.

Closes #1234
```

 - Always add a brief description of the commit to the _first_ line of
 the commit and terminate by two newlines (it will work without the
 second newline, but that is not nice for the interfaces).

 - First line (the brief description) must only be one sentence and
 should start with a capital letter unless it starts with a lowercase
 symbol or identifier. Don't use a trailing period either. Don't exceed
 72 characters.

 - The main description (the body) is normal prose and should use normal
 punctuation and capital letters where appropriate. Consider the commit
 message as an email sent to the developers (or yourself, six months
 down the line) detailing **why** you changed something. There's no need
 to specify the **how**: the changes can be inlined.

 - When committing code on behalf of others use the `--author` option, e.g.
 `git commit -a --author "Awesome Coder <awesome@coder.org>"` and `--signoff`.

 - If your commit is addressing an issue, use the
 [GitLab syntax](https://docs.gitlab.com/ce/user/project/issues/automatic_issue_closing.html)
 to automatically close the issue when merging the commit with the upstream
 repository:

```plain
Closes #1234
Fixes #1234
Closes: https://gitlab.gnome.org/GNOME/devhelp/issues/1234
```

 - If you have a merge request with multiple commits and none of them
 completely fixes an issue, you should add a reference to the issue in
 the commit message, e.g. `Bug: #1234`, and use the automatic issue
 closing syntax in the description of the merge request.
