/*=======================================================================
 * Version: $Id: cgiSrv.c,v 1.1 2014/10/13 19:39:53 nroche Exp $
 * Project: MediaTeX
 * Module : cgi-server
 *
 * Manage cgi queries

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Nicolas Roche
 
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

#include "../mediatex.h"
#include "../misc/log.h"
#include "../misc/tcp.h"
#include "../misc/md5sum.h"
#include "../memory/confTree.h"
#include "../common/openClose.h"
#include "cache.h"
#include "extract.h"
#include "cgiSrv.h"

#include <sys/socket.h> // shutdown

static char message[][64] = {
  "000 no error\n",               // not used
  "100 nobody\n",                 // not used (non sens)
  "200 ok\n",                     // not used (need to add the path)
  "300 not here\n",
  "400 internal error\n",

  "301 collection not managed\n" // cgiErrno = 5
  "302 no extraction plan\n"     // cgiErrno = 6 (not use in fact)
};

// tcpWrite only deal with sockets (on unit test we use stdout)
#ifdef utMAIN
#define tcpWrite write
#endif

/*=======================================================================
 * Function   : extractLocalArchive
 * Description: Get or extract one archive into the local cache
 * Synopsis   : Archive* extractLocalArchive(Archive* archive)
 * Input      : Archive* archive = what we are looking for
 * Output     : Archive* = copy of the archive researched
 =======================================================================*/
int 
extractCgiArchive(Collection* coll, Archive* archive, int* found)
{
  int rc = FALSE;
  Configuration* conf = NULL;
  ExtractData data;

  logEmit(LOG_DEBUG, "%s", "local extraction");
  *found = FALSE;
  data.coll = coll;
  data.context = X_CGI;
  if (!(data.toKeeps = createRing())) goto error;

  if (!(conf = getConfiguration())) goto error;
  if (!loadCollection(coll, SERV | EXTR | CACH)) goto error;
  data.target = archive;
  if (!extractArchive(&data, archive)) goto error2;

  *found = data.found;
  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV | EXTR | CACH)) rc = FALSE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "local extraction fails");
  }
  destroyOnlyRing(data.toKeeps);
  return rc;
} 

/*=======================================================================
 * Function   : cgiServer
 * Description: Deal with the CGI client
 * Synopsis   : int cgiServer(ArchiveTree* demands, int fd)
 * Input      : ArchiveTree* demands = what we have reveive from socket
 *              int fd = the socket for replying
 * Output     : TRUE on success

 * Note: Only the first archive is processed
 =======================================================================*/
int cgiServer(RecordTree* recordTree, Connexion* connexion)
{
  int rc = FALSE;
  Collection* coll = NULL;
  Record* record = NULL;
  Record* record2 = NULL;
  Archive* archive = NULL;
  int found = FALSE;
  char buffer[256];
  char* extra = NULL;
  int cgiErrno = 0;

  logEmit(LOG_DEBUG, "%s", "cgiServer: serving cgi query");
  /* #ifdef utMAIN */
  /*   logEmit(LOG_INFO, "%s", "input archive tree:"); */
  /* serializeRecordTree(recordTree, NULL); */
  /* #endif */

  // get the archiveTree's collection
  if ((coll = recordTree->collection) == NULL) {
    logEmit(LOG_ERR, "%s", "unknown collection for archiveTree");
    cgiErrno = 5;
    goto error;
  }

  if (isEmptyRing(recordTree->records)) {
    logEmit(LOG_ERR, "%s", "please provide a Record tree for cgiServer");
    cgiErrno = 4;
    goto error;
  }

  // only process the first archive
  if (!(record = (Record*)recordTree->records->head->it)) goto error;
  if (!(archive = record->archive)) goto error;

  /* #ifdef utMAIN */
  /*   logEmit(LOG_INFO, "%s", "first archive:"); */
  /*   serializeRecord(recordTree, record); */
  /* #endif */

  if (!lockCacheRead(coll)) goto error;

  if (isEmptyString(record->extra) || !strncmp(record->extra, "!w", 2)) {

    // synchronous query: search/extraction
    cgiErrno = 3;
    if (!extractCgiArchive(coll, archive, &found)) goto error2;
    if (!found) {
      logEmit(LOG_NOTICE, "not found %s:%i", archive->hash, archive->size);
    }
    else {
      logEmit(LOG_NOTICE, "query for %s", archive->localSupply->extra);
      sprintf(buffer, "200 %s/%s\n", 
	      coll->cacheUrl, archive->localSupply->extra);
      tcpWrite(connexion->sock, buffer, strlen(buffer));
      cgiErrno = 2;
    }
  }
  else {
    // register query: merge archive trees
    logEmit(LOG_NOTICE, "final demand for %s:%lli %s", 
	    archive->hash, archive->size, record->extra);
    cgiErrno = 4;
    if (!(extra = createString(record->extra))) goto error2;
    if (!(record2 = addRecord(coll, coll->localhost, archive, 
			      DEMAND, extra))) goto error2;
    if (!addCacheEntry(coll, record2)) goto error2;
    tcpWrite(connexion->sock, "200 \n", 5);
    cgiErrno = 2;
  }
      
  rc = TRUE;
 error2:
  if (!unLockCache(coll)) rc = FALSE;
 error:
  if (cgiErrno != 2) {
    sprintf(buffer, "%s", message[cgiErrno]);
    tcpWrite(connexion->sock, buffer, strlen(buffer));
  }
  if (connexion->sock != STDOUT_FILENO && 
      shutdown(connexion->sock, SHUT_WR) == -1) {
    logEmit(LOG_ERR, "shutdown fails: %s", strerror(errno));
  }
  if (cgiErrno == 4) {
    logEmit(LOG_ERR, "%s", "fails to serve cgi query");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "utFunc.h"
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
 * modif      : 2013/01/10
 * Description: Unit test for cache module.
 * Synopsis   : ./utcache
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
  Collection* coll = NULL;
  Connexion connexion;
  RecordTree* tree = NULL;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"input-rep", required_argument, NULL, 'd'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
    case 'd':
      if(optarg == NULL || *optarg == (char)0) {
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
  connexion.sock = STDOUT_FILENO;
  if (!(tree = ask4logo(coll, NULL))) goto error;

  utLog("%s", "Clean the cache:", NULL);
  if (!utCleanCaches()) goto error;
  
  utLog("%s", "1) try to extract the demand", NULL);
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "2) retry with it on cache", NULL);
  if (!utCopyFileOnCache(coll, inputRep, "logo.png")) goto error;
  if (!quickScan(coll)) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "3) retry with tgz on cache", NULL);
  if (!utCleanCaches()) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tgz")) goto error;
  if (!quickScan(coll)) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "4) try with only part1 on cache", NULL);
  if (!utCleanCaches()) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!quickScan(coll)) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "5) try with part2 added on cache too", NULL);
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.cat")) goto error;
  if (!quickScan(coll)) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "6) register a mail", NULL);
  if (!utCleanCaches()) goto error;
  if (!quickScan(coll)) goto error;
  tree = destroyRecordTree(tree);
  if (!(tree = ask4logo(coll, "test@test.com"))) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "Clean the cache:", NULL);
  if (!utCleanCaches()) goto error;
  tree = destroyRecordTree(tree);
  /************************************************************************/

  rc = TRUE;
 error:
  freeConfiguration();
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

#endif // utMAIN
