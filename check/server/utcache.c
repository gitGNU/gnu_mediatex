/*=======================================================================
 * Project: MediaTeX
 * Module : cache
 *
 * Manage local cache directory and DB

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

#include "mediatex.h"
#include "server/mediatex-server.h"
#include "server/utFunc.h"

static int addFiles(Collection* coll, char* inputRep)
{
  if (!utCopyFileOnCache(coll, inputRep, "logo.png")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tgz")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.iso")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.iso")) goto error;
  return TRUE;
 error:
  return FALSE;
}

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually argv[0].
 * Output     : N/A
 =======================================================================*/
static void 
usage(char* programName)
{
  mdtxUsage(programName);
  fprintf(stderr, " [ -d repository ]");

  mdtxOptions();
  fprintf(stderr, "  ---\n"
	  "  -d, --input-rep\tsrcdir directory for make distcheck\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for cache module.
 * Synopsis   : ./utcache
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
  Collection* coll = 0;
  Archive* archive = 0;
  Record* record = 0;
  off_t size = 0;
  char* extra = 0;
  char path[256];
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"input-rep", required_argument, 0, 'd'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'd':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input repository\n",
		programName);
	rc = EINVAL;
	break;
      }
      strncpy(inputRep, optarg, strlen(optarg)+1);
      break; 
     
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;
  /************************************************************************/
  utLog("%s", "Unit test Cleanup:", 0);
  if (!utCleanCaches()) goto error;

  // test getFinalSupplyInPath function
  utLog("%s", "1) Paths", 0);
  strcpy(path, "/path/to/file");
  extra = getFinalSupplyInPath(path);
  logMain(LOG_NOTICE, "finalSupply input %s -> %s", path, extra);
  extra = destroyString(extra);
  extra = getFinalSupplyOutPath(path);
  logMain(LOG_NOTICE, "finalSupply output %s -> %s", path, extra);
  extra = destroyString(extra);
  strcpy(path, "/path/to/file:");
  extra = getFinalSupplyInPath(path);
  logMain(LOG_NOTICE, "finalSupply input %s -> %s", path, extra);
  extra = destroyString(extra);
  extra = getFinalSupplyOutPath(path);
  logMain(LOG_NOTICE, "finalSupply output %s -> %s", path, extra);
  extra = destroyString(extra);
  strcpy(path, "/path/to/file:place/to/strore/newFilename");
  extra = getFinalSupplyInPath(path);
  logMain(LOG_NOTICE, "finalSupply input %s -> %s", path, extra);
  extra = destroyString(extra);
  extra = getFinalSupplyOutPath(path);
  logMain(LOG_NOTICE, "finalSupply output %s -> %s", path, extra);
  extra = destroyString(extra);
  strcpy(path, "/path/to/file:place/to/strore/newDirectory/");
  extra = getFinalSupplyInPath(path);
  logMain(LOG_NOTICE, "finalSupply input %s -> %s", path, extra);
  extra = destroyString(extra);
  extra = getFinalSupplyOutPath(path);
  logMain(LOG_NOTICE, "finalSupply output %s -> %s", path, extra);
  extra = destroyString(extra);
  //===============================================================
  
  // test (re-)loading functions
  utLog("%s", "2) API", 0);
  if (!(coll = mdtxGetCollection("coll1"))) goto error;
  if (!addFiles(coll, inputRep)) goto error;
  utLog("%s", "Begin with:", coll);

  utLog("%s", "2.1) Reload the cache:", 0);
  if (!loadCache(coll)) goto error; 
  if (!loadCache(coll)) goto error; // test removing support file
  utLog("%s", "Reload gives:", coll);
  
  utLog("%s", "2.2) Quick scan the cache directory:", 0);
  if (!scanCollection(coll, TRUE)) goto error;
  utLog("%s", "Quick scan gives:", coll);
    
  utLog("%s", "2.3) Scan the cache directory:", 0);
  coll->fileState[iCACH] = LOADED; // force loosing modifications 
  if (!diseaseCollection(coll, CACH)) goto error;
  if (!loadCache(coll)) goto error;
  utLog("%s", "Begin with:", coll);
  if (!scanCollection(coll, FALSE)) goto error;
  utLog("%s", "Scan gives:", coll);

  utLog("%s", "2.4) Trim cat1:", 0);
  if (!trimCache(coll)) goto error;
  utLog("%s", "Trim gives:", coll);

  utLog("%s", "2.5) Clean cat1 and iso1:", 0);
  coll->fileState[iCACH] = LOADED; // force loosing modifications 
  if (!diseaseCollection(coll, CACH)) goto error;
  if (!addFiles(coll, inputRep)) goto error;
  if (!loadCache(coll)) goto error;
  utLog("%s", "Begin with:", coll);
  if (!cleanCache(coll)) goto error;
  utLog("%s", "Clean gives:", coll);
  
  utLog("%s", "2.6) Clean only cat1 (no local image):", 0);
  coll->fileState[iCACH] = LOADED; // force loosing modifications 
  if (!diseaseCollection(coll, CACH)) goto error;
  if (!addFiles(coll, inputRep)) goto error;
  if (!loadCache(coll)) goto error;

  // iso1 no more available from local supports 
  if (!(archive =
	getArchive(coll, "de5008799752552b7963a2670dc5eb18", 391168)))
    goto error;
  if (!delImage(coll, getImage(coll, coll->localhost, archive)))
    goto error;
  
  utLog("%s", "Begin with:", coll);
  if (!cleanCache(coll)) goto error;
  utLog("%s", "Clean gives:", coll);

  utLog("%s", "2.7) Purge iso1 :", 0);
  if (!purgeCache(coll)) goto error;
  utLog("%s", "Purge gives:", coll);
  //===============================================================

  utLog("%s", "3) Allocations...", 0);
  if (!(coll = mdtxGetCollection("coll3"))) goto error;
  if (!addFiles(coll, inputRep)) goto error;
  if (!loadCache(coll)) goto error;
  utLog("%s", "Begin with:", coll);
  /*--------------------------------------------------------*/
  utLog("%s", "3.1) Keep logoP1.iso and logoP2.iso:", 0);
  if (!(archive =
  	getArchive(coll, "de5008799752552b7963a2670dc5eb18", 391168)))
    goto error;
  if (!keepArchive(coll, archive)) goto error;
  if (!(archive =
  	getArchive(coll, "0a7ecd447ef2acb3b5c6e4c550e6636f", 374784)))
    goto error;
  if (!keepArchive(coll, archive)) goto error;
  utLog("%s", "Now we have :", coll);

  /*--------------------------------------------------------*/
  utLog("%s", "3.2) Ask for too much place :", 0);
  size = coll->cacheTree->totalSize;
  if (!(archive = addArchive(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  			     size))) goto error;
  if (cacheAlloc(&record, coll, archive)) goto error;
  utLog("reply : %i", record?1:0, coll); // 0: too much
  
  /*--------------------------------------------------------*/
  utLog("%s", "3.3) Ask for little place so do not suppress anything :", 0);
  size = coll->cacheTree->totalSize - coll->cacheTree->useSize;
  archive->size = size;
  record = 0;
  if (!cacheAlloc(&record, coll, archive)) goto error;
  record->extra[0]='a'; // MALLOC_SUPPLY -> LOCAL_SUPPLY
  // ALLOCATED -> AVAILABLE
  archive->extractScore = 9;
  if (!computeArchiveStatus(coll, archive)) goto error;
  extra = getAbsoluteCachePath(coll, record->extra);
  fopen(extra, "w"); // something to unlink
  utLog("reply : %i", record?1:0, coll); // 1: already avail

  /*--------------------------------------------------------*/
  utLog("%s", "3.4) Ask for place so as we need to suppress files :", 0);
  size = coll->cacheTree->totalSize - coll->cacheTree->frozenSize;
  if (!(archive = addArchive(coll, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
  			    size))) goto error;
  record = 0;
  if (!cacheAlloc(&record, coll, archive)) goto error;
  utLog("reply : %i", record?1:0, coll); // 1: del some entries
  if (!statusCache(coll)) goto error;
  if (!delCacheEntry(coll, record)) goto error;
  record = 0;

  /*--------------------------------------------------------*/
  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  extra = destroyString(extra);
  record = destroyRecord(record);
  freeConfiguration();
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
