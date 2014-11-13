/*=======================================================================
 * Version: $Id: setuid.c,v 1.2 2014/11/13 16:36:44 nroche Exp $
 * Project: MediaTeX
 * Module : command
 *
 * setuid API
 *
 * Note: setuid do not works with threads.

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Nicolas Roche

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

#include "log.h"
#include "command.h"
#include "setuid.h"

/*=======================================================================
 * Function   : getPasswdLine
 * Description: find the /etc/password line corresponding to a user
 * Synopsis   : int getPasswdLine (char* label, uid_t uid,
 *                                 struct passwd* pw, char** buffer)
 * Input      : char *label: the username to match
 *              uid_t uid: the uid to match, if username is NULL
 * Output     : struct passwd** pw: the password structure that match 
 *              char** buffer: the buffer used by pointers in pw
 *              FALSE on error or if not found
 =======================================================================*/
int 
getPasswdLine (char* label, uid_t uid, struct passwd* pw, char** buffer)
{
  struct passwd *result = (struct passwd *)0;
  size_t bufsize = 0;
  int err = 0;

#ifdef utMAIN  
  logEmit (LOG_DEBUG, "%s", "find an /etc/passwd line");
#endif

  // allocate buffer
  *buffer = NULL;
  if ((bufsize = sysconf(_SC_GETPW_R_SIZE_MAX)) == -1) {
    bufsize = 16384;        // Should be more than enough
  }
  if ((*buffer = malloc(bufsize)) == NULL) {
    logEmit (LOG_ERR, "cannot malloc: %s", strerror (errno));
    goto error;
  }

  // use the uid if no label is provided
  if (label == NULL || *label == (char)0) {
    if ((err = getpwuid_r(uid, pw, *buffer, bufsize, &result))) {
	logEmit (LOG_ERR, "getpwuid_r fails: %s", strerror (err));
	goto error;
      }
  }
  else {
    if ((err = getpwnam_r(label, pw, *buffer, bufsize, &result))) {
      logEmit (LOG_ERR, "getpwnam_r fails: %s", strerror (err));
      goto error;
    }
  }

  if (result == NULL) {
    logEmit (LOG_WARNING, "%s/%i user not found", label, uid);
    goto error;
  }
  
  return TRUE;
 error:
  if (*buffer) free (*buffer);
  *buffer = NULL;
  return FALSE;
}
  
/*=======================================================================
 * Function   : getGroupLine
 * Description: find the /etc/group line corresponding to a user
 * Synopsis   : int getGroupLine (char* label, git_t gid,
 *                                struct groupe* gr, char** buffer)
 * Input      : char *label: the group name to match
 *              gid_t gid: the gid to match, if group is NULL
 * Output     : struct groupe* gr: the groupe structure that match 
 *              char* buffer: the buffer used by pointers in gr
 *            : 0 on error or if not found
 =======================================================================*/ 
int 
getGroupLine (char* label, gid_t gid, struct group* gr, char** buffer)
{
  struct group *result = (struct group*)0;
  size_t bufsize = 0;
  int s;

#ifdef utMAIN
  logEmit (LOG_DEBUG, "%s", "find an /etc/group line");
#endif

  // allocate buffer
  *buffer = NULL;
  if ((bufsize = sysconf(_SC_GETGR_R_SIZE_MAX)) == -1) {
    bufsize = 16384;        /* I do not know what to put here ? */
  }
  if ((*buffer = malloc(bufsize)) == NULL) {
    logEmit (LOG_ERR, "cannot malloc: %s", strerror (errno));
    goto error;
  }

  // use the uid if no label is provided
  if (label == NULL || *label == (char)0) {
    s = getgrgid_r(gid, gr, *buffer, bufsize, &result);    
  }
  else {
    s = getgrnam_r(label, gr, *buffer, bufsize, &result);
  }
  if (result == NULL) {
    if (s == 0) {
      logEmit (LOG_WARNING, "group not found: %s", label);
    }
    else {
      logEmit (LOG_ERR, "getgrnam_r error: %s", strerror (s));
    }
    goto error;
  }
   
  return TRUE;
 error:
  free (*buffer);
  *buffer = NULL;
  return FALSE;
}

/*=======================================================================
 * Function   : undo_seteuid
 * Description: Set the effective UID to the real UID :
 * Synopsis   : int undo_seteuid (void)
 * Input      : N/A
 * Output     : TRUE on success
 * Note       :
 * This function must be called first and as soon as possible 
 * in order to forgives the setuid bit priviledge.
 =======================================================================*/
int
undo_seteuid (void)
{
  int rc = FALSE;
  int uid = getuid();

  logEmit (LOG_DEBUG, "%s", "hide euid");

  // we do not return on error for unit tests
  if (geteuid() != 0) {
    logEmit (LOG_ERR, "%s", 
	     "binary not owned by root or setuid bit not set");
    goto error;
  } 

#ifdef utMAIN
  logEmit (LOG_DEBUG, "= ruid=%i euid=%i", getuid (), geteuid ());     
#endif

  if ((rc = setresuid (-1, uid, 0))) {
    logEmit (LOG_ERR, "setrsuid fails: %s", strerror (errno));
    goto error;
  }
  
  rc = TRUE;
 error:
#ifdef utMAIN
  logEmit (LOG_DEBUG, "> ruid=%i euid=%i", getuid (), geteuid ());
#endif
  if (!rc) {
    logEmit (LOG_ERR, "%s", "fails to hide eid");
  }
  return rc;
}

/*=======================================================================
 * Function   : allowedUser
 * Description: int allowedUser (char* label)
 * Synopsis   : Check if current user is allowed to change to 
 *              the label user
 * Input      : char* label: the username to become
 * Output     : TRUE on success
 =======================================================================*/
int
allowedUser (char* label)
{
  int rc = FALSE;
  struct passwd pw;
  struct group gr;
  char *buf1 = NULL;
  char *buf2 = NULL;
  int belongs = 0;

  if (!label) goto error;
  logEmit (LOG_DEBUG, "check permissions to become %s user", label);

#ifndef utMAIN
  if (env.noRegression) return TRUE;
#endif

  if (
#ifdef utMAIN
      strncmp(label, "ut-", 3) &&
      strncmp(label+3, env.confLabel, strlen(env.confLabel))
#else 
      strncmp(label, env.confLabel, strlen(env.confLabel))
#endif
      ) {
    logEmit (LOG_ERR, "the %s user account is not manage by mediatex", 
	     label);
    goto error;
  }

  // get current user name
  if (!getPasswdLine (NULL, getuid(), &pw, &buf1)) goto error;
  logEmit (LOG_INFO, "you are %s", pw.pw_name);

  // if current user is already label
  if (strlen(label) == strlen(pw.pw_name) &&
      !strcmp(label, pw.pw_name)) goto nothingToDo;

  // if current user is root
  if (pw.pw_uid == 0) goto nothingToDo;

  // if current user is mdtx 
  if (strlen(env.confLabel) == strlen(pw.pw_name) &&
      !strcmp(env.confLabel, pw.pw_name)) goto nothingToDo;

  // check if current user belongs to the mdtx group
  if (!getGroupLine (env.confLabel, -1, &gr, &buf2)) goto error;
  while (*(gr.gr_mem) != NULL) {
    if (!strcmp(*(gr.gr_mem), pw.pw_name)) {
      belongs=1;
      break;
    }
    ++(gr.gr_mem);
  }
  logEmit (LOG_INFO, "you do%s belongs to the %s group", 
	   belongs?"":" NOT", env.confLabel);
  if (!belongs) goto error; 

 nothingToDo:
  rc = TRUE;
 error:
  if (buf1) free (buf1);
  if (buf2) free (buf2);
  if (!rc) {
    logEmit (LOG_ERR, "%s", "allowedUser fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : becomeUser
 * Description: int becomeUser (char* label, int doCheck)
 * Synopsis   : Change to label user
 * Input      : char* label: the username to become
 *              int check: check if current user belongs to the new user 
 *                         group
 * Output     : TRUE on success
 * Note       : This function should not be call twice.
 *              I mean, we should call logoutUser before a new call.
 =======================================================================*/
int
becomeUser (char* label, int doCheck)
{
  int rc = FALSE;
  struct passwd pw;
  struct group gr;
  char *buf1 = NULL;
  char *buf2 = NULL;

  if (!label) goto error;
  logEmit (LOG_DEBUG, "becomeUser %s", label);
#ifndef utMAIN
  if (env.noRegression) return TRUE;
#endif
  if (doCheck && !allowedUser (label)) goto error;

  // get current user name
  if (!getPasswdLine (NULL, getuid(), &pw, &buf1)) goto error;
  if (!strcmp(label, pw.pw_name)) goto nothingToDo;
  free (buf1);
  buf1 = NULL;

  // get new user name and group labels
  if (!getGroupLine (label, -1, &gr, &buf2)) goto error;
  if (!getPasswdLine (label, -1, &pw, &buf1)) goto error;

  // get privileges  
  if ((rc = setresuid (0, 0, -1))) {
    logEmit (LOG_ERR, "setresuid fails: %s", strerror (errno));
    goto error;
  }
  
  // change the group list
  // when root became mdtx, I get open /dev/cdrom: Permission denied
  // => need to call initgroups("mdtx", 0) as root before setuid
  // to give group indirect access (like cdrom).
  if (initgroups(label, gr.gr_gid) == -1) {
    logEmit (LOG_ERR, "fails to reset groups list: %s", strerror (errno));
    goto error;
  }

  // change group
  if ((rc = setgid (gr.gr_gid))) {
    logEmit (LOG_ERR, "setgid fails: %s", strerror (errno));
    goto error;
  }

  // export the HOME environment variable
  if (setenv("HOME", pw.pw_dir, 1) == -1) {
    logEmit(LOG_ERR, "setenv home variable failed: ", strerror(errno));
    goto error;
  }
  
  // change to label user
  if ((rc = setresuid (pw.pw_uid, pw.pw_uid, 0))) {
    logEmit (LOG_ERR, "setrsuid fails: %s", strerror (errno));
    goto error;
  }

  logEmit (LOG_INFO, "current user is %s", label);
 nothingToDo:
  rc = TRUE;
 error:
  if (buf1) free (buf1);
  if (buf2) free (buf2);
  if (!rc) {
    logEmit (LOG_ERR, "%s", "becomeUser fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : logoutUser
 * Description: int logoutUser (void)
 * Synopsis   : Change back normal user
 * Input      : N/A
 * Output     : TRUE on success
 * Note       : It seems to me that this function may be call twice.
 *              I mean, no mather if we de not change user before.
 =======================================================================*/
int
logoutUser (int uid)
{
  int rc = FALSE;
  struct passwd pw;
  char* buf = NULL;

  //if (uid == 0) goto error; // needed by setConcurentAccessLock
  logEmit (LOG_DEBUG, "logoutUser %i", uid);
#ifndef utMAIN
  if (env.noRegression) return TRUE;
#endif

  if (uid == getuid()) goto nothingToDo;

  // get current user name
  if (!getPasswdLine (NULL, uid, &pw, &buf)) goto error;
  if (!becomeUser (pw.pw_name, FALSE)) goto error;

 nothingToDo:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit (LOG_ERR, "%s", "logoutUser fails");
  }
  if (buf) free (buf);
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include <pthread.h>
#include "command.h"
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
  char* inputFile = NULL;
  int i;
  char *argvExec[] = { NULL, "parameter1", NULL};
  int uid = getuid();
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"i:u:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"sudo-user", required_argument, NULL, 'u'},
    {"input-file", required_argument, NULL, 'i'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv (&env);

  // parse the command line
  while ((cOption = getopt_long (argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch (cOption) {
      
    case 'i':
      if (optarg == NULL || *optarg == (char)0) {
	fprintf (stderr, 
		 "%s: nil or empty argument for the input stream\n", 
		 programName);
	rc = EINVAL;
      }
      inputFile = optarg;
      break;

    case 'u':
      if (optarg == NULL || *optarg == (char)0) {
	fprintf (stderr, "%s: nil or empty argument for the user name\n", 
		programName);
	rc = EINVAL;
      }
      env.confLabel = optarg;
      break;
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv (programName, &env)) goto optError;

  /************************************************************************/
  if (env.confLabel == NULL) {
    usage (programName);
    logEmit (LOG_ERR, "%s", "Please provide a user to become");
    goto error;
  }
  if (inputFile == NULL) {
    usage (programName);
    logEmit (LOG_ERR, "%s", "Please provide an input file");
    goto error;
  }
  argvExec[0] = inputFile;

  // must be done first
  if (!undo_seteuid ()) goto error;

  // first layer
  logEmit (LOG_NOTICE, "%s", "** first layer");
  for (i=0; i<2; ++i) {

    // get privileges
    if ((rc = setresuid (0, 0, -1))) {
      logEmit (LOG_ERR, "setresuid fails: %s", strerror (errno));
      goto error;
    }

    logEmit (LOG_INFO, "> ruid=%i euid=%i", getuid (), geteuid ());

    // change to label user
    if ((rc = setresuid (uid, uid, 0))) {
      logEmit (LOG_ERR, "setrsuid fails: %s", strerror (errno));
      goto error;
    }

    logEmit (LOG_INFO, "< ruid=%i euid=%i", getuid (), geteuid ());
  }

  // API for wrapper
  logEmit (LOG_NOTICE, "%s", "** API for wrapper");
  uid = getuid();
  for (i=0; i<2; ++i) {
    if (!(rc = execScript (argvExec, NULL, NULL, FALSE))) goto error;
    if (!becomeUser(env.confLabel, TRUE)) goto error;
    if (!(execScript (argvExec, NULL, NULL, FALSE))) goto error;
    if (!logoutUser(uid)) goto error;
  }

  // API for thread
  logEmit (LOG_NOTICE, "%s", "** API for thread");
  for (i=0; i<2; ++i) {
    if (!execScript(argvExec, env.confLabel, NULL, FALSE)) goto error;
    if (!execScript(argvExec, NULL, NULL, FALSE)) goto error;
  }
  /************************************************************************/

  rc = TRUE;
 error:
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

