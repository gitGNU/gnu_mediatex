/*=======================================================================
 * Version: $Id: utnotify.c,v 1.3 2015/08/19 01:09:08 nroche Exp $
 * Project: MediaTeX
 * Module : notify

 * Unit test for notify

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

int running = TRUE;

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

  mdtxOptions();
  //fprintf(stderr, "  ---\n");
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

 * TODO       : unit test for the receive case
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
  Collection* coll = 0;
  Archive* archive = 0;
  Server* server1 = 0;
  Server* server2 = 0;
  Record* record = 0;
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
  if (!(coll = mdtxGetCollection("coll1"))) goto error;
  if (!(server2 = getLocalHost(coll))) goto error;
  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;
  
  /*
    we try to be in the same situation as describe by utserverTree
    server1: 746d6ceeb76e05cfa2dea92a1c5753cd
    server2: 6b18ed0194b0fbadd08e0a13cccda00e
    server3: bedac32422739d7eced624ba20f5912e
   */

  logMain(LOG_NOTICE, "localhost is %s", server2->fingerPrint);

  // remote server (we should be server2 as using -c mdtx2)
  if (!(server1 = addServer(coll, "746d6ceeb76e05cfa2dea92a1c5753cd")))
    goto error;

  if (!(archive = 
	addArchive(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075))) 
    goto error;

  // 1) test sending message
  utLog("%s", "*** test sending message:", 0);
  utLog("%s", "Populate the cache:", 0);

  // add a remote demand
  if (!(extra = createString("!wanted"))) goto error;
  if (!(record = addRecord(coll, server1, archive, DEMAND, extra)))
    goto error;
  if (!addCacheEntry(coll, record)) goto error;
  
  // add a local demand
  if (!utAddFinalDemand(coll)) goto error;

  // add local supplies (logoP1.cat have a good score,others no)
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tgz")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.iso")) goto error;
  if (!quickScan(coll)) goto error;

  utLog("%s", "we have :", coll);
  if (!sendRemoteNotify(coll)) goto error;

  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;

  // 2) test receiving message
  utLog("%s", "*** test receiving message:", 0);
  utLog("%s", "Build message:", 0);
  if (!(connexion = utConnexion(coll, NOTIFY, server1))) goto error;

  // add a remote demand
  if (!(extra = createString("!wanted"))) goto error;
  if (!(record = addRecord(coll, server1, archive, DEMAND, extra)))
    goto error;
  if (!rgInsert(connexion->message->records, record)) goto error;

  utLog("%s", "message we receive :", 0);
  logRecordTree(LOG_MAIN, LOG_NOTICE, connexion->message, 0);
  utLog("%s", "actually we have :", coll);
  if (!acceptRemoteNotify(connexion))  {
    utLog("reply : %s", connexion->status, 0);
    goto error;
  }
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
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
