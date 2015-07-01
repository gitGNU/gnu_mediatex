/*=======================================================================
 * Version: $Id: utconfTree.c,v 1.1 2015/07/01 10:49:45 nroche Exp $
 * Project: mediaTeX
 * Module : configuration
 *
 * /etc configuration producer interface

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
  memoryUsage(programName);

  memoryOptions();
  //fprintf(stderr, "\t\t---\n");
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for confTree module.
 * Synopsis   : ./utconfTree -i
 * Input      : N/A
 * Output     : create the mediatex.conf file
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = 0;
  Collection* coll = 0;
  char* string = 0;
  RGIT* curr = 0;
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
  // test building and serializing
  if (!createExempleConfiguration()) goto error;  
  if (!serializeConfiguration(getConfiguration())) goto error;

  // test accessing the tree
  if (getCollection("coll0")) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!(coll = getCollection("coll2"))) goto error;
  if (!(coll = getCollection("coll3"))) goto error;
  if (!addCollection("coll4")) goto error;
  freeConfiguration();

  // Create 2 other configurations for later unit-tests based on
  // network topologie (utNotify: not used, utExtract: todo). 
  // So we have:
  // - mdtx1 on "www" network (default)
  // - mdtx2 is the gateway for private1 network
  // - mdtx3 on "private1" network

  // Each server share together 3 collections.
  // Here, the collection's server keys are the sames for each server
  // on every collection. This should never append, but remains possible.

  env.confLabel="mdtx2";
  if (!createExempleConfiguration()) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!(string = addNetwork("www"))) goto error;
  if (!rgInsert(conf->networks, string)) goto error;
  if (!(string = addNetwork("private1"))) goto error;
  if (!rgInsert(conf->networks, string)) goto error;
  if (!rgInsert(conf->gateways, string)) goto error;

  // overwrite gateway on collection settings
  if (!(coll = getCollection("coll3"))) goto error;
  if (!(string = addNetwork("private2"))) goto error;
  if ((curr = rgHaveItem(coll->gateways, string))) {
    rgRemove_r(coll->gateways, &curr);
  }
  if (!(string = addNetwork("none"))) goto error;
  if (!rgInsert(coll->gateways, string)) goto error;

  if (!serializeConfiguration(getConfiguration())) goto error;
  freeConfiguration();

  env.confLabel="mdtx3";
  if (!createExempleConfiguration()) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!(string = addNetwork("private1"))) goto error;
  if (!rgInsert(conf->networks, string)) goto error;
  if (!serializeConfiguration(getConfiguration())) goto error;
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

