#!/usr/bin/perl

#
# * Copyright (C) 2004 Marc Britten <mbritten@monochromatic.net>
# *
# * This program is free software; you can redistribute it and/or
# * modify it under the terms of the GNU General Public License as
# * published by the Free Software Foundation; either version 2 of the
# * License, or (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# * General Public License for more details.
# *
# * You should have received a copy of the GNU General Public
# * License along with this program; if not, write to the
# * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# * Boston, MA 02111-1307, USA.
#

# We should be fully functional with search and all that
# if using -S its useful to use -t to make a special book for a specific API
# probably want to use -f at that point too.

use Getopt::Std;

$var = @ARGV;

# depending on distro you may have to change this
$cgi = "lynxcgi:/usr/lib/cgi-bin/man/man2html";

if($var < 1) {
  print "WARNING: this script deletes all html files found in current directory!!!\n\n";
  print "Please specify man section to convert and option flags.\n";
  print "Valid flags are:\n";
  print "-f name of .devhelp file.\n";
  print "-t title of book.\n";
  print "-S search string for manpages to include. Uses grep - very CPU intensive.\n";
  print "-s man section (manditory).\n";
  print "\n To make a specific API book use all 4 options.\n";
  print "bulkman.pl -f SDL.devbook -t \"SDL API Reference\" -S \"SDL API Reference\" -s 3\n\n";
  die " bulkman.pl <opt> -s section \n";
}

getopt("f:t:s=iS:");

$section = $opt_s;

if(!$section) {
  die "Please specify a section with -s number \n";
}

$HTML = "rman -f HTML -S -r '../man$section/%s.%s.html'";

if($opt_f ne "") {
  $xmlfile = $opt_f;
  ($name) = split(/\./, $opt_f);
  $index = $name . ".html";
}
else {
  $xmlfile = "man$section.devhelp";
  $index = "man$section.html";
}

print "HTML index page is being called: $index\n";

print "Cleaning up $xmlfile\n";
unlink($xmlfile);

print "Cleaning html files\n";
unlink(<*.html>);

print "Finding MANPATH's...\n";
open(MANHANDLE, "/etc/man.config") || open(MANHANDLE, "/etc/manpath.config");
while(<MANHANDLE>) {
  if(/\b^MANPATH\b/ || /\b^MANDATORY_MANPATH\b/) {
	chop($_);
	($junk, @path) = split(/\s/, $_);
	foreach $thing (@path) {
	  #something wicked in debian makes me haveto do this
	  if($thing ne "") {
		push(@dirs, $thing);
	  }
	}
  }
}

close(MANHANDLE);

if($opt_t) {
  $title = $opt_t;
}
else {
  $title = "man$section";
}

open(INDEX, ">$index");
open(XMLHANDLE, ">$xmlfile");
print XMLHANDLE "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n";
print XMLHANDLE "<book name=\"man$N\" version=\"0.1\" title=\"$title\" link=\"$index\">\n";
print XMLHANDLE "<chapters>\n";

print INDEX "<HTML>\n<HEAD>\n<TITLE>$title</TITLE>\n</HEAD>\n<BODY>\n<H2>$title</H2><P><UL>\n";

print "Creating XML file and HTML conversions.\n";

if($opt_S) {
  print "Using search string: $opt_S\n";
}
# Open each dir in turn and do our thing on em
# The gziped non-gziped dual support made this a little clunky.
# so did the man2html's inability to use the .so support in manfiles to link
foreach $dir (@dirs) {
  $mandir = "$dir//man$section";
  if(opendir(DIR, "$mandir")) {
	foreach $file (sort readdir(DIR)) {
	  #	  print "Test: $file\n";
	  $fullfile = $mandir . "//" . $file;
	  if( -f $fullfile) {
		if($opt_S) {
		  if($fullfile =~ /\.gz$/) {
			$syscmd = "gunzip -c $fullfile | grep -q \"$opt_S\"";
		  } else {
			$syscmd = "grep -q \"$opt_S\" $fullfile";
		  }
		  #system returns are backwards so success == failed == next
		  if((system "$syscmd")) {	
			#			print "failed 1st: $file\n";
			# OK the file failed but that doesn't mean it's not a real match.
			# reopen the file and see if it uses .so linking to another manpage.
			if($fullfile =~ /.gz$/) {
			  $syscmd = "gunzip -c $fullfile | grep -q \"\.so\sman.\"";
			} else {
			  $syscmd = "grep -q \"\.so\sman.\" $fullfile";
			}
			if(!(system "$syscmd")) {
			  print "good .so: $file\n";
			  if($fullfile =~ /.gz$/) {
				open(BADFILE, "gunzip -c $fullfile|");
			  } else {
				open(BADFILE, "$fullfile");
			  }
			  @dat = <BADFILE>;
			  ($junk, $data) = split(/\s/, $dat[0]);
			  if(-f "$dir/$data") {
				$syscmd = "grep -q \"$opt_S\" $dir/$data";
			  } elsif(-f "$dir/$data.gz") {
				$syscmd = "gunzip -c $dir/$data | grep -q \"$opt_S\" $dir/$data";
			  }
			  else {
				print "Cannot locate file $data\n";
				$syscmd = "/bin/false";
			  }
			  if((system "$syscmd")) {
				# failed again, skip the damn thing
				next;
			  }
			  else {
				#				print "good test: $syscmd\n";
			  }
			  close(BADFILE);
			}
			else {
			  next;
			}
		  }
		}

		#		print "Good: $file\n";
		if($fullfile =~ /.gz$/) {
			$file =~ s/.gz//g;
			(!system "gunzip -c $fullfile | $HTML > $file.html 2>/dev/null");
		} else {
			(!system "$HTML -n $file $fullfile > $file.html 2>/dev/null");
		}
		($page) = split(/\./, $file);
		print XMLHANDLE "<sub name=\"$page\" link=\"$file.html\">\n";
		print XMLHANDLE "</sub>\n";
		print INDEX "<LI><A HREF=\"$file.html\">$page</A><br>";
	  }
	}
	closedir(DIR);
  }
}

print XMLHANDLE "</chapters>\n";
print XMLHANDLE "<functions>\n";

print INDEX "</UL></BODY></HTML>";
close(INDEX);

print "Fixing up XML search info.\n";

# Now we do it again to setup functions
foreach $dir (@dirs) {
  $mandir = "$dir//man$section";
  if(opendir(DIR, "$mandir")) {
	foreach $file (sort readdir(DIR)) {
	  $fullfile = $mandir . "//" . $file;
	  if( -f $fullfile) {
		($page) = split(/\./, $file);
		print XMLHANDLE "<function name=\"$page\" link=\"$file.html\"/>\n"
	  }
	}
	closedir(DIR);
  }
}

print XMLHANDLE "</functions>\n";
print XMLHANDLE "</book>\n";

close(XMLHANDLE);

# Now its time to cleanup some of the mess.  Some of the manfiles use an
# internal map system to use an already existing man file
# man2html hates that, so we figure out which ones caused an error
# and use symlinks to simulate an existing file.

print "Fixing invalid conversions with links.\n";

open(GREP, "grep -l \"Invalid Manpage\" *.html|");
@bad = <GREP>;
close(GREP);

foreach $file (@bad) {
  foreach $dir (@dirs) {
	$mandir = "$dir//man$section";

	($page) = split(/\./, $file);
	$manfile = $page . "." . $section . ".gz";
	$fullfile = $mandir . "//" . $manfile;

	if(open(BADFILE, "gunzip -c $fullfile|")) {
	  @dat = <BADFILE>;
	  ($junk, $data) = split(/\s/, $dat[0]);
	  ($junk, $data) = split(/\//, $data);
	  $link = $data . ".gz.html";

	  chop($file);
	  unlink($file);
	  symlink($link, $file);

	  close(BADFILE);
	  last;
	}
  }
}

#die "testing no fixing\n";
# Yet another loop through.  This time we need to fixup man2html BS
print "Fixing man2html formatting.\n";

open(PWD, "pwd|");
($mydir) = <PWD>;
chop($mydir);
close(PWD);

opendir(MYDIR, $mydir) || die "can't open $mydir.";
foreach $file (sort readdir(MYDIR)) {
  if(-f $file) {
	($page) = split(/\./, $file);
	# Inplace editing trick from Perl Cookbook
	open(FILE, "+<$file");
	@data = <FILE>;
	#	print $file . "\n";
	foreach $line (@data) {
	  $_ = $line; # for regex only, need to mod $line to get file to change
	  $line =~ s/Content-type: text\/html//g;

	  while($line =~ /$cgi/g) {
		$line =~ s/\"$cgi\"/\"$index\"/g;
		$line =~ s/Return to Main Contents/Documentation Index/g;
		$line =~ s/<A HREF=\"#index\">Index<\/A>//g;

		m{<B><A.*>(.*)</[aA]}sx;
		# $1 should now be our manpage
		$link = $1;
		
		m{\((\d)\).}sx;
		# $1 should now be our man section
		$sec = $1;
		if($sec =~ /\D/) {
		  $sec = $opt_s;
		  if(/(3G)/) {
			$sec = $sec . "x";
		  }
		}
		if($link ne "") {
		  	  #print $file . ": " . $link . ": " . $sec . "\n";
		  $line =~ s/$cgi....//g;
			  #print $line;
		  $line =~ s/\"$link\"/\"$link.$sec.gz.html\"/g;
			  #print $line;
		}
		else {
		  last;
		}
	  }
	}
	seek(FILE, 0, 0);
	print FILE @data;
	truncate(FILE,tell(FILE));
	close(FILE);
  }
}
closedir(DIR);
