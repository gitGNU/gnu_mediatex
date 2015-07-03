#  * Version: $Id: Makefile.am,v 1.5 2015/07/03 11:53:59 nroche Exp $
#  * Project: MediaTex
#  * Module:  figures for the documentation
#  *
#  * Process this file with configure to produce a Makefile.
# 
#  MediaTex is an Electronic Records Management System
#  Copyright (C) 2014 2015 Nicolas Roche
# 
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

INFO_DIR = $(infodir)@mdtx_mediatexdir@-figures
HTML_DIR = $(htmldir)@mdtx_mediatexdir@.html@mdtx_mediatexdir@-figures

noinst_HEADERS = $(srcdir)/*.fig logo.xpm logo.txt

figuresdir = $(INFO_DIR)
dist_figures_DATA = $(PNG_FILES)

dvi-local: @EPS_FILES@ logo.eps
ps-local: @EPS_FILES@ logo.eps
html-local: @PNG_FILES@ logo.png
pdf-local: @PNG_FILES@ logo.png

# foo is just use here to disable an autoconf warning:
# `%'-style pattern rules are a GNU make extension
%.eps %.foo: %.fig
	fig2dev -L eps -F -m 0.65 $< $*.eps

%.png %.foo: %.fig
	fig2dev -L png -F -m 0.65 $< $*.png

logo.eps: logo.xpm
	convert -resize 50x50 $< logo.eps

logo.png: logo.xpm
	convert -resize 50x50 $< logo.png

install-html-local:
	install -m 755 -d $(HTML_DIR)
	install -m 755 *.png $(HTML_DIR)

uninstall-local:
	rm -fr $(INFO_DIR) $(HTML_DIR)

distclean-local:
	rm -f *.{png,eps}
