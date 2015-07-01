/*=======================================================================
 * Version: $Id: utextractTree.c,v 1.1 2015/07/01 10:49:45 nroche Exp $
 * Project: MediaTeX
 * Module : extraction tree
 *
 * Extraction producer interface

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
 * Description: Unit test for confTree module.
 * Synopsis   : utconfTree
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = 0;
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
  if (!createExempleExtractTree(coll)) goto error;
  
  // test serializing
  if (!serializeExtractTree(coll)) {
    logMemory(LOG_ERR, "%s", "Error while serializing the extract exemple");
    goto error;
  }
  
  // test disease
  if (!diseaseExtractTree(coll)) goto error;
  env.dryRun = TRUE;
  if (!serializeExtractTree(coll)) goto error;
  if (avl_count(coll->extractTree->containers) != 0) goto error;
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

