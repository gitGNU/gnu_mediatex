/*=======================================================================
 * Version: $Id: utmessage.c,v 1.5 2015/10/20 19:41:48 nroche Exp $
 * Project: MediaTeX
 * Module : have
 *
 * Unit test for have

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

// callback functions requiered (not used here)
int hupManager(){return 0;};
int termManager(){return 0;};
void* signalJob(void* arg){return 0;};
void* socketJob(void* arg){return 0;};

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
 * Description: Unit test for incoming messages
 * Synopsis   : ./utmessage
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = 0;
  Connexion* connexion = 0;
  Server* server = 0;
  Server* localhost = 0;
  Record* record = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS    ;
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
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
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll = mdtxGetCollection("coll1"))) goto error;
  if (!(server = createServer())) goto error;
  strcpy(server->fingerPrint, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  if (!(connexion = utConnexion(0, 0, server))) goto error;

  utLog("%s", "no collection", 0);
  if (checkMessage(connexion)) goto error;
  logMain(LOG_NOTICE, "reply : %s", connexion->status);
  connexion->message->collection = coll;

  utLog("%s", "cannot match server", 0);
  if (checkMessage(connexion)) goto error;
  logMain(LOG_NOTICE, "reply : %s", connexion->status);
  if (!(localhost = getLocalHost(coll))) goto error;
  strcpy(connexion->message->fingerPrint, localhost->fingerPrint);

  utLog("%s", "record related to other server", 0);
  if (!(record = utRemoteDemand(coll, server))) goto error;
  if (!rgInsert(connexion->message->records, record)) goto error;
  if (checkMessage(connexion)) goto error;
  logMain(LOG_NOTICE, "reply : %s", connexion->status);
  /************************************************************************/

  rc = TRUE;
 error:
  destroyServer(server);
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

