/*=======================================================================
 * Version: $Id: mediatex.h,v 1.3 2015/06/03 14:03:27 nroche Exp $
 * Project: MediaTex
 * Module : mediatex pre-configuration output file
 *
 * This file is included by all the C modules

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 Nicolas Roche

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

#ifndef MDTX_H
#define MDTX_H 1

// This is a must be in order to handle large files
// Note this have to be done before including the relevant system headers
#define _FILE_OFFSET_BITS 64

#define _GNU_SOURCE // setresuid

#include "mediatex-config.h" // from ./configure

#include <errno.h>  // errno
#include <stdio.h>  // printf
#include <stdlib.h> // exit
#include <unistd.h> // getopt

// gettext
#include <libintl.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

// AVL tree
typedef struct avl_tree_t AVLTree;
typedef struct avl_node_t AVLNode;

// memory/*.h types (because header include them each others)
typedef struct Configuration Configuration;
typedef struct ScoreParam ScoreParam;
typedef struct Support Support;

typedef struct Collection Collection;
typedef struct CacheTree CacheTree;
typedef struct Archive Archive;

typedef struct ServerTree ServerTree;
typedef struct Server Server;
typedef struct Image Image;

typedef struct ExtractTree ExtractTree;
typedef struct Container Container;
typedef struct FromAsso FromAsso;

typedef struct CatalogTree CatalogTree;
typedef struct Category Category;
typedef struct Document Document;
typedef struct Human Human;
typedef struct Carac Carac;
typedef struct Role Role;
typedef struct AssoCarac AssoCarac;
typedef struct AssoRole AssoRole;

typedef struct RecordTree RecordTree;
typedef struct Record Record;

// we should remove the below definition by providing a header file for parsers
// (hiding the XX_parse functions)
typedef void* yyscan_t;

// Defined in config.h:
// Installers are expected to override these default values when calling 
// make (e.g., make prefix=/usr install) or 
// configure (e.g., configure --prefix=/usr)

// Default values are :
// CONF_PREFIX = ""
// CONF_SYSCONFDIR = CONF_PREFIX "/etc"
// CONF_LOCALSTATEDIR = CONF_PREFIX "/var"
// CONF_DATAROOTDIR = CONF_EXEC_PRDIR "/share"
// CONF_EXEC_PREFIX = /usr
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
     --libexecdir=/usr/lib/mediatex \
     --disable-dependency-tracking &&
*/

// The above values are passes as CONF_MDTX_xxx environment variables 
// to the scripts.

// absolute paths
#define CONF_ETCDIR   CONF_SYSCONFDIR CONF_MEDIATEXDIR
#define CONF_DATADIR  CONF_DATAROOTDIR CONF_MEDIATEXDIR
#define CONF_STATEDIR CONF_LOCALSTATEDIR "/lib" CONF_MEDIATEXDIR
#define CONF_CACHEDIR CONF_LOCALSTATEDIR "/cache" CONF_MEDIATEXDIR
#define CONF_PIDDIR   CONF_LOCALSTATEDIR "/run" CONF_MEDIATEXDIR
#define CONF_SCRIPTS  CONF_DATADIR  "/scripts"
#define CONF_EXAMPLES CONF_DATADIR  "/examples"
#define CONF_HOSTSSH  CONF_SYSCONFDIR "/ssh"

// parts for relative paths
#define CONF_MD5SUMS  "/md5sums"
#define CONF_CACHES   "/cache"
#define CONF_EXTRACT  "/tmp"
#define CONF_HOME     "/home"
#define CONF_CVSCLT   "/cvs"
#define CONF_SSHDIR   "/.ssh"
#define CONF_HTMLDIR  "/public_html"
#define CONF_CONFFILE ".conf"
#define CONF_PIDFILE  "d.pid"
#define CONF_TMPDIR   "/tmp/ut-" // only used by C code for "make check"

// relative path for files
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

// define the fundamental boolean constants
#define TRUE 1
#define FALSE 0

// no ipv4 for localhost to optimize sort
#define LOCALHOST 0 

// maximum size needed for char conversion (without the leading \0)
#define MAX_SIZE_HASH 32 // MD5_DIGEST_LENGTH bytes printed using %x
#define MAX_SIZE_SIZE 19 // off_t printed using %lli
#define MAX_SIZE_COLL 20 
#define MAX_SIZE_STAT 10
#define MAX_SIZE_NAME 64
#define MAX_SIZE_AES  16
#define MAX_SIZE_HOST 255 // http://en.wikipedia.org/wiki/Hostname
#define MAX_SIZE_CMD 4    // 3 + \0
#define MAX_SIZE_STRING 511 // use by parser

// HTML
#define MAX_FILES_PER_DIR 1000
#define MAX_NUM_DIR_SIZE 3 // related to MAX_FILES_PER_DIR (10^X =)
#define MAX_INDEX_PER_PAGE 25

// default values
#define KILO ((unsigned long long int)(1<<10))
#define MEGA ((unsigned long long int)(1<<20))
#define GIGA ((unsigned long long int)(1<<30))
#define MINUTE ((long int)60)
#define HOUR   ((long int)60*60)
#define DAY    ((long int)60*60*24)
#define WEEK   ((long int)60*60*24*7)
#define MONTH  ((long int)60*60*24*30)
#define YEAR   ((long int)60*60*24*365)
#define DEFAULT_MDTXUSER "mdtx"
#define DEFAULT_HOST "localhost"
#define DEFAULT_CACHE_SIZE 100*MEGA // cache size
#define DEFAULT_TTL_CACHE 15*DAY    // local-supply TTL on cache
#define DEFAULT_TTL_QUERY 7*DAY     // final-query  TTL 
#define DEFAULT_TTL_CHECK 6*MONTH   // support check TTL
#define DEFAULT_TTL_SUPP  5*YEAR    // support TTL
#define DEFAULT_MAX_SCORE 20 // cf above
#define DEFAULT_BAD_SCORE 1  // cf above
#define DEFAULT_POW_SUPP  2  // cf above
#define DEFAULT_POW_IMAGE 2  // cf above
#define DEFAULT_FACT_SUPP 2  // cf above
#define DEFAULT_MIN_GEO   2  // number of distantes copies expected

// maximum allocated size before we try to disease memory
// note: we deals with mediatex memory so not same as VMS, RSS
#define DEFAULT_MALLOC_LIMIT 32*MEGA

// default average upload rate in b.s-1 (still not used)
#define CONF_UPLOAD_RATE 500*KILO

// device
#define MISC_CHECKSUMS_MAX_NBLINK 10
#define MISC_CHECKSUMS_MTAB       "/etc/mtab"

// threads
#define MAX_TASK_SOCKET_THREAD 3
#define MAX_TASK_SIGNAL_THREAD 3

// log with or without line numbers
#define MISC_LOG_LINES 1

// ipcs
#define MISC_SHM_PROJECT_ID 6561
#define COMMON_OPEN_CLOSE_PROJECT_ID 6562

// hight level type computed from type, host and path
typedef enum {
  UNDEF_RECORD = 0,
  FINALE_SUPPLY = 1, 
  LOCALE_SUPPLY = 2,
  REMOTE_SUPPLY = 4,
  MALLOC_SUPPLY = 8,   // use by merger to reserve space
  FINALE_DEMAND = 16, 
  LOCALE_DEMAND = 32,
  REMOTE_DEMAND = 64,
  TOKEEP_DEMAND = 128, // NO MORE USED !
  ALL_SUPPLY    = 1|2|4,
  ALL_DEMAND    = 16|32|64|128,
  ALL_RECORD    = 1|2|4|16|32|64|128,
} RecordType;

struct ScoreParam {
  float  maxScore; // maximum score (usually 10)
  float  badScore; // maximum score for outdated supports (usually 1)
  float  powSupp;  // power that reduce score from age of supports (2)
  float  factSupp; // factor that reduce score of out-dated supports (2)
  time_t suppTTL;  // support time to live (around 5 years)
};

#define MISC_CHECKSUMS_PROGBARSIZE 128

typedef struct ProgBar {
  char bar[MISC_CHECKSUMS_PROGBARSIZE];
  char spaces[MISC_CHECKSUMS_PROGBARSIZE];

  unsigned int progress_last_percent;
  unsigned int progress_last_time;
  unsigned int progress_pos;
} ProgBar;

typedef struct MdtxProgBar {
  ProgBar bar;
  char* label;
  off_t max;
  off_t cur;

} MdtxProgBar;

// above struct related to command.h
// do not free them (man 3 getenv)
typedef struct MdtxEnv {
  // logging
  char *logFile;
  char *logFacility;
  char *logSeverity;

  // allocating
  long long int allocLimit;
  int (*allocDiseaseCallBack)(long);

  // configuration
  char* confLabel;
  int dryRun;        // output to stdout (set by unit tests by default)
  int noRegression;  // fixed dates and no sig to server (unit tests only)
  int noCollCvs;     // do not call cvs loading/saving collections
  int cvsprintMax;   // maximum size for files handle by CVS

  // debug options
  int debugAlloc;
  int debugScript;
  int debugMemory;
  int debugLexer;
  int debugParser;
  int debugCommon;

  // global data structures:
  Configuration* confTree;
  int running;
  char commandLine[512];
  MdtxProgBar progBar;
} MdtxEnv;

#define DEFAULT_SCORE_PARAM {10, 1, 2, 2, DEFAULT_TTL_SUPP}

#if MISC_LOG_LINES
#define LOG_TEMPLATE "[%s %s:%i] "
#define MISC_LOG_OFFSET 11
#else
#define LOG_TEMPLATE "[%s %s] "
#define MISC_LOG_OFFSET 8
#endif

// Global variable definition having its default values for unit tests
#define GLOBAL_MDTX_STRUCT_UT						\
  {									\
    /* logging */							\
    0, "file", "info",							\
      /* allocating */							\
      0, (int (*)(long))0,						\
      /* configuration */						\
      DEFAULT_MDTXUSER "1", TRUE, TRUE, TRUE, 10*MEGA,			\
      /* debug */							\
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,				\
      /* global data structure */					\
      0, TRUE								\
      }

// Global variable definition having its default values for binaries
// file -> local2 for mdtxd (set by /etc/init.d/mdtxd)
#define GLOBAL_MDTX_STRUCT_BIN						\
  {									\
    /* logging */							\
    0, "file", "notice",						\
      /* allocating */							\
      128, (int (*)(long))0,						\
      /* configration */						\
      DEFAULT_MDTXUSER, FALSE, FALSE, TRUE, 10*MEGA,			\
      /* debug and tests */						\
      FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,				\
      /* global data structure */					\
      0, TRUE								\
      }

#define GLOBAL_STRUCT_DEF MdtxEnv env =	GLOBAL_MDTX_STRUCT_UT
#define GLOBAL_STRUCT_DEF_BIN MdtxEnv env = GLOBAL_MDTX_STRUCT_BIN

// void printCacheSize(FILE* fd, char *fmt, char *lbl, off_t size);
#define printCacheSize(fd, fmt, lbl, size) {		\
    fprintf(fd, fmt, lbl);				\
    do {						\
      if ((size) % GIGA == 0) {				\
	fprintf(fd, " %llu Go\n", (size) >> 30);	\
	break;						\
      }							\
      if ((size) % MEGA == 0) {				\
	fprintf(fd, " %llu Mo\n", (size) >> 20);	\
	break;						\
      }							\
      if ((size) % KILO == 0) {				\
	fprintf(fd, " %llu Ko\n", (size) >> 10);	\
	break;						\
      }							\
      fprintf(fd, " %llu o\n", size);			\
    } while (0);					\
  }

#define sprintSize(buf, size) {				\
    do {						\
      if ((size) >= GIGA) {				\
	sprintf(buf, "%llu Go", (size) >> 30);		\
	break;						\
      }							\
      if ((size) >= MEGA) {				\
	sprintf(buf, "%llu Mo", (size) >> 20);		\
	break;						\
      }							\
      if ((size) >= KILO) {				\
	sprintf(buf, "%llu Ko", (size) >> 10);		\
	break;						\
      }							\
      sprintf(buf, "%llu o", size);			\
    } while (0);					\
  }

// void printLapsTime(FILE* fd, char *fmt, char *lbl, time_t ttl) {
#define printLapsTime(fd, fmt, lbl, ttl) {		\
    fprintf(fd, fmt, lbl);				\
    do {						\
      if (ttl % YEAR == 0) {				\
	fprintf(fd, " %li Year\n", ttl / YEAR);		\
	break;						\
      }							\
      if (ttl % MONTH == 0) {				\
	fprintf(fd, " %li Month\n", ttl / MONTH);	\
	break;						\
      }							\
      if (ttl % WEEK == 0) {				\
	fprintf(fd, " %li Week\n", ttl / WEEK);		\
	break;						\
      }							\
      if (ttl % DAY == 0) {				\
	fprintf(fd, " %li Day\n", ttl / DAY);		\
	break;						\
      }							\
      if (ttl % HOUR == 0) {				\
	fprintf(fd, " %li hour\n", ttl / HOUR);		\
	break;						\
      }							\
      if (ttl % MINUTE == 0) {				\
	fprintf(fd, " %li minute\n", ttl / MINUTE);	\
	break;						\
      }							\
      fprintf(fd, " %li second\n", (long int)ttl);	\
    } while (0);					\
  }

// Defensive programming macros

#define checkLabel(label) {					\
    if (label == (char*)0 || *label == (char)0)	{		\
      logEmit(LOG_ERR, "%s", "please provide a label");		\
      goto error;						\
    }								\
  }

#define checkCollection(coll) {					\
    if (coll == (Collection*)0) {				\
      logEmit(LOG_ERR, "%s", "please provide a collection");	\
      goto error;						\
    }								\
  }

#define checkSupport(coll) {					\
    if (coll == (Support*)0) {					\
      logEmit(LOG_ERR, "%s", "please provide a support");	\
      goto error;						\
    }								\
  }

#define checkServer(server) {					\
    if (server == (Server*)0) {					\
      logEmit(LOG_ERR, "%s", "please provide a server");	\
      goto error;						\
    }								\
  }

#define checkImage(image) {					\
    if (image == (Image*)0) {					\
      logEmit(LOG_ERR, "%s", "please provide an image");	\
      goto error;						\
    }								\
  }

#define checkArchive(archive) {					\
    if (archive == (Archive*)0) {				\
      logEmit(LOG_ERR, "%s", "please provide an archive");	\
      goto error;						\
    }								\
  }

#define checkContainer(container) {				\
    if(container == (Container*)0) {				\
      logEmit(LOG_ERR, "%s", "please provide a container");	\
      goto error;						\
    }								\
  }

#define checkRecord(record) {					\
    if (record == (Record*)0) {					\
      logEmit(LOG_ERR, "%s", "please provide a record");	\
      goto error;						\
    }								\
  }

#define checkRecordTree(recordTree) {				\
    if (recordTree == (RecordTree*)0) {				\
      logEmit(LOG_ERR, "%s", "please provide a record tree");	\
      goto error;						\
    }								\
    if (recordTree->collection == (Collection*)0) {		\
      logEmit(LOG_ERR, "%s", "recordTree without collection");	\
      goto error;						\
    }								\
  }

// Global variable
extern MdtxEnv env;

#endif /* MDTX_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
