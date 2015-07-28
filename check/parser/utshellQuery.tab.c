/*=======================================================================
 * Version: $Id: utshellQuery.tab.c,v 1.2 2015/07/28 11:45:42 nroche Exp $
 * Project: Mediatex
 * Module : shell parser
 *
 * test for shell query parser

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
  parserUsage(programName);
  fprintf(stderr, " query");

  parserOptions();
  fprintf(stderr, "  ---\n"
	  "  query\t\t\tTo document here...\n");
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for confFile module.
 * Synopsis   : utconfFile
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = PARSER_SHORT_OPTIONS"i";
  struct option longOptions[] = {
    PARSER_LONG_OPTIONS,
    {0, 0, 0, 0}
  };
       
  // import mediatex environment
  getEnv(&env);

  env.logSeverity[LOG_MEMORY] = LOG_NOTICE;
  env.logSeverity[LOG_MAIN] = LOG_NOTICE;

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {

      GET_PARSER_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mediatex environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(conf = getConfiguration())) goto error;
  if (!parseShellQuery(argc, argv, optind)) goto error;
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

 
