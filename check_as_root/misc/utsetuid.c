/*=======================================================================
 * Version: $Id: utsetuid.c,v 1.2 2015/07/28 11:45:44 nroche Exp $
 * Project: MediaTeX
 * Module : command
 *
 * setuid API
 *
 * Note: setuid do not works with threads.

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
 * Synopsis   : static void usage (char* programName)
 * Input      : programName = the name of the program; usually argv[0].
 * Output     : N/A
 =======================================================================*/
static void 
usage (char* programName)
{
  mdtxUsage (programName);
  fprintf (stderr, "\n\t\t -u user");
  fprintf (stderr, "\n\t\t -i scriptPath");

  mdtxOptions();
  fprintf (stderr, "  ---\n");
  fprintf (stderr, "  -u, --sudo-user\tuser to become\n");
  fprintf (stderr, "  -i, --input-file\tinput script to exec\n");

  fprintf (stderr, "\nNeed prior to do as root:\n" \
	  "# chown root. utsetuid\n" \
	  "# chmod u+s utsetuid\n");

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
main (int argc, char** argv)
{
  char* inputFile = 0;
  int i;
  char *argvExec[] = { 0, "parameter1", 0};
  int uid = getuid();
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"i:u:";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"sudo-user", required_argument, 0, 'u'},
    {"input-file", required_argument, 0, 'i'},
    {0, 0, 0, 0}
  };

  // import misc environment
  env.noRegression = FALSE;
  getEnv (&env);

  // parse the command line
  while ((cOption = getopt_long (argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch (cOption) {
      
    case 'i':
      if (optarg == 0 || *optarg == (char)0) {
	fprintf (stderr, 
		 "%s: nil or empty argument for the input stream\n", 
		 programName);
	rc = EINVAL;
      }
      inputFile = optarg;
      break;

    case 'u':
      if (optarg == 0 || *optarg == (char)0) {
	fprintf (stderr, "%s: nil or empty argument for the user name\n", 
		programName);
	rc = EINVAL;
      }
      env.confLabel = optarg;
      break;
      
      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv (programName, &env)) goto optError;

  /************************************************************************/
  if (env.confLabel == 0) {
    usage (programName);
    logMain (LOG_ERR, "%s", "Please provide a user to become");
    goto error;
  }
  if (inputFile == 0) {
    usage (programName);
    logMain (LOG_ERR, "%s", "Please provide an input file");
    goto error;
  }
  argvExec[0] = inputFile;

  // must be done first
  if (!undo_seteuid ()) goto error;

  // first layer
  logMain (LOG_NOTICE, "%s", "** first layer");
  for (i=0; i<2; ++i) {

    // get privileges
    if ((rc = setresuid (0, 0, -1))) {
      logMain (LOG_ERR, "setresuid fails: %s", strerror (errno));
      goto error;
    }

    logMain (LOG_INFO, "> ruid=%i euid=%i", getuid (), geteuid ());

    // change to label user
    if ((rc = setresuid (uid, uid, 0))) {
      logMain (LOG_ERR, "setrsuid fails: %s", strerror (errno));
      goto error;
    }

    logMain (LOG_INFO, "< ruid=%i euid=%i", getuid (), geteuid ());
  }

  // API for wrapper
  logMain (LOG_NOTICE, "%s", "** API for wrapper");
  uid = getuid();
  for (i=0; i<2; ++i) {
    if (!(rc = execScript (argvExec, 0, 0, FALSE))) goto error;
    if (!becomeUser(env.confLabel, TRUE)) goto error;
    if (!(execScript (argvExec, 0, 0, FALSE))) goto error;
    if (!logoutUser(uid)) goto error;
  }

  // API for thread
  logMain (LOG_NOTICE, "%s", "** API for thread");
  for (i=0; i<2; ++i) {
    if (!execScript(argvExec, env.confLabel, 0, FALSE)) goto error;
    if (!execScript(argvExec, 0, 0, FALSE)) goto error;
  }
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

