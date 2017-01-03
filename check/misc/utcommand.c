/*=======================================================================
 * Project: Mediatex
 * Module : unit tests
 *
 * test for system API

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche

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
  miscUsage(programName);
  fprintf(stderr, "\n\t\t  -i scriptPath");

  miscOptions();
  fprintf(stderr, "  ---\n");
  fprintf(stderr, "  -i, --input-file\tinput script to exec\n");

  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/11/11
 * Description: Unit test for md5sum module
 * Synopsis   : ./utcommand -i scriptPath
 * Input      : -i option for scriptPath to exec
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputFile = 0;
  char *argvExec[] = { 0, "parameter1", 0};
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"i:";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"input-file", required_argument, 0, 'i'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'i':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the input stream\n", 
		programName);
	rc = EINVAL;
      }
      inputFile = optarg;
      break;
      
      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (inputFile == 0) {
    usage(programName);
    goto error;
  }

  argvExec[0] = inputFile;
  if (!execScript(argvExec, 0, 0, FALSE)) goto error;
  /************************************************************************/

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

