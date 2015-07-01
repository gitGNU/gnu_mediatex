/*=======================================================================
 * Version: $Id: utserverTree.c,v 1.1 2015/07/01 10:49:46 nroche Exp $
 * Project: MediaTeX
 * Module : serverTree

* Server producer interface

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
  memoryUsage(programName);

  memoryOptions();
  //fprintf(stderr, "\t\t---\n");
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for serverTree module.
 * Synopsis   : utconfTree
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = 0;
  Server* server1 = 0;
  Server* server2 = 0;
  Server* server3 = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MEMORY_SHORT_OPTIONS;
  struct option longOptions[] = {
    MEMORY_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.dryRun = FALSE;
  env.debugMemory = TRUE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MEMORY_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!createExempleConfiguration()) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!createExempleServerTree(coll)) goto error;
  
  // test serializing
  if (!serializeServerTree(coll)) {
    logMemory(LOG_ERR, "%s", "sorry, cannot serialize the exemple");
    goto error;
  }

  if (!(server1 = getServer(coll, "746d6ceeb76e05cfa2dea92a1c5753cd"))
      || !(server2 = getServer(coll, "6b18ed0194b0fbadd08e0a13cccda00e"))
      || !(server3 = getServer(coll, "bedac32422739d7eced624ba20f5912e")))
    goto error;
  if (rgShareItems(server1->networks, server3->networks)) goto error;

  // test reachability
  if (!isReachable(coll, server1, server2)) goto error;
  if (isReachable(coll, server1, server3)) goto error;
  if (!isReachable(coll, server2, server1)) goto error;
  if (!isReachable(coll, server2, server3)) goto error;
  if (!isReachable(coll, server3, server1)) goto error;
  if (!isReachable(coll, server3, server2)) goto error;

  // test disease
  if (!diseaseServer(coll, server1)) goto error;
  if (!diseaseServer(coll, server2)) goto error;
  if (!diseaseServer(coll, server3)) goto error;
  env.dryRun = TRUE;
  if (!serializeServerTree(coll)) goto error;
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
