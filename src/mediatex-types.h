/*=======================================================================
 * Version: $Id: mediatex-types.h,v 1.2 2015/07/02 17:59:10 nroche Exp $
 * Project: MediaTex
 * Module : headers
 *
 * This file is included first by all others

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

#ifndef MDTX_TYPES_H
#define MDTX_TYPES_H 1

// This is a must be in order to handle large files
// Note this have to be done before including the relevant system headers
#define _FILE_OFFSET_BITS 64

#define _GNU_SOURCE // setresuid

// recurrent includes files
#include <errno.h>     // errno
#include <stdio.h>     // printf
#include <stdlib.h>    // exit, getenv
#include <unistd.h>    // getopt
#include <string.h>    // memset, strlen
#include <sys/types.h> // open, getpwuid_r, ftok, ...
#include <sys/stat.h>  // open
#include <fcntl.h>     // open  
#include <pthread.h>   // pthread_mutex_init, pthread_sigmask
#include <avl.h>       // 

// gettext
#include <libintl.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

// AVL tree
typedef struct avl_tree_t AVLTree;
typedef struct avl_node_t AVLNode;

// Alloc counters
typedef struct Alloc Alloc;

// Global variable
typedef struct MdtxEnv MdtxEnv;

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

// needed by parsers so as to include scanners headers
typedef void* yyscan_t;

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
#define MAX_SIZE_STRING 511 // hope is enought

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

// log defaults
#define MISC_LOG_FILE 99 // log to stdout
#define MISC_LOG_LINES 1 // log with line numbers

// maximum allocated size before we try to disease memory
// note: we deals with mediatex memory so not same as VMS, RSS
#define DEFAULT_MALLOC_LIMIT 32*MEGA

// default average upload rate in b.s-1 (still not used)
#define CONF_UPLOAD_RATE 500*KILO

// device
#define MISC_CHECKSUMS_MAX_NBLINK 10
#define MISC_CHECKSUMS_MTAB       "/etc/mtab"
#define MISC_CHECKSUMS_PROGBARSIZE 128

// threads
#define MAX_TASK_SOCKET_THREAD 3
#define MAX_TASK_SIGNAL_THREAD 3

// ipcs
#define MISC_SHM_PROJECT_ID 6561
#define COMMON_OPEN_CLOSE_PROJECT_ID 6562

struct ScoreParam {
  float  maxScore; // maximum score (usually 10)
  float  badScore; // maximum score for outdated supports (usually 1)
  float  powSupp;  // power that reduce score from age of supports (2)
  float  factSupp; // factor that reduce score of out-dated supports (2)
  time_t suppTTL;  // support time to live (around 5 years)
};

#define DEFAULT_SCORE_PARAM {10, 1, 2, 2, DEFAULT_TTL_SUPP}

#define printCacheSize(fd, fmt, lbl, size) {		\
    fprintf(fd, fmt, lbl);				\
    do {						\
      if ((size) % GIGA == 0) {				\
	fprintf(fd, " %llu Go\n",			\
		(unsigned long long int) (size) >> 30);	\
	break;						\
      }							\
      if ((size) % MEGA == 0) {				\
	fprintf(fd, " %llu Mo\n",			\
		(unsigned long long int) (size) >> 20);	\
	break;						\
      }							\
      if ((size) % KILO == 0) {				\
	fprintf(fd, " %llu Ko\n",			\
		(unsigned long long int) (size) >> 10);	\
	break;						\
      }							\
      fprintf(fd, " %llu o\n",				\
	      (unsigned long long int) size);		\
    } while (0);					\
  }

#define sprintSize(buf, size) {				\
    do {						\
      if ((size) >= GIGA) {				\
	sprintf(buf, "%llu Go",				\
		(unsigned long long int) (size) >> 30);	\
	break;						\
      }							\
      if ((size) >= MEGA) {				\
	sprintf(buf, "%llu Mo",				\
		(unsigned long long int) (size) >> 20);	\
	break;						\
      }							\
      if ((size) >= KILO) {				\
	sprintf(buf, "%llu Ko",				\
		(unsigned long long int) (size) >> 10);	\
	break;						\
      }							\
      sprintf(buf, "%llu o",				\
	      (unsigned long long int) size);		\
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

#endif /* MDTX_TYPES_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
