/*=======================================================================
 * Version: $Id: mediatex.h,v 1.9 2015/10/20 19:41:50 nroche Exp $
 * Project: MediaTex
 * Module : headers
 *
 * This file include all mediatex headers.
 * It should be included by final user.

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 Nicolas Roche

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

#include "mediatex-types.h"

// We not include alloc.h here so as to not disturb user with this
// library feature.

#include "misc/log.h"
#include "misc/command.h"
#include "misc/setuid.h"
#include "misc/signals.h"
#include "misc/perm.h"
#include "misc/shm.h"
#include "misc/locks.h"
#include "misc/progbar.h"
#include "misc/device.h"
#include "misc/md5sum.h"
#include "misc/cypher.h"
#include "misc/keys.h"
#include "misc/address.h"
#include "misc/tcp.h"
#include "misc/getcgivars.h"
#include "misc/html.h"
#include "memory/strdsm.h"
#include "memory/ardsm.h"
#include "memory/cvsPrint.h"
#include "memory/confTree.h"
#include "memory/archive.h"
#include "memory/extractTree.h"
#include "memory/catalogTree.h"
#include "memory/serverTree.h"
#include "memory/cacheTree.h"
#include "memory/recordTree.h"
#include "memory/supportTree.h"
#include "memory/catalogTree.h"
#include "common/register.h"
#include "common/connect.h"
#include "common/ssh.h"
#include "common/upgrade.h"
#include "common/openClose.h"
#include "common/extractScore.h"

// alloc (alloc.h is not included by library user)
extern void memoryStatus(int priority, char* file, int line);
extern void exitMalloc();

// parsers
extern int parseSupports(const char* path);
extern int parseConfiguration(const char* path);
extern int parseServerFile(Collection* coll, const char* path);
extern int parseExtractFile(Collection* coll, const char* path);
extern int parseCatalogFile(Collection* coll, const char* path);
extern RecordTree* parseRecords(int fd);

// above struct related to command.h
// do not free them (man 3 getenv)
typedef struct MdtxEnv {

  // loggin
  int logFacility;
  char *logFile;
  int logSeverity[LOG_MAX_MODULE];
  LogHandler* logHandler;

  // allocating
  float allocLimit;
  int (*allocDiseaseCallBack)(long);
  Alloc* alloc;

  // configuration
  char* confLabel;
  int dryRun;        // output to stdout (set by unit tests by default)
  int noRegression;  // fixed dates and no sig to server (unit tests only)
  int noCollCvs;     // do not call cvs loading/saving collections
  int cvsprintMax;   // maximum size for files handle by CVS

  // debug options
  int debugLexer;

  // global data structures:
  Configuration* confTree;
  int running;
  char commandLine[512];
  MdtxProgBar progBar;

} MdtxEnv;

// Global variable mediatex need you provides (using GLOBAL_STRUCT_DEF)
// Global variable
extern MdtxEnv env;
extern MdtxEnv envUnitTest; // only used for copy 

// Global variable definition having its default values for unit tests
#define GLOBAL_MDTX_ENV_UT						\
  {									\
    /* logging */							\
    99,	0,								\
      {LOG_INFO, LOG_INFO, LOG_INFO, LOG_INFO,				\
	  LOG_INFO, LOG_INFO, LOG_INFO},				\
      0,								\
	/* allocating */						\
	0, (int (*)(long))0, 0,						\
	/* configuration */						\
	DEFAULT_MDTXUSER "1", TRUE, TRUE, TRUE, 500*KILO,		\
	/* debug */							\
	FALSE,							\
	/* global data structure */					\
	0, TRUE								\
	}

// Global variable definition having its default values for binaries
// file -> local2 for mdtxd (set by /etc/init.d/mediatexd)
#define GLOBAL_MDTX_ENV_BIN						\
  {									\
    /* logging */							\
    99, 0,								\
      {	LOG_NOTICE, LOG_NOTICE, LOG_NOTICE, LOG_NOTICE,			\
	  LOG_NOTICE, LOG_NOTICE, LOG_NOTICE},				\
      0,								\
	/* allocating */						\
	128, (int (*)(long))0, 0,					\
	/* configration */						\
	DEFAULT_MDTXUSER, FALSE, FALSE, TRUE, 500*KILO,			\
	/* debug */						\
	FALSE,							\
	/* global data structure */					\
	0, TRUE								\
	}

#endif /* MDTX_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
