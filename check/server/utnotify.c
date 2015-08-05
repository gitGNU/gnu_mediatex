/*=======================================================================
 * Version: $Id: utnotify.c,v 1.2 2015/08/05 12:12:01 nroche Exp $
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
  Server* server = 0;
  Record* record = 0;
  char* extra = 0;
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
  if (!getLocalHost(coll)) goto error;
  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;

  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tgz")) goto error;

  // available incoming
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.iso")) goto error;

  utLog("%s", "add a demands and local supplies:", 0);
  
  if (!(archive = 
	addArchive(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075))) 
    goto error;
  if (!(server = addServer(coll, "7af51aceb06864e690fa6a9e00000001")))
    goto error;

  // remote demand
  if (!(extra = createString("!wanted"))) goto error;
  if (!(record = addRecord(coll, server, archive, DEMAND, extra)))
    goto error;
  if (!addCacheEntry(coll, record)) goto error;
  
  // local demand
  if (!(extra = createString("test@test"))) goto error;
  if (!(record = addRecord(coll, coll->localhost, archive, DEMAND, extra)))
    goto error;
  if (!addCacheEntry(coll, record)) goto error;

  if (!quickScan(coll)) goto error;
  utLog("%s", "we have :", coll);
  
  utLog("%s", "here we send", 0);
  if (!sendRemoteNotify(coll)) goto error;

#warning Needs more tests  
  //utLog("%s", "here we receive", 0);
  // ... need to copy the ring and clean the cache
  //if (!acceptRemoteNotify(diff, LOCALHOST)) goto error;

  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
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
