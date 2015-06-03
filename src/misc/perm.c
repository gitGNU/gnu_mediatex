/*=======================================================================
 * Version: $Id: perm.c,v 1.3 2015/06/03 14:03:46 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
 *
 * md5sum computation

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

#include "command.h"
#include "setuid.h"
#include "perm.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* #include <dirent.h> // opendir */

/*  */
/* /\*======================================================================= */
/*  * Function   : buildDirectory */
/*  * Description: Try to build a directory if it doesn't exist. */
/*  * Synopsis   : static int buildDirectory(char* path, mode_t mode)) */
/*  * Input      : const char* path : the path of the directory */
/*  *              mode_t mode: permission mode on the new directory */
/*  * Output     : TRUE on success */
/*  =======================================================================*\/ */
/* static int  */
/* buildDirectory(char* path, mode_t mode) */
/* { */
/*   int rc = FALSE; */
/*   DIR* dir  = 0; */
/*   size_t i=1; // shift the leading slash */
/*   char* copy = 0; */

/*   logEmit(LOG_DEBUG, "buildDirectory %s", path); */
  
/*   if (path == 0 || *path == (char)0) { */
/*     logEmit(LOG_ERR, "%s", "cannot check empty directory path"); */
/*     goto error; */
/*   } */

/*   // needed if we have path in static memory */
/*   if ((copy = malloc(strlen(path))) == 0) { */
/*     logEmit(LOG_ERR, "malloc: %s", strerror(errno)); */
/*     goto error; */
/*   } */
/*   strncpy(copy, path, strlen(path)); */

/*   // loop on each parent directory */
/*   do { */
/*     // troncate string to next parent's dirname  */
/*     printf("copy1 = %s\n", copy); */

/*     while (copy[i] != (char)0 && copy[i] != '/') ++i; */
/*     if (copy[i] == (char)0) break; // ignore ending copy file */
/*     copy[i] = (char)0; */
      
/*     printf("copy2 = %s\n", copy); */

/*     // try to open dir */
/*     while ((dir = opendir(copy)) == 0) { */
/*       // try to create it */
/*       logEmit(LOG_NOTICE, "create new directory: %s", copy);  */
/*       if (mkdir(copy, mode) != 0) { */
/* 	logEmit(LOG_ERR, "cannot create this directory: %s",  */
/* 		strerror(errno)); */
/* 	logEmit(LOG_ERR, "%s", "Please, create it by hand"); */
/* 	goto error; */
/*       } */
/*     } */
/*     if (dir != 0 && closedir(dir) != 0) { */
/*       logEmit(LOG_ERR, "cannot close directory: %s", strerror(errno));  */
/*       goto error; */
/*     } */

/*     // restore the original string */
/*     copy[i] = '/'; */
/*     ++i; */
/*   } */
/*   while (i<strlen(copy)); */
  
/*   rc = TRUE; */
/*  error: */
/*   if (!rc) { */
/*     logEmit(LOG_ERR, "%s", "build directory fails"); */
/*   } */
/*   free (copy); */
/*   return rc; */
/* } */


/*=======================================================================
 * Function   : checkDirectory
 * Description: check existence, owner and permission of a directory.
 * Synopsis   : static int checkDirectory(char* path, 
 *                         char* user, char* group, mode_t mode)
 * Input      : const char* path : the path of the directory
 *              char* user:  expected user
 *              char* group: expected group
 *              mode_t mode  expected mode
 * Output     : TRUE on success
 =======================================================================*/
int 
checkDirectory(char* path, char* user, char* group, mode_t mode)
{
  int rc = FALSE;
  struct stat sb;
  struct passwd pw;
  struct group gr;
  char *buf = 0;
  mode_t mask = 07777;

  if (path == 0 || *path == (char)0) {
    logEmit(LOG_ERR, "%s", "cannot check empty directory path");
    goto error;
  }

  logEmit(LOG_DEBUG, "checkDirectory %s %s %s %lo", 
	  path, user, group, mode);

  if (stat(path, &sb) == -1) {
    logEmit(LOG_ERR, "stat fails on path %s", path);
    logEmit(LOG_ERR, "stat: %s", strerror(errno));
    goto error;
  }
  
  // check permissions
  if (env.noRegression) mask = 00777;
  if ((sb.st_mode & mask) != (mode & mask)) {
    logEmit(LOG_ERR, "%s should be share as %lo, not %lo",
	    path, mode & mask, sb.st_mode & mask);
    goto error;
  }

  // we bypass owner checks if we are running a unit test
  if (env.noRegression) {
    rc = TRUE;
    goto error;
  }

  // check user
  if (!getPasswdLine (0, sb.st_uid, &pw, &buf)) goto error;
  if (strcmp(user, pw.pw_name) != 0) {
    logEmit(LOG_ERR, "%s should be owne by %s user, not %s",
	    path, user, pw.pw_name);
    goto error;
  }
  
  // check group  
  if (buf) free (buf);
  if (!getGroupLine (0, sb.st_gid, &gr, &buf)) goto error;
  if (strcmp(group, gr.gr_name) != 0) {
    logEmit(LOG_ERR, "%s should be owne by %s group, not %s",
	    path, group, gr.gr_name);
    goto error;
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "checkDirectory fails");
  } 

  if (buf) free (buf);
  return rc;
}


/*=======================================================================
 * Function   : currentTime
 * Description: get the current date
 * Synopsis   : time_t currentTime() 
 * Input      : N/A
 * Output     : current time or -1 on FAILURE
 * Note       :
 * for unit tests (nv.noRegression = TRUE) the current date 
 * is alway 2010-01-01 00:00:00 +00:00
 =======================================================================*/
time_t
currentTime() 
{
  time_t rc = -1;

  // current date
  if (env.noRegression) {
    // unit test simulate that we are alway the same day
    rc = 1262304000; // $ date -d "2010-01-01 00:00:00 +00:00" // +"%s"
    goto end;
  }

  if (time(&rc) == -1) {
    logEmit(LOG_ERR, "time error: %s", strerror(errno));
  }
 end:
 return rc;
}

/************************************************************************/

#ifdef utMAIN
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
      if (sscanf(optarg, "%lo", (long unsigned int *)&mode) != 1) {
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
  logEmit(LOG_NOTICE, "unit test's current date : %d", currentTime());

  if (inputPath == 0) {
    usage(programName);
    logEmit(LOG_ERR, "%s", "Please provide a directory to check");
    goto error;
  }

  if (user == 0) {
    usage(programName);
    logEmit(LOG_ERR, "%s", "Please provide a user");
    goto error;
  }

  if (group == 0) {
    usage(programName);
    logEmit(LOG_ERR, "%s", "Please provide a group");
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

#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
