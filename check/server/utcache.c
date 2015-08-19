/*=======================================================================
 * Version: $Id: utcache.c,v 1.6 2015/08/19 01:09:07 nroche Exp $
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
#include "server/mediatex-server.h"
#include "server/utFunc.h"

GLOBAL_STRUCT_DEF;

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
	  "  -d, --input-rep\trepository with logo files\n");
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
  Connexion* connexion = 0;
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
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
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
  if (!(coll = mdtxGetCollection("coll3"))) goto error;

  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;

  utLog("%s", "Scan the cache directory :", 0);
  if (!utCopyFileOnCache(coll, inputRep, "logo.png")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tgz")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.iso")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.iso")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan gives :", coll);

  /*--------------------------------------------------------*/
  utLog("%s", "Keep logoP1.iso and logoP2.iso:", 0);
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
  utLog("%s", "Ask for too much place :", 0);
  size = coll->cacheTree->totalSize;
  if (!(archive = addArchive(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  			     size))) goto error;
  if (cacheAlloc(&record, coll, archive)) goto error;
  utLog("reply : %i", record?1:0, coll); // 0: too much
  
  /*--------------------------------------------------------*/
  utLog("%s", "Ask for little place so do not suppress anything :", 0);
  size = coll->cacheTree->totalSize - coll->cacheTree->useSize;
  archive->size = size;
  record = 0;
  if (!cacheAlloc(&record, coll, archive)) goto error;
  record->extra[0]='*';
  utLog("reply : %i", record?1:0, coll); // 1: already avail
  if (!delCacheEntry(coll, record)) goto error;

  /*--------------------------------------------------------*/
  utLog("%s", "Ask for place so as we need to suppress files :", 0);
  size = coll->cacheTree->totalSize - coll->cacheTree->frozenSize;
  if (!(archive = addArchive(coll, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
  			    size))) goto error;
  record = 0;
  if (!cacheAlloc(&record, coll, archive)) goto error;
  record->extra[0]='*'; // ALLOCATED -> AVAILABLE
  if (!delCacheEntry(coll, record)) goto error;
  utLog("reply : %i", record?1:0, coll); // 1: del some entries
  record = 0;

  /*--------------------------------------------------------*/
  utLog("%s", "API to upload files :", 0);
  if (!(extra = createString(inputRep))) goto error;
  if (!(extra = catString(extra, "/../misc/"))) goto error;
  if (!(extra = catString(extra, "README"))) goto error;
  if (!(connexion = utUploadMessage(coll, extra))) goto error;
  if (!uploadFinaleArchive(connexion)) {
    utLog("reply : %s", connexion->status, 0);
    goto error;
  }
  utLog("reply : %s", "upload ok", coll);

  /*--------------------------------------------------------*/
  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  extra = destroyString(extra);
  record = destroyRecord(record);
  if (connexion) destroyRecordTree(connexion->message);
  free (connexion);
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
