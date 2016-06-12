/*=======================================================================
 * Project: MediaTeX
 * Module : checksums
 *
 * md5sum computation

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

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static 
void usage(char* programName)
{
  miscUsage(programName);
  fprintf(stderr, "\n\t\t[ -i device ] [ -p ]");

  miscOptions();
  fprintf(stderr, "  ---\n"
	  "  -i, --input-file\tinput device to compute checksums on\n"
	  "  -p, --no-progbar\tdo not show the progbar\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for md5sum module
 * Synopsis   : ./utcommand -i scriptPath
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputPath = 0;
  CheckData data;
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
	fprintf(stderr, "%s: nil or empty argument for the input device\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((inputPath = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the input device path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(inputPath, optarg, strlen(optarg)+1);
      break;

      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (inputPath == 0) {
    usage(programName);
    logMain(LOG_ERR, "Please provide a file to compute checksums");
    goto error;
  }

  memset((void*)&data, 0, sizeof(data)); // valgrind complains
  data.path = inputPath;
  
  logMain(LOG_NOTICE, 
	  "Quick computation, no path resolution, no progbar");
  data.opp = CHECK_CACHE_ID;
  if (!doChecksum(&data)) goto error;
  
  memset((void*)&data, 0, sizeof(data));
  data.path = inputPath;

  logMain(LOG_NOTICE, "Full computation, path resolution, progbar");
  data.opp = CHECK_SUPP_ADD;
  if (!doChecksum(&data)) goto error;

  logMain(LOG_NOTICE, "Quick computation, path resolution, progbar");
  data.opp = CHECK_SUPP_ID;
  if (!doChecksum(&data)) goto error;
        
  logMain(LOG_NOTICE, "Full check, path resolution, progbar");
  data.opp = CHECK_SUPP_CHECK;
  if (!doChecksum(&data)) goto error;
  
  if (data.rc != CHECK_SUCCESS) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  if (inputPath) free(inputPath);
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
