/*=======================================================================
 * Version: $Id: utcacheTree.c,v 1.10 2015/10/20 19:41:43 nroche Exp $
 * Project: MediaTeX
 * Module : cache
 *
 * Manage local cache directory and DB

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

#include "mediatex.h"
#include "memory/utFunc.h"


/*=======================================================================
 * Function   : testThread
 * Description: Testing the concurent access
 * Synopsis   : void* testThread(void* arg)
 * Input      : void* arg: CacheTree* mergeMd5
 * Output     : N/A
 =======================================================================*/
void* 
testThread(void* arg)
{
  //env.recordTree = copyCurrentRecordTree(mergeMd5);
  return 0;
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
  memoryUsage(programName);
  fprintf(stderr, " [ -d repository ]");

  memoryOptions();
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
  RecordTree* tree = 0;
  Record* record = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MEMORY_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MEMORY_LONG_OPTIONS,
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

      GET_MEMORY_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!createExempleConfiguration(inputRep)) goto error;
  if (!(coll = getCollection("coll1"))) goto error;

  if (!lockCacheRead(coll)) goto error;
  if (!unLockCache(coll)) goto error;
  if (!lockCacheWrite(coll)) goto error;
  if (!unLockCache(coll)) goto error;
  if (!lockCacheRead(coll)) goto error;
  if (!lockCacheRead(coll)) goto error;
  if (!unLockCache(coll)) goto error;
  if (!unLockCache(coll)) goto error;

  if (!(tree = createExempleRecordTree(coll))) goto error;
  if (!computeExtractScore(coll)) goto error;

  // index the record tree into cache
  logMemory(LOG_DEBUG, "___ test indexation ___");
  tree->aes.fd = STDERR_FILENO;

  while ((record = rgHead(tree->records))) {
    logMain(LOG_NOTICE, "---"); 
    logMain(LOG_NOTICE, "add new record into cache"); 
    if (!serializeRecord(tree, record)) goto error;
    aesFlush(&tree->aes);
    fprintf(stderr, "\n");

    if (!addCacheEntry(coll, record)) goto error;
    rgRemove(tree->records);

    if (record->archive->state >= AVAILABLE) {
      logMain(LOG_NOTICE, "available: score=%.2f toKeep: %s\n",
		record->archive->extractScore, 
		record->archive->state == TOKEEP?"yes":"no");

      logMain(LOG_NOTICE, "keep record"); 
      if (!keepArchive(coll, record->archive)) goto error;

      logMain(LOG_NOTICE, "unkeep record"); 
      if (!unKeepArchive(coll, record->archive)) goto error;
    }

    logMain(LOG_NOTICE, "del record from cache"); 
    if (!delCacheEntry(coll, record)) goto error;
    if (!cleanCacheTree(coll)) goto error;
  }

  // free memory
  if (!diseaseCacheTree(coll)) goto error;
  tree = destroyRecordTree(tree);
  /************************************************************************/

  freeConfiguration();
  rc = TRUE;
 error:
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
