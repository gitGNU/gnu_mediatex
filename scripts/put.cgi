#!/usr/bin/perl -w
#=======================================================================
# * Version: $Id: search.cgi,v 1.1.1.1 2015/05/31 14:39:44 nroche Exp $
# * Project: MediaTex
# * Module : Upload cgi script
# *
# * This cgi-script make glue between put.shtml and put.sh scripts
#
# MediaTex is an Electronic Records Management System
# Copyright (C) 2014  Nicolas Roche
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#=======================================================================

use strict;
use utf8;
use CGI;
use CGI::Carp qw ( fatalsToBrowser );
use File::Basename;

$CGI::POST_MAX = 1024 * 1024 * 1024 * 5; # 5 Go
my $safe_filename_characters = "a-zA-Z0-9_.-";
my $uploadDir = "/tmp/somedir";
my $query = CGI->new; # create new CGI object
print $query->header('text/html; charset=utf-8');

my $fd;
my $rc = 0;
my $shell;
my $parameters;

###################################################################
sub getCarac
{
    my $label = $_[0];
    my @caracArray;
    my @carac;
    my $caracLabel;
    my $caracValue;
    my $caracNb = 1;
    do {
	$caracLabel = $query->param($label.'Carac'.$caracNb.'Label');
	$caracValue = $query->param($label.'Carac'.$caracNb.'Value');
	$caracNb++;
	
	if ( $caracLabel ) {
	    my @carac = ($caracLabel, $caracValue);
	    push @caracArray, \@carac;
	}
    } while ( $caracLabel );

    return \@caracArray;
}

sub getParent
{
    my $label = $_[0];
    my @parentArray;
    my @parent;
    my $parentLabel;
    my $parentValue;
    my $parentNb = 1;
    do {
	$parentLabel = $query->param($label.'Parent'.$parentNb.'Label');
	$parentValue = $query->param($label.'Parent'.$parentNb.'Value');
	$parentNb++;
	
	if ( $parentLabel ) {
	    my @parent = ($parentLabel, $parentValue);
	    push @parentArray, \@parent;
	}
    } while ( $parentLabel );

    return \@parentArray;
}

sub getPath
{
    my $label = $_[0];
    my $filename = $_[1];
    my $rc = 0;
    my $fd;

    $filename = $query->param($label.'Source');
    
    if ( !$filename ) {
	print "no filename";
	goto error;
    }

    my ( $name, $path, $extension ) = fileparse ( $filename, '..*' );
    $filename = $name . $extension;
    
    $filename =~ tr/ /_/;
    $filename =~ s/[^$safe_filename_characters]//g;
    
    if ( $filename =~ /^([$safe_filename_characters]+)$/ ) {
	$filename = $1;
    }
    else {
	print "Filename contains invalid characters";
	goto error;
    }

    my $uploadFileHandler = $query->upload($label.'Source');
    if (! open ($fd, ">", "$uploadDir/$filename")) {
	print "$!";
	goto error;
    }
    binmode $fd;
    while (<$uploadFileHandler>) {
	print $fd $_;
    }
    close $fd;

    $_[1] = $filename;
    $rc = 1;
  error:
    if (!$rc) {
	print "getPath fails"
    }
    return $rc;
}

###################################################################

# print header
if (! open ($fd, "<", "../cgiHeader.shtml")) {
    print "$!";
    goto error;
}
while ( <$fd> ) {
    print $_;
}
close $fd;

# get temporary upload dir
$shell = `mktemp -d`;
$shell =~ s/\n//g;
$uploadDir = $shell;
$shell = `chmod o+rx $uploadDir`;

###################################################################

# retrieve categories
my @categoryArray;
my @category;
my $categoryLabel;
my $categoryTop;
my $categoryCarac;
my $categoryParent;
my $categoryNb = 0;
do {
    $categoryNb++;
    $categoryLabel = $query->param('category'.$categoryNb.'Label');
    $categoryTop = $query->param('category'.$categoryNb.'Top');
    $categoryCarac = getCarac('category'.$categoryNb);
    $categoryParent = getParent('category'.$categoryNb);

    if ( $categoryLabel ) {
	my @category = ($categoryLabel, $categoryTop, 
			$categoryCarac, $categoryParent);
	push @categoryArray, \@category;
    }
} while ( $categoryLabel );

# retrieve humans
my @humanArray;
my @human;
my $humanFirstName;
my $humanSecondName;
my $humanRole;
my $humanCarac;
my $humanParent;
my $humanNb = 0;
do {
    $humanNb++;
    $humanFirstName = $query->param('human'.$humanNb.'FirstName');
    $humanSecondName = $query->param('human'.$humanNb.'SecondName');
    $humanRole = $query->param('human'.$humanNb.'Role');
    $humanCarac = getCarac('human'.$humanNb);
    $humanParent = getParent('human'.$humanNb);

    if ( $humanFirstName ) {
	my @human = ($humanFirstName, $humanSecondName, $humanRole, 
		     $humanCarac, $humanParent);
	push @humanArray, \@human;
    }
} while ( $humanFirstName );

# retrieve archives
my @archiveArray;
my @archive;
my $archiveSource;
my $archiveTarget;
my $archiveCarac;
my $archivePath;
my $archiveMd5sum;
my $archiveSize;    
my $archiveNb = 0;

do {
    $archiveNb++;
    $archiveSource = $query->param('archive'.$archiveNb.'Source');
    $archiveTarget = $query->param('archive'.$archiveNb.'Target');
    $archiveCarac = getCarac('archive'.$archiveNb);

    if ( $archiveSource ) {
	if (!getPath('archive'.$archiveNb, $archivePath)) {
	    goto error;
	}
	$archiveMd5sum = `md5sum $uploadDir/$archivePath | cut -d' ' -f1`;
	$archiveMd5sum =~ s/\n//g;
	$archiveSize = `ls -l $uploadDir/$archivePath | awk '{print \$5}'`;
	$archiveSize =~ s/\n//g;

	my @archive = ($archivePath, $archiveTarget, $archiveCarac,
	    $archiveMd5sum, $archiveSize);
	push @archiveArray, \@archive;
    }
} while ( $archiveSource );

# retrieve document
my $document=$query->param('document');
my $documentCarac = getCarac('document');
my $documentParent = getParent('document');

# check if all ok
if (!defined($document)) {
    print "No good. Maybe you try to upload more than ".
	($CGI::POST_MAX /1024 /1024) . "Mo.<br>";
    goto error;
}

###################################################################

# serialize extractNNN.cat
if (! open ($fd, ">", "$uploadDir/extractNNN.cat")) {
    print "$!";
    goto error;
}

# print category
foreach my $ref (@categoryArray) {
    my $top = "";
    my $first = 1;
    
    if ( $ref->[1] ) {$top = "Top ";}
    print $fd $top."Category \"".$ref->[0]."\"";

    # print parents
    foreach my $ref2 (@{$ref->[3]}) {
	if ($first) {
	    print $fd ": ";
	}
	else {
	    print $fd ", ";
	}
	print $fd " \"".$ref2->[0]."\"";
	$first = 0;
    }
    print $fd "\n";

    # print caracs
    foreach my $ref2 (@{$ref->[2]}) {
	print $fd " \"".$ref2->[0]."\" = \"".$ref2->[1]."\"\n";
    }
    print $fd "\n";
}

# print humans
foreach my $ref (@humanArray) {
    my $first = 1;

    print $fd "Human\t\"".$ref->[0]."\" \"".$ref->[1]."\"";
    
    # print parents
    foreach my $ref2 (@{$ref->[4]}) {
	if ($first) {
	    print $fd ": ";
	}
	else {
	    print $fd ", ";
	}
	print $fd " \"".$ref2->[0]."\"";
	$first = 0;
    }
    print $fd "\n";

    # print caracs
    foreach my $ref2 (@{$ref->[3]}) {
	print $fd " \"".$ref2->[0]."\" = \"".$ref2->[1]."\"\n";
    }
    print $fd "\n";
}

# print archives
foreach my $ref (@archiveArray) {

    print $fd "Archive\t".$ref->[3].":".$ref->[4]."\n";
    
    # print caracs
    foreach my $ref2 (@{$ref->[2]}) {
	print $fd " \"".$ref2->[0]."\" = \"".$ref2->[1]."\"\n";
    }
    print $fd "\n";
}

# print document
print $fd "Document \"".$document."\"";

# print parents
my $first = 1;
foreach my $ref (@$documentParent) {
    if ($first) {
	print $fd ": ";
    }
    else {
	print $fd ", ";
    }
    print $fd " \"".$ref->[0]."\"";
    $first = 0;
}
print $fd "\n";

foreach my $ref (@humanArray) {
    print $fd " With\t\"".$ref->[2]."\" = \"".$ref->[0]."\" \"".$ref->[1]."\"\n";
}

# print caracs
foreach my $ref (@$documentCarac) {
    print $fd " \"".$ref->[0]."\" = \"".$ref->[1]."\"\n";
}

foreach my $ref (@archiveArray) {
    print $fd " ".$ref->[3].":".$ref->[4]."\n";
}

close $fd;
###################################################################

# display the catalog meta-data
print "<pre>";
if (! open ($fd, "<", "$uploadDir/extractNNN.cat")) {
    print "$!";
    goto error;
}
while ( <$fd> ) {
    print $_;
}
close $fd;

###################################################################

$parameters = "catalog $uploadDir/extractNNN.cat";
foreach my $ref (@archiveArray) {
    $parameters .= " file $uploadDir/$ref->[0]";
    if ($ref->[1]) {
	$parameters .= " as $ref->[1]";
    }
}

print "<hr>";
#print $parameters;
$shell = `./put.sh $parameters`;
$shell =~ s/\n/<br>/g;
print $shell;

print "</pre><hr>\n";

if ($shell !~ /\[err /) {
    $rc = 1;
}
error:
if (! $rc ) {
	print "Upload fails";
}
else {
    print "Upload success";
}

$shell = `rm -fr $uploadDir`;

# print footer
open ( $fd, "<", "../footer.html" ) or die "$!";
while ( <$fd> ) {
    print $_;
}
close $fd;

