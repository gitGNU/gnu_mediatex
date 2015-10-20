 /*=======================================================================
 * Version: $Id: utmisc.c,v 1.8 2015/10/20 19:41:42 nroche Exp $
 * Project: MediaTeX
 * Module : misc
 *
 * Unit test for misc

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
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2010/12/10
 * Description: entry point for make module
 * Synopsis   : ./utmake
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* path = 0;
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
  //env.dryRun = FALSE;
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
  // build the HTML catalog
  logMain(LOG_NOTICE, "*************************************************");
  env.dryRun = FALSE;
  if (!mdtxMake("coll1")) goto error;

  // build the audit file (for later test done by utdeliver.c)
  logMain(LOG_NOTICE, "*************************************************");
  env.dryRun = FALSE;
  if (!mdtxAudit("coll1", "test@mediatex.org")) goto error;

  // display the audit file
  logMain(LOG_NOTICE, "*************************************************");
  env.dryRun = TRUE;
  if (!mdtxAudit("coll1", "test@mediatex.org")) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  path = destroyString(path);
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
