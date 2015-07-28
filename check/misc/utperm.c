/*=======================================================================
 * Version: $Id: utperm.c,v 1.3 2015/07/28 11:45:41 nroche Exp $
 * Project: MediaTeX
 * Module : perm
 *
 * misc stuffs

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
GLOBAL_STRUCT_DEF;

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
  fprintf(stderr, "\n\t\t -d directory -u user -g group [ -p mode ]");

  miscOptions();
  fprintf(stderr, "  ---\n"
	  "  -d, --dir\tpath to the directory to check\n"
	  "  -u, --user\texpected owner user of the directory\n"
	  "  -g, --group\texpected owner group of the directory\n"
	  "  -p, --perm\texpected permissions on the directory (777)\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for md5sum module
 * Synopsis   : ./utperm -d ici -u toto -g toto -p 777
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputPath = 0;
  char* user = 0;
  char* group = 0;
  mode_t mode = 0111;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"d:u:g:p:";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"dir", required_argument, 0, 'd'},
    {"user", required_argument, 0, 'u'},
    {"group", required_argument, 0, 'g'},
    {"perm", required_argument, 0, 'p'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.noRegression = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'd':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for directory\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((inputPath = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the directory path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(inputPath, optarg, strlen(optarg)+1);
      break;
      
    case 'u':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for user\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((user = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the user name: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(user, optarg, strlen(optarg)+1);
      break;

    case 'g':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for group\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((group = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the group name: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(group, optarg, strlen(optarg)+1);
      break;

    case 'p':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the permission\n",
		programName);
	rc = EINVAL;
	break;
      }
      if (sscanf(optarg, "%o", (unsigned int *)&mode) != 1) {
	fprintf(stderr, "sscanf: %s\n", strerror(errno));
	rc = EINVAL;
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
  logMain(LOG_NOTICE, "unit test's current date : %d", currentTime());

  if (inputPath == 0) {
    usage(programName);
    logMain(LOG_ERR, "%s", "Please provide a directory to check");
    goto error;
  }

  if (user == 0) {
    usage(programName);
    logMain(LOG_ERR, "%s", "Please provide a user");
    goto error;
  }

  if (group == 0) {
    usage(programName);
    logMain(LOG_ERR, "%s", "Please provide a group");
    goto error;
  }

  if (!checkDirectory(inputPath, user, group, mode)) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  if (inputPath) free(inputPath);
  if (user) free(user);
  if (group) free(group);
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
