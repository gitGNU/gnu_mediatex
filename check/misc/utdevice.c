/*=======================================================================
 * Version: $Id: utdevice.c,v 1.6 2015/10/20 19:41:45 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
 *
 * Device informations

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
static void usage(char* programName)
{
  miscUsage(programName);
  fprintf(stderr, "\n\t\t -i device ");

  miscOptions();
  fprintf(stderr, "  ---\n" 
	  "  -i, --input-file\tinput device file to test\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Description: Unit test for md5sum module
 * Synopsis   : ./utcommand -i scriptPath
 * Input      : a file or a device
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputPath = 0;
  char* devicePath = 0;
  int isBlockDev = FALSE;
  int fd = 0;
  off_t size = 0;
  unsigned short int bs = 0;
  unsigned long int count = 0;
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
      if (!(inputPath = createString(optarg))) {
	fprintf(stderr, "cannot malloc the input device path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
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
    logMain(LOG_ERR, "Please provide an input device path");
    goto error;
  }

  if (!getDevice(inputPath, &devicePath)) goto error;
  if (!isBlockDevice(devicePath, &isBlockDev)) goto error;
  logMain(LOG_NOTICE, "%s -> %s (is %sa block device)",
	  inputPath, devicePath, isBlockDev?"":"not ");

  if ((fd = open(devicePath, O_RDONLY)) == -1) {
    logMain(LOG_ERR, "open: %s", strerror(errno));
    goto error;
  }
  
  // compute iso size if block device
  if (!getIsoSize(fd, &size, &count, &bs)) goto error;
  logMain(LOG_NOTICE, "%s is %san iso",
	  devicePath, (size > 0)?"":"not ");
  if (size > 0) {
    logMain(LOG_NOTICE, "volume size = %lu", count);
    logMain(LOG_NOTICE, "block size = %hu", bs);
    logMain(LOG_NOTICE, "size = %lli", (long long int)size);
  }
  /************************************************************************/

  rc = TRUE;
 error:
  destroyString(devicePath);
  destroyString(inputPath);
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


