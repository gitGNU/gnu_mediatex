#  * Version: $Id: Makefile.am,v 1.1 2014/10/13 19:38:43 nroche Exp $
#  * Project: MediaTex
#  * Module:  C codes
#  *
#  * Process this file with configure to produce a Makefile.
# 
#  MediaTex is an Electronic Records Management System
#  Copyright (C) 2014  Nicolas Roche
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

include $(top_srcdir)/Make-rules

# Build recursively 
SUBDIRS = . misc memory parser common client server bin

headersdir = $(includedir)/mediatex
dist_headers_HEADERS = ../config.h mediatex.h
dist_check_SCRIPTS = utMediatex.sh

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

