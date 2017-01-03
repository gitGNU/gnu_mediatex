/*=======================================================================
 * Project: MediaTeX
 * Module : keys
 *
 * ssh keys retrievals

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
 * Input      : programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static 
void usage(char* programName)
{
  miscUsage(programName);
  fprintf(stderr, "\n\t\t-i path");

  miscOptions();
  fprintf(stderr, "  ---\n"
  	  "  -i, --input-key\tprint the key content\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for md5sum module
 * Synopsis   : ./utkeys -d ici -u toto -g toto -p 777
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputFile = 0;
  char* key = 0;
  char fingerprint[MAX_SIZE_MD5+1];
  int i;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"i:";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"input-key", required_argument, 0, 'i'},
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
      if ((inputFile = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the input device path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(inputFile, optarg, strlen(optarg)+1);
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
  
  // load the dsa public key
  if ((key = readPublicKey(inputFile)) == 0) goto error;
  if (!getFingerPrint(key, fingerprint)) goto error;

  // print the fingerprint
  printf("%c%c", fingerprint[0], fingerprint[1]);
  for (i=2; i<MAX_SIZE_MD5; i+=2)
    printf(":%c%c", fingerprint[i], fingerprint[i+1]);
  printf("\n");
  /************************************************************************/

  rc = TRUE;
 error:
  ENDINGS;
  rc=!rc;
 optError:
  if (inputFile) free(inputFile);
  if (key) free(key);
  exit(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
