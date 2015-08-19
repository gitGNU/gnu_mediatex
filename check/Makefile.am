#  * Version: $Id: Makefile.am,v 1.6 2015/08/19 01:09:03 nroche Exp $
#  * Project: MediaTex
#  * Module: unitary tests
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

include $(top_srcdir)/Make-rules

AM_CFLAGS = -Wall -I $(top_srcdir)/src

check_PROGRAMS = \
	misc/utgetcgivars \
	misc/utlog \
	misc/utcommand \
	misc/utalloc \
	misc/utsignals \
	misc/utdevice \
	misc/utprogbar \
	misc/utmd5sum \
	misc/utcypher \
	misc/utaddress \
	misc/uttcp \
	misc/utshm \
	misc/utperm \
	misc/utkeys \
	misc/utlocks \
	misc/uthtml \
	memory/utstrdsm \
	memory/utardsm \
	memory/utcvsPrint \
	memory/utconfTree \
	memory/utsupportTree \
	memory/utarchive \
	memory/utcatalogTree \
	memory/utextractTree \
	memory/utserverTree \
	memory/utrecordTree \
	memory/utcacheTree \
	parser/utsupportFile \
	parser/utsupportFile.tab \
	parser/utconfFile \
	parser/utconfFile.tab \
	parser/utserverFile \
	parser/utserverFile.tab \
	parser/utextractFile \
	parser/utextractFile.tab \
	parser/utcatalogFile \
	parser/utcatalogFile.tab \
	parser/utrecordFile \
	parser/utrecordFile.tab \
	parser/utshellQuery \
	parser/utshellQuery.tab \
	common/utregister \
	common/utconnect \
	common/utssh \
	common/utupgrade \
	common/utopenClose \
	common/utextractScore \
	client/utserv \
	client/utconf \
	client/utsupp \
	client/utmotd \
	client/utcommonHtml \
	client/utcatalogHtml \
	client/utextractHtml \
	client/utmisc \
	client/utupload \
	server/utcache \
	server/utdeliver \
	server/utextract \
	server/utcgiSrv \
	server/uthave \
	server/utnotify \
	server/utmessage \
	server/utthreads

TESTS = \
	scripts/utlog.sh \
	scripts/utinclude.sh \
	misc/getcgivars.sh \
	misc/log.sh \
	misc/command.sh \
	misc/alloc.sh \
	misc/signals.sh \
	misc/md5sum.sh \
	misc/cypher.sh \
	misc/address.sh \
	misc/tcp.sh \
	misc/shm.sh \
	misc/perm.sh \
	misc/keys.sh \
	misc/locks.sh \
	misc/html.sh \
	memory/strdsm.sh \
	memory/ardsm.sh \
	memory/cvsPrint.sh \
	memory/confTree.sh \
	memory/supportTree.sh \
	memory/archive.sh \
	memory/catalogTree.sh \
	memory/extractTree.sh \
	memory/serverTree.sh \
	memory/recordTree.sh \
	memory/cacheTree.sh \
	parser/supportFile.sh \
	parser/confFile.sh \
	parser/serverFile.sh \
	parser/extractFile.sh \
	parser/catalogFile.sh \
	parser/recordFile.sh \
	parser/shellQuery.sh \
	common/register.sh \
	common/connect.sh \
	common/ssh.sh \
	common/upgrade.sh \
	common/openClose.sh \
	cgi.sh \
	common/extractScore.sh \
	client/serv.sh \
	client/conf.sh \
	client/supp.sh \
	client/motd.sh \
	client/commonHtml.sh \
	client/catalogHtml.sh \
	client/extractHtml.sh \
	client/misc.sh \
	client/upload.sh \
	server/cache.sh \
	server/deliver.sh \
	server/extract.sh \
	server/cgiSrv.sh \
	server/have.sh \
	server/notify.sh \
	server/message.sh \
	server/threads.sh

dist_check_SCRIPTS = \
	$(TESTS) \
	utmediatex.sh \
	utscripts.sh \
	misc/env.sh

dist_check_DATA = \
	scripts/include.exp \
	scripts/log.exp \
	misc/getcgivars.exp \
	misc/log.exp \
	misc/command.exp \
	misc/alloc.exp \
	misc/signals.exp \
	misc/md5sum.exp \
	misc/cypher.exp \
	misc/address.exp \
	misc/tcp.exp \
	misc/shm.exp \
	misc/perm.exp \
	misc/locks.exp \
	misc/html.exp \
	memory/strdsm.exp \
	memory/ardsm.exp \
	memory/cvsPrint.exp \
	memory/confTree.exp \
	memory/confTree.exp2 \
	memory/hostKey_rsa \
	memory/hostKey_rsa.pub \
	memory/user1Key_rsa \
	memory/user2Key_rsa \
	memory/user3Key_dsa \
	memory/user1Key_rsa.pub \
	memory/user2Key_rsa.pub \
	memory/user3Key_dsa.pub \
	memory/supportTree.exp \
	memory/supportTree.exp2 \
	memory/archive.exp \
	memory/catalogTree.exp \
	memory/catalogTree.exp2 \
	memory/extractTree.exp \
	memory/extractTree.exp2 \
	memory/serverTree.exp \
	memory/serverTree.exp2 \
	memory/recordTree.exp \
	memory/recordTree.exp2 \
	memory/recordTree.aes \
	memory/cacheTree.exp \
	parser/shellQuery.exp \
	common/register.exp \
	common/connect.exp \
	common/ssh.exp \
	common/ssh.exp2 \
	common/ssh.exp3 \
	common/ssh.exp4 \
	common/upgrade.exp \
	common/openClose.exp \
	cgi.exp \
	common/extractScore.exp \
	client/serv.exp \
	client/conf.exp \
	client/supp.exp \
	client/motd.exp \
	client/commonHtml.exp \
	client/catalogHtml.exp \
	client/extractHtml.exp \
	client/misc.exp \
	client/upload.exp \
	server/cache.exp \
	server/deliver.exp \
	server/extract.exp \
	server/cgiSrv.exp \
	server/have.exp \
	server/notify.exp \
	server/message.exp \
	server/threads.exp

# build script (so as to sed values into) and object for tests
check_HEADERS = \
	memory/utFunc.h \
	server/utFunc.h
EXTRA_DIST = \
	utmediatex.src \
	memory/utFunc.c \
	server/utFunc.c 
CLEANFILES = \
	utmediatex.sh

# Dependency to "Makefile" force rebuild on distcheck
utmediatex.sh: utmediatex.src Makefile
	$(do_subst) < $< > $@
	@chmod +x $@

# Note: 2 choices for LDADD
# - ../src/libmediatex.la : should be better
# - ../src/.libs/libmediatex.a : easier to debug and no ps's hangup
AM_LDFLAGS = -pthread -lcrypto -lavl -lm
LDADD = ../src/.libs/libmediatex.a
memory_ldadd = \
	../src/.libs/libmediatex.a \
	memory/utFunc.o
client_ldadd = \
	../src/client/libclient.a \
	../src/.libs/libmediatex.a
server_ldadd = \
	../src/server/libserver.a \
	../src/.libs/libmediatex.a \
	server/utFunc.o

misc_utgetcgivars_SOURCES = misc/utgetcgivars.c
misc_utlog_SOURCES = misc/utlog.c
misc_utcommand_SOURCES = misc/utcommand.c
misc_utalloc_SOURCES = misc/utalloc.c
misc_utsignals_SOURCES = misc/utsignals.c
misc_utdevice_SOURCES = misc/utdevice.c
misc_utprogbar_SOURCES = misc/utprogbar.c
misc_utmd5sum_SOURCES = misc/utmd5sum.c
misc_utcypher_SOURCES = misc/utcypher.c
misc_utaddress_SOURCES = misc/utaddress.c
misc_uttcp_SOURCES = misc/uttcp.c
misc_utshm_SOURCES = misc/utshm.c
misc_utperm_SOURCES = misc/utperm.c
misc_utkeys_SOURCES = misc/utkeys.c
misc_utlocks_SOURCES = misc/utlocks.c
misc_uthtml_SOURCES = misc/uthtml.c

memory_utstrdsm_SOURCES = memory/utstrdsm.c
memory_utardsm_SOURCES = memory/utardsm.c
memory_utcvsPrint_SOURCES = memory/utcvsPrint.c
memory_utconfTree_SOURCES = memory/utconfTree.c
memory_utconfTree_LDADD = $(memory_ldadd)
memory_utsupportTree_SOURCES = memory/utsupportTree.c
memory_utsupportTree_LDADD = $(memory_ldadd)
memory_utarchive_SOURCES = memory/utarchive.c
memory_utarchive_LDADD = $(memory_ldadd)
memory_utcatalogTree_SOURCES = memory/utcatalogTree.c
memory_utcatalogTree_LDADD = $(memory_ldadd)
memory_utextractTree_SOURCES = memory/utextractTree.c
memory_utextractTree_LDADD = $(memory_ldadd)
memory_utserverTree_SOURCES = memory/utserverTree.c
memory_utserverTree_LDADD = $(memory_ldadd)
memory_utrecordTree_SOURCES = memory/utrecordTree.c
memory_utrecordTree_LDADD = $(memory_ldadd)
memory_utcacheTree_SOURCES = memory/utcacheTree.c
memory_utcacheTree_LDADD = $(memory_ldadd)

parser_utsupportFile_SOURCES = parser/utsupportFile.c
parser_utsupportFile_tab_SOURCES = parser/utsupportFile.tab.c
parser_utconfFile_SOURCES = parser/utconfFile.c
parser_utconfFile_tab_SOURCES = parser/utconfFile.tab.c
parser_utserverFile_SOURCES = parser/utserverFile.c
parser_utserverFile_tab_SOURCES = parser/utserverFile.tab.c
parser_utextractFile_SOURCES = parser/utextractFile.c
parser_utextractFile_tab_SOURCES = parser/utextractFile.tab.c
parser_utcatalogFile_SOURCES = parser/utcatalogFile.c
parser_utcatalogFile_tab_SOURCES = parser/utcatalogFile.tab.c
parser_utrecordFile_SOURCES = parser/utrecordFile.c
parser_utrecordFile_tab_SOURCES = parser/utrecordFile.tab.c

common_utregister_SOURCES = common/utregister.c
common_utconnect_SOURCES = common/utconnect.c
common_utssh_SOURCES = common/utssh.c
common_utupgrade_SOURCES = common/utupgrade.c
common_utopenClose_SOURCES = common/utopenClose.c
common_utextractScore_SOURCES = common/utextractScore.c

client_utserv_SOURCES = client/utserv.c
client_utserv_LDADD = $(client_ldadd)
client_utconf_SOURCES = client/utconf.c
client_utconf_LDADD = $(client_ldadd)
client_utsupp_SOURCES = client/utsupp.c
client_utsupp_LDADD = $(client_ldadd)
client_utmotd_SOURCES = client/utmotd.c
client_utmotd_LDADD = $(client_ldadd)
client_utcommonHtml_SOURCES = client/utcommonHtml.c
client_utcommonHtml_LDADD = $(client_ldadd)
client_utcatalogHtml_SOURCES = client/utcatalogHtml.c
client_utcatalogHtml_LDADD = $(client_ldadd)
client_utextractHtml_SOURCES = client/utextractHtml.c
client_utextractHtml_LDADD = $(client_ldadd)
client_utmisc_SOURCES = client/utmisc.c
client_utmisc_LDADD = $(client_ldadd)
client_utupload_SOURCES = client/utupload.c
client_utupload_LDADD = $(client_ldadd)
parser_utshellQuery_SOURCES = parser/utshellQuery.c
parser_utshellQuery_LDADD = $(client_ldadd)
parser_utshellQuery_tab_SOURCES = parser/utshellQuery.tab.c
parser_utshellQuery_tab_LDADD = $(client_ldadd)

server_utcache_SOURCES = server/utcache.c
server_utcache_LDADD = $(server_ldadd)
server_utdeliver_SOURCES = server/utdeliver.c
server_utdeliver_LDADD = $(server_ldadd)
server_utextract_SOURCES = server/utextract.c
server_utextract_LDADD = $(server_ldadd)
server_utcgiSrv_SOURCES = server/utcgiSrv.c
server_utcgiSrv_LDADD = $(server_ldadd)
server_uthave_SOURCES = server/uthave.c
server_uthave_LDADD = $(server_ldadd)
server_utnotify_SOURCES = server/utnotify.c
server_utnotify_LDADD = $(server_ldadd)
server_utmessage_SOURCES = server/utmessage.c
server_utmessage_LDADD = $(server_ldadd)
server_utthreads_SOURCES = server/utthreads.c
server_utthreads_LDADD = $(server_ldadd)

check-local: \
	utmediatex.sh

clean-local:
	@rm -f 	scripts/*.out \
		misc/*.out* \
		misc/*.txt \
		misc/*.html \
		misc/key \
		misc/key.pub \
		memory/*.out \
		parser/*.out \
		parser/*.aes \
		parser/*.txt \
		common/*.out \
		client/*.out* \
		client/*.pub \
		client/upload.cat \
		client/upload.ext \
		server/*.out \
		*.out
	@rm -fr tmp

#localedir = $(datadir)/locale
#DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

