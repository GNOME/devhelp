#!/usr/bin/env python
import os.path
import sgmllib
import string
import sys

def does_dict_have_keys (dict, keys):
    for key in keys:
	if not dict.has_key (key):
	    return 0
    if len(dict) != len(keys):
	return 0
    return 1

def walk (dict, level=0, parent=None):
    if dict.has_key ('order'):
	list = dict['order']
    else:
	list = dict.keys()

    for key in list:
	if key in ['name', 'order', 'link']:
	    continue
	if dict[key].has_key ('link') and  \
  	   does_dict_have_keys (dict[key], ['link']):
	    link = dict[key]['link']
	else:
	    link = ""

	if level:
	    print '*' * level, key, '-', link
	else:
	    print key, '-', link

	walk (dict[key], level + 1, dict)

class BookParser (sgmllib.SGMLParser):
    def __init__ (self):
	sgmllib.SGMLParser.__init__ (self)
	self.a = self.parents = []
	self.dict = {}
	self.last = self.link = ""
	self.is_a = self.level = 0
	self.first = 1

    def unknown_starttag (self, tag, attrs):
	if tag == 'a':
	    self.is_a = 1
	    for attr in attrs:
		if attr[0] == "href":
		    self.link = attr[1]
		    break

	if tag in ['dd', 'ul']:
	    self.parents.append (self.last)
	    self.level = self.level + 1

    def unknown_endtag (self, tag):
	if tag == 'a':
	    self.is_a = 0

	if tag in ['dd', 'ul']:
	    self.level = self.level - 1
	    self.parents.pop()

    def handle_data (self, data):
	data = string.strip (data)
	if not data or data in [ ">", "<" ]:
	    return

	if self.first:
	    self.dict['name'] = data
	    self.first = 0
	    return

	if data == self.dict['name'] or data in [ "Next Page", "Previous Page", "Home", "Next"]:
	    return

	if len (self.parents) == 0:
	    dict = self.dict
	elif len (self.parents) == 1:
	    dict = self.dict[self.parents[0]]
	elif len (self.parents) == 2:
	    dict = self.dict[self.parents[0]][self.parents[1]]
	elif len (self.parents) == 3:
	    dict = self.dict[self.parents[0]][self.parents[1]][self.parents[2]]
	else:
	    dict = None

	if self.is_a:
	    if dict == None:
		return

	    if not dict.has_key (data):
		dict[data] = {}
	    if not dict.has_key ('order'):
		dict['order'] = []
	    dict['order'].append (data)
	    dict[data]['link'] = self.link

	    self.last = data

def parse_book (url):
    if os.path.exists (url + "/index.html"):
	filename = url + "/index.html"
    elif os.path.exists (url + "/book1.html"):
	filename = url + "/book1.html"
    elif os.path.exists (url):
	filename = url
    else:
	print "Error; Can't find an index :("
	raise SystemExit

    fd = open (filename)
    p = BookParser()
    p.feed (fd.read())
    p.close()
    return p.dict

filename = sys.argv[1]

dict =  parse_book (sys.argv[1])

print '<?xml version="1.0"?>'
print '<book title="%s"\nname=""\nbase=""\nlink="%s">' % (dict['name'], os.path.basename (sys.argv[1]))

print '<chapters>'
for chap in dict['order']:
    print '  <sub name="%s" link="%s">' % (chap, dict[chap]['link'])
    if dict[chap].has_key ('order'):
        for sub in dict[chap]['order']:
            if not does_dict_have_keys (dict[chap][sub], ['link']):
                print '    <sub name="%s" link="%s">' % (sub, dict[chap][sub]['link'])

                for sub2 in dict[chap][sub]['order']:
                    print '      <sub name="%s" link="%s"/>' % (sub2, dict[chap][sub][sub2]['link'])
                print '    </sub>'
            else:
                print '    <sub name="%s" link="%s"/>' % (sub, dict[chap][sub]['link'])

    print '  </sub>'
    print

print '</chapters>'
print
print '</book>'

