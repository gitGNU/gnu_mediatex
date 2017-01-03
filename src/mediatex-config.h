/*=======================================================================
 * Project: MediaTex
 * Module : headers
 *
 * This file include the config.h file generated by ./configure.
 * It should only be included by the mediatex binaries, because library 
 * users should provide the same standard definitions by using  automake
 * too for instance.
 * 

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
=======================================================================*/

#ifndef MDTX_CONFIG_H
#define MDTX_CONFIG_H 1

#include "mediatex.h"   // general API
#include "misc/alloc.h" // mediatex use its own allocator
#include "config.h"     // generated by ./configure

// Below values ar defined by config.h.
// Installers are expected to override these default values when calling 
// make (make prefix=/usr install) or configure (configure --prefix=/usr).
// However, mediatex is using too theses values.
// So, DO NOT use make to overide theses values but USE ./configure.

// Default values are :
// CONF_PREFIX = ""
// CONF_EXEC_PREFIX = /usr
// CONF_SYSCONFDIR = CONF_PREFIX "/etc"
// CONF_LOCALSTATEDIR = CONF_PREFIX "/var"
// CONF_BINDIR = CONF_EXEC_PREFIX "/bin"
// CONF_DATAROOTDIR = CONF_EXEC_PREFIX "/share"
// CONF_MEDIATEXDIR "/mediatex"

// Debian package use the following values :
// cf /usr/share/perl5/Debian/Debhelper/Buildsystem/autoconf.pm
/* $ ./configure \
     --prefix=/usr \
     --includedir=/usr/include \
     --mandir=/usr/share/man \
     --infodir=/usr/share/info \
     --sysconfdir=/etc \
     --localstatedir=/var \
     --libexecdir=/usr/lib/mediatex
*/

// Scripts are greping from and here too the above and below definitions.

// absolute paths
#define CONF_ETCDIR   CONF_SYSCONFDIR CONF_MEDIATEXDIR
#define CONF_DATADIR  CONF_DATAROOTDIR CONF_MEDIATEXDIR
#define CONF_STATEDIR CONF_LOCALSTATEDIR "/lib" CONF_MEDIATEXDIR
#define CONF_CACHEDIR CONF_LOCALSTATEDIR "/cache" CONF_MEDIATEXDIR
#define CONF_PIDDIR   CONF_LOCALSTATEDIR "/run" CONF_MEDIATEXDIR
#define CONF_SCRIPTS  CONF_DATADIR  "/scripts"
#define CONF_MISC     CONF_DATADIR  "/misc"
#define CONF_HOSTSSH  CONF_SYSCONFDIR "/ssh"

// relative paths
#define CONF_JAIL     "/jail"
#define CONF_MD5SUMS  "/md5sums"
#define CONF_CACHES   "/cache"
#define CONF_EXTRACT  "/tmp"
#define CONF_HOME     "/home"
#define CONF_GITCLT   "/git"
#define CONF_SSHDIR   "/.ssh"
#define CONF_HTMLDIR  "/public_html"
#define CONF_CONFFILE ".conf"
#define CONF_PIDFILE  "d.pid"
#define CONF_SUPPD    ":supports/"
#define CONF_AUDIT    "audit_"

// basenames
#define CONF_SUPPFILE  "/supports.txt"
#define CONF_SERVFILE  "/servers"
#define CONF_CATHFILE  "/catalog"
#define CONF_EXTRFILE  "/extract"
#define CONF_RSAUSERKEY "/id_rsa.pub"
#define CONF_DSAUSERKEY "/id_dsa.pub"
#define CONF_RSAHOSTKEY "/ssh_host_rsa_key.pub"
#define CONF_DSAHOSTKEY "/ssh_host_dsa_key.pub"
#define CONF_SSHKNOWN  "/known_hosts"
#define CONF_SSHAUTH   "/authorized_keys"
#define CONF_SSHCONF   "/config"
#define CONF_COLLKEY   "/aesKey.txt"
#define TESTING_PORT  6560
#define CONF_PORT     6561 // (as default)
#define SSH_PORT      22
#define HTTP_PORT     80
#define HTTPS_PORT    443

// base acl
#define BASE_ACL "u::rwx g::rwx o::--- m:rwx"

// take care to update misc/perm.c too
enum {
  ETC_M = 0,
  VAR_RUN_M,
  VAR_LIB_M,
  VAR_LIB_M_MDTX,
  VAR_LIB_M_MDTX_MDTX,
  VAR_LIB_M_MDTX_COLL,
  VAR_CACHE_M,
  VAR_CACHE_M_MDTX,
  VAR_CACHE_M_MDTX_CACHE,
  VAR_CACHE_M_MDTX_CACHE_M,
  VAR_CACHE_M_MDTX_CACHE_COLL,
  VAR_CACHE_M_MDTX_HTML,
  VAR_CACHE_M_MDTX_GIT,
  VAR_CACHE_M_MDTX_GIT_MDTX,
  VAR_CACHE_M_MDTX_GIT_COLL,
  VAR_CACHE_M_MDTX_TMP,
  VAR_CACHE_M_MDTX_TMP_COLL,
  VAR_CACHE_M_MDTX_HOME,
  VAR_CACHE_M_MDTX_HOME_COLL,
  VAR_CACHE_M_MDTX_HOME_COLL_SSH,
  VAR_CACHE_M_MDTX_HOME_COLL_HTML,
  VAR_CACHE_M_MDTX_HOME_COLL_VIEWVC,
  VAR_CACHE_M_MDTX_MD5SUMS,
  VAR_CACHE_M_MDTX_JAIL
};

// ACL: "u:%s:rwx g:%s:rwx [u:%s:r-x]"
// rwx for mdtx user and group ; and eventually permissions for mdtx-coll
#define _ETC_M                             {"root", "root", 0755, "NO ACL"}
#define _VAR_RUN_M                         {"root", "root", 0777, "NO ACL"}
#define _VAR_LIB_M                         {"root", "root", 0755, "NO ACL"}
#define _VAR_LIB_M_MDTX                    {"root", "root", 0755, "NO ACL"}
#define _VAR_LIB_M_MDTX_MDTX               {"root", "root", 0750, "u:%s:rwx g:%s:rwx u:www-data:r-x"}
#define _VAR_LIB_M_MDTX_COLL               {"root", "root", 0750, "u:%s:rwx g:%s:rwx u:%s:rwx u:www-data:r-x"}
#define _VAR_CACHE_M                       {"root", "root", 0755, "NO ACL"}
#define _VAR_CACHE_M_MDTX                  {"root", "root", 0755, "NO ACL"}
#define _VAR_CACHE_M_MDTX_CACHE            {"root", "root", 0755, "NO ACL"}
#define _VAR_CACHE_M_MDTX_CACHE_M          {"root", "root", 0755, "NO ACL"}
#define _VAR_CACHE_M_MDTX_CACHE_COLL       {"root", "root", 0750, "u:%s:rwx g:%s:rwx u:%s:r-x u:www-data:r-x"}
#define _VAR_CACHE_M_MDTX_HTML             {"root", "root", 0750, "u:%s:rwx g:%s:rwx u:www-data:r-x"}
#define _VAR_CACHE_M_MDTX_GIT              {"root", "root", 0755, "NO ACL"}
#define _VAR_CACHE_M_MDTX_GIT_MDTX         {"root", "root", 0750, "u:%s:rwx g:%s:rwx u:www-data:r-x"}
#define _VAR_CACHE_M_MDTX_GIT_COLL         {"root", "root", 0750, "u:%s:rwx g:%s:rwx u:%s:rwx u:www-data:r-x"}
#define _VAR_CACHE_M_MDTX_TMP              {"root", "root", 0755, "NO ACL"}
#define _VAR_CACHE_M_MDTX_TMP_COLL         {"root", "root", 0750, "u:%s:rwx g:%s:rwx u:%s:rwx"}
#define _VAR_CACHE_M_MDTX_HOME             {"root", "root", 0755, "NO ACL"}
#define _VAR_CACHE_M_MDTX_HOME_COLL        {"root", "root", 0755, "NO ACL"}
#define _VAR_CACHE_M_MDTX_HOME_COLL_SSH    {"%s",   "%s",   0700, "NO ACL"}
#define _VAR_CACHE_M_MDTX_HOME_COLL_HTML   {"root", "root", 0750, "u:%s:rwx g:%s:rwx u:%s:r-x u:www-data:r-x"}
#define _VAR_CACHE_M_MDTX_HOME_COLL_VIEWVC {"root", "root", 0750, "u:%s:rwx g:%s:rwx u:%s:r-x u:www-data:r-x"}
#define _VAR_CACHE_M_MDTX_MD5SUMS          {"root", "root", 0750, "u:%s:rwx g:%s:rwx"}
#define _VAR_CACHE_M_MDTX_JAIL             {"root", "root", 0755, "NO ACL"}

#endif /* MDTX_CONFIG_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
