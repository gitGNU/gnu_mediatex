
/*=======================================================================
 * Project: MediaTeX
 * Module : conf
 *
 * unit test for conf

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
#include "client/mediatex-client.h"

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
 * modif      : 2010/12/10
 * Description: entry point for conf module
 * Synopsis   : ./utconf
 * Input      : N/A
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll3 = 0;
  Collection* coll4 = 0;
  char* supp = "SUPP21_logo.part1";
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS;
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;
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
  if (!(coll3 = addCollection("coll3"))) goto error;

  logMain(LOG_NOTICE, "*** List collections: ");
  if (!mdtxListCollection(FALSE)) goto error;

  logMain(LOG_NOTICE, "*** List master collections: ");
  if (!mdtxListCollection(TRUE)) goto error;

  logMain(LOG_NOTICE, "*** Add a collection:");
  if (!(coll4 = createCollection())) goto error;
  strncpy(coll4->label, "coll4", MAX_SIZE_COLL);
  strncpy(coll4->masterHost, "localhost", MAX_SIZE_HOST);
  if (!mdtxAddCollection(coll4)) goto error;
  coll4 = destroyCollection(coll4);

  logMain(LOG_NOTICE, "*** List collections:");
  if (!mdtxListCollection(FALSE)) goto error;

  logMain(LOG_NOTICE, "*** Del collection coll 4:");
  if (!mdtxDelCollection("coll4")) goto error;
  if (!saveConfiguration("topo")) goto error;

  logMain(LOG_NOTICE, "*** Share a support:");
  if (!mdtxShareSupport(supp, "coll3")) goto error;
  if (!saveConfiguration("topo")) goto error;

  logMain(LOG_NOTICE, "*** Share a support second time:");
  if (!mdtxShareSupport(supp, "coll3")) goto error;
  
  logMain(LOG_NOTICE, "*** Share support with all collections:");
  if (!mdtxShareSupport(supp, 0)) goto error;
  if (!saveConfiguration("topo")) goto error;

  logMain(LOG_NOTICE, "*** Withdraw a support:");
  if (!mdtxWithdrawSupport(supp, "coll3")) goto error;
  if (!saveConfiguration("topo")) goto error;

  logMain(LOG_NOTICE, "*** Withdraw second time:");
  if (!mdtxWithdrawSupport(supp, "coll3")) goto error;

  logMain(LOG_NOTICE, "*** Withdraw support from all collections:");
  if (!mdtxWithdrawSupport(supp, 0)) goto error;
  if (!saveConfiguration("topo")) goto error;
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
