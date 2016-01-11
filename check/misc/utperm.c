/*=======================================================================
 * Version: $Id: utperm.c,v 1.8 2015/10/20 19:41:46 nroche Exp $
 * Project: MediaTeX
 * Module : perm
 *
 * misc stuffs

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
  fprintf(stderr, "\n\t\t -d directory -u user -g group [ -p mode ]");

  miscOptions();
  fprintf(stderr, "  ---\n"
	  "  -d, --dir\tpath to the directory to check\n"
	  "  -w, --pwd\tpath to the current directory (for make distcheck)\n"
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
  char* pwdPath = ".";
  char* user = 0;
  char* group = 0;
  mode_t mode = 0111;
  char file[MAX_SIZE_STRING+1];
  char dir[MAX_SIZE_STRING+1];
  char post[MAX_SIZE_STRING+1];
  char* output = 0;
  int isThere = 0;
  int isDirectory = 0;
  FILE* fd = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"d:u:g:p:w:";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"dir", required_argument, 0, 'd'},
    {"user", required_argument, 0, 'u'},
    {"group", required_argument, 0, 'g'},
    {"perm", required_argument, 0, 'p'},
    {"pwd", required_argument, 0, 'w'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;
  env.noRegression = FALSE;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
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

    case 'w':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for directory\n",
		programName);
	rc = EINVAL;
	break;
      }
      if (strcmp(optarg, ".")) {
	if ((pwdPath = malloc(strlen(optarg) + 1)) == 0) {
	  fprintf(stderr, "cannot malloc the directory path: %s", 
		  strerror(errno));
	  rc = ENOMEM;
	  break;
	}
	strncpy(pwdPath, optarg, strlen(optarg)+1);
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
    logMain(LOG_ERR, "Please provide a directory to check");
    goto error;
  }

  // Unit tests
  logMain(LOG_NOTICE, "***********************************************"); 
  if (user == 0 && group == 0) {
    logMain(LOG_NOTICE, "* Perm: internal tests");
    logMain(LOG_NOTICE, "***********************************************"); 
   
    // cleaning
    logMain(LOG_NOTICE, "* cleaning");
    sprintf(dir, "%s%s", pwdPath, "/tmp/dir1/");
    removeDir(pwdPath, dir);
    sprintf(dir, "%s%s", pwdPath, "/tmp/dir2/");
    removeDir(pwdPath, dir);
    sprintf(file, "%s%s", pwdPath, "/tmp/foo/bar.txt");
    unlink(file);
    sprintf(file, "%s%s", pwdPath, "/tmp/foo_00");
    unlink(file);
    sprintf(dir, "%s%s", pwdPath, "/tmp/foo/");
    removeDir(pwdPath, dir);

    // currentTime
    logMain(LOG_NOTICE, "* unit test's current date: %d", currentTime());

    // callAccess
    logMain(LOG_NOTICE, "* callAccess");
    if (!sprintf(file, "%s%s", pwdPath, "/doNotExists")) goto error;
    if (!callAccess(file, &isThere)) goto error;
    if (isThere) goto error;    
    // -
    if (!sprintf(file, "%s%s", inputPath, "/../misc/logo.png")) goto error;
    if (!callAccess(file, &isThere)) goto error;
    if (!isThere) goto error;

    // checkDirectory
    logMain(LOG_NOTICE, "* checkDirectory");
    if (!checkDirectory(file, &isDirectory)) goto error;
    if (isDirectory) goto error;
    // -
    if (!sprintf(dir, "%s%s", pwdPath, "/doNotExists")) goto error;
    if (checkDirectory(dir, &isDirectory)) goto error;
    // -
    if (!sprintf(dir, "%s%s", inputPath, "/../misc")) goto error;
    if (!checkDirectory(dir, &isDirectory)) goto error;
    if (!isDirectory) goto error;

    // makeDir
    logMain(LOG_NOTICE, "* makeDir");
    if (!sprintf(dir, "%s%s", pwdPath, "/tmp")) goto error;
    if (!checkDirectory(dir, &isThere)) goto error;
    if (!isThere) goto error;
    if (!sprintf(dir, "%s%s", pwdPath, "/tmp/someFile")) goto error;
    if (!makeDir(pwdPath, dir, 0777)) goto error;
    // -
    if (!sprintf(dir, "%s%s", pwdPath, "/tmp/dir1/")) goto error;
    if (!makeDir(pwdPath, dir, 0777)) goto error;
    if (!checkDirectory(dir, &isThere)) goto error;
    if (!isThere) goto error;
    // -
    if (!sprintf(dir, "%s%s", pwdPath, "/tmp/dir2/dir2/")) goto error;
    if (!makeDir(pwdPath, dir, 0777)) goto error;
    if (!checkDirectory(dir, &isThere)) goto error;
    if (!isThere) goto error;

    // removeDir
    logMain(LOG_NOTICE, "* removeDir");
    if (!sprintf(dir, "%s%s", pwdPath, "/tmp/")) goto error;
    if (!removeDir(pwdPath, dir)) goto error;
    // -
    if (!sprintf(dir, "%s%s", pwdPath, "/tmp/dir1/someFile")) goto error;
    if (!removeDir(pwdPath, dir)) goto error;
    if (!callAccess(dir, &isThere)) goto error;
    if (isThere) goto error;
    // -
    if (!sprintf(dir, "%s%s", pwdPath, "/tmp/dir2/dir2/")) goto error;
    if (!removeDir(pwdPath, dir)) goto error;
    if (!sprintf(dir, "%s%s", pwdPath, "/tmp/dir2/")) goto error;
    if (!callAccess(dir, &isThere)) goto error;
    if (isThere) goto error;

    // getUnusedPath
    logMain(LOG_NOTICE, "* getUnusedPath");
    if (!sprintf(file, "%s%s", pwdPath, "/tmp/foo")) goto error;
    if (!(fd = fopen(file, "w"))) goto error;
    if ((fclose(fd))) goto error;
    if (!getUnusedPath(file, 3)) goto error;
    logMain(LOG_NOTICE, "tmp/foo => %s", file);
    if (!sprintf(file, "%s%s", pwdPath, "/tmp/foo")) goto error;
    if (unlink(file)) goto error;
    //-
    if (!sprintf(file, "%s%s", pwdPath, "/tmp/bar.txt")) goto error;
    if (!(fd = fopen(file, "w"))) goto error;
    if ((fclose(fd))) goto error;
    if (!getUnusedPath(file, 7)) goto error;
    logMain(LOG_NOTICE, "tmp/bar.txt => %s", file);
    if (!sprintf(file, "%s%s", pwdPath, "/tmp/bar.txt")) goto error;
    if (unlink(file)) goto error;

    // buildAbsoluteTargetPath
    logMain(LOG_NOTICE, "* buildAbsoluteTargetPath");
    if (!sprintf(file, "%s%s", pwdPath, "/tmp/foo/")) goto error;
    if (!makeDir(pwdPath, file, 0777)) goto error;
    if (!sprintf(post, "%s", "tmp/foo")) goto error;
    if (!buildAbsoluteTargetPath(&output, pwdPath, post)) goto error;
    logMain(LOG_NOTICE, "tmp/foo => %s", output);
    output = destroyString(output);
    if (!sprintf(file, "%s%s", pwdPath, "/tmp/foo_00")) goto error;
    if (!(fd = fopen(file, "w"))) goto error;
    if ((fclose(fd))) goto error;
    //-
    if (!sprintf(post, "%s", "tmp/foo/bar.txt")) goto error;
    if (!buildAbsoluteTargetPath(&output, pwdPath, post)) goto error;
    logMain(LOG_NOTICE, "tmp/foo/bar.txt => %s", output);
    output = destroyString(output);
    if (!sprintf(file, "%s%s", pwdPath, "/tmp/foo/bar.txt")) 
      goto error;
    if (!(fd = fopen(file, "w"))) goto error;
    if ((fclose(fd))) goto error;
    //-
    if (!buildAbsoluteTargetPath(&output, pwdPath, post)) goto error;
    logMain(LOG_NOTICE, "tmp/foo/bar.txt => %s", output);
    output = destroyString(output);
    
    // cleaning
    logMain(LOG_NOTICE, "* cleaning");
    if (!sprintf(file, "%s%s", pwdPath, "/tmp/foo/bar.txt")) goto error;
    if (unlink(file)) goto error;
    if (!sprintf(file, "%s%s", pwdPath, "/tmp/foo_00")) goto error;
    if (unlink(file)) goto error;
    if (!sprintf(dir, "%s%s", pwdPath, "/tmp/foo/")) goto error;
    if (!removeDir(pwdPath, dir)) goto error;
  }
  else {
     // Tests using more arguments 
    logMain(LOG_NOTICE, "* Perm: %s %s %s %o (noRegression=%i)", 
	    inputPath, user, group , mode, env.noRegression);
    logMain(LOG_NOTICE, "***********************************************"); 
    if (user == 0) {
      usage(programName);
      logMain(LOG_ERR, "Please provide a user");
      goto error;
    }
    
    if (group == 0) {
      usage(programName);
      logMain(LOG_ERR, "Please provide a group");
      goto error;
    }

    if (!checkDirectoryPerm(inputPath, user, group, mode)) goto error;
  }
  /************************************************************************/

  rc = TRUE;
 error:
  if (inputPath) free(inputPath);
  if (strcmp(pwdPath, ".")) free(pwdPath);
  if (user) free(user);
  if (group) free(group);
  env.noRegression = TRUE;
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
