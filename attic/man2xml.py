#!/usr/bin/env python
import os
import os.path
import string

# Add $GNOME_PATH/man too
MAN_PATH = ["/usr/man", "/usr/share/man", "/usr/local/man"]
MAN_SUBS = ["man1", "man2", "man3", "man4", "man5",
            "man6", "man7", "man8", "man9"]

MAN_NAME = { "man1" : "Applications",
             "man2" : "System Calls",
	     "man3" : "Library Functions",
	     "man4" : "Devices",
	     "man5" : "Configuration Files",
	     "man6" : "Games",
	     "man7" : "Conventions",
	     "man8" : "System Administration",
	     "man9" : "Kernel Routines"}

# 1. Get a list of all mandirs
chapters = []
for mandir in MAN_PATH:
    dir_list = os.listdir (mandir)
    for dir in dir_list:
	if os.path.isdir (mandir + "/" + dir) and dir in MAN_SUBS:
	    chapters.append (mandir + "/" + dir)

# 2. Get a list of all manpages
man_pages = {}
funcs = []
for mandir in chapters:
    list = os.listdir (mandir)
    for file in list:
	# Extract the name of the man page
	name, section = string.split (file, ".", 1)
	if section[-3:] == ".gz":
	    section = section[:-3]

	man = os.path.basename (mandir)
	filename = mandir + "/" + file

	# Check out section 2* and 3*, but not 3pm
	if section in ['2', '3'] and section[-2:] != "pm":
	    # Remove perl funtions
	    if string.find (name, "::") == -1:
		funcs.append ((name, filename))

	#  Does the the dict have the key?
	if not man_pages.has_key (man):
	    man_pages[man] = []

	if name == "." or not name:
	    continue

	man_pages[man].append ((name, filename))

# 3. Print out xml
print '<?xml version="1.0"?>'
print '<book title="Man pages">'
print

# 3.1 Print chapters
# Chapters
chapters.sort()
written = []
print '<chapters>'
for chap in chapters:
    name = os.path.basename (chap)
    if not man_pages.has_key (name):
	continue

    if name in written:
	continue
    written.append (name)

    print '  <chapter name="%s">' % MAN_NAME[name]
    list = man_pages[name]
    list.sort()

    for name, page in list:
	print '    <sub name="%s" link="%s"/>' % (name, page)
    print '  </chapter>'
    print

print '</chapters>'
print

# 3.2 print functions
# Functions
funcs.sort()
print '<functions>'
for name, file in funcs:
    print '  <function name="%s" link="%s"/>' % (name, file)
print '</functions>'
print
print '</book>'


