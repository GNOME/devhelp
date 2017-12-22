#!/usr/bin/env python
import formatter
import htmllib
import sgmllib
import string
import sys
import os.path
import os

SKIP_DATA = [ "Next Page", "Previous Page", "Home", "Next", "Up", "<", ">"]
class HTMLParser (htmllib.HTMLParser):
    is_a = 0
    a = []
    link = ""
    def anchor_bgn (self, href, name, type):
        self.is_a = 1
        self.link = href

    def anchor_end (self):
        self.is_a = 0

    def handle_data (self, data):
        data = string.strip (data)
        if data in SKIP_DATA:
            return

        if not '#' in self.link:
            return

        if self.link[:2] == "..":
            return
            
        if self.is_a and self.link:
            self.a.append ((data, self.link))

def parse_file (filename, bookname):
    fd = open (filename)
    try:
	p = HTMLParser (formatter.NullFormatter())
	p.feed (fd.read())
	p.close()
    except KeyboardInterrupt:
	raise SystemExit
    return p.a

dirname = os.path.abspath (sys.argv[1])
bookname = os.path.basename (os.path.abspath (sys.argv[1]))
files = os.listdir (dirname)
files.sort()

funcs = []
for file in files:
    if file[-5:] != ".html":
        continue

    print "parsing", file
    links = parse_file (dirname + "/" + file, bookname)
	
    for link in links:
	if not link in funcs:
	    funcs.append (link)

print "Sorting function list"
funcs.sort()

filename = "%s/%s.index" % (dirname, bookname)
print "Writing index to", filename

fd = open (filename, "w")

fd.write ("<functions>\n")
for name, link in funcs:
    if ' ' in name or '\n' in name:
	continue
    fd.write ('  <function name="%s" link="%s"/>\n' % (name, link))
fd.write ("</functions>\n</book>\n")

fd.close()

