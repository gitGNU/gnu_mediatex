/*=======================================================================
 * Version: $Id: setuid.c,v 1.7 2015/08/13 21:14:36 nroche Exp $
 * Project: MediaTeX
 * Module : command
 *
 * setuid API
 *
 * Note: setuid do not works with threads.

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

#include "mediatex-config.h"

/*=======================================================================
 * Function   : getPasswdLine
 * Description: find the /etc/password line corresponding to a user
 * Synopsis   : int getPasswdLine (char* label, uid_t uid,
 *                                 struct passwd* pw, char** buffer)
 * Input      : char *label: the username to match
 *              uid_t uid: the uid to match, if username is 0
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

  logMisc (LOG_DEBUG, "find an /etc/passwd line");

  // allocate buffer
  *buffer = 0;
  if ((bufsize = sysconf(_SC_GETPW_R_SIZE_MAX)) == -1) {
    bufsize = 16384;        // Should be more than enough
  }
  if ((*buffer = malloc(bufsize)) == 0) {
    logMisc (LOG_ERR, "cannot malloc: %s", strerror (errno));
    goto error;
  }

  // use the uid if no label is provided
  if (label == 0 || *label == (char)0) {
    if ((err = getpwuid_r(uid, pw, *buffer, bufsize, &result))) {
	logMisc (LOG_ERR, "getpwuid_r fails: %s", strerror (err));
	goto error;
      }
  }
  else {
    if ((err = getpwnam_r(label, pw, *buffer, bufsize, &result))) {
      logMisc (LOG_ERR, "getpwnam_r fails: %s", strerror (err));
      goto error;
    }
  }

  if (result == 0) {
    logMisc (LOG_WARNING, "%s/%i user not found", label, uid);
    goto error;
  }
  
  return TRUE;
 error:
  if (*buffer) free (*buffer);
  *buffer = 0;
  return FALSE;
}
  
/*=======================================================================
 * Function   : getGroupLine
 * Description: find the /etc/group line corresponding to a user
 * Synopsis   : int getGroupLine (char* label, git_t gid,
 *                                struct groupe* gr, char** buffer)
 * Input      : char *label: the group name to match
 *              gid_t gid: the gid to match, if group is 0
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

  logMisc (LOG_DEBUG, "find an /etc/group line");

  // allocate buffer
  *buffer = 0;
  if ((bufsize = sysconf(_SC_GETGR_R_SIZE_MAX)) == -1) {
    bufsize = 16384;        /* I do not know what to put here ? */
  }
  if ((*buffer = malloc(bufsize)) == 0) {
    logMisc (LOG_ERR, "cannot malloc: %s", strerror (errno));
    goto error;
  }

  // use the uid if no label is provided
  if (label == 0 || *label == (char)0) {
    s = getgrgid_r(gid, gr, *buffer, bufsize, &result);    
  }
  else {
    s = getgrnam_r(label, gr, *buffer, bufsize, &result);
  }
  if (result == 0) {
    if (s == 0) {
      logMisc (LOG_WARNING, "group not found: %s", label);
    }
    else {
      logMisc (LOG_ERR, "getgrnam_r error: %s", strerror (s));
    }
    goto error;
  }
   
  return TRUE;
 error:
  free (*buffer);
  *buffer = 0;
  return FALSE;
}

/*=======================================================================
 * Function   : undoSeteuid
 * Description: Set the effective UID to the real UID :
 * Synopsis   : int undoSeteuid (void)
 * Input      : N/A
 * Output     : TRUE on success
 * Note       :
 * This function must be called first and as soon as possible 
 * in order to forgives the setuid bit priviledge.
 =======================================================================*/
int
undoSeteuid (void)
{
  int rc = FALSE;
  int uid = getuid();

  logMisc (LOG_DEBUG, "hide euid");

  // we do not return on error for unit tests
  if (geteuid()) {
    logMisc (LOG_ERR, 
	     "Was expecting root or setuid bit set");
    goto error;
  } 

  //logMisc (LOG_DEBUG, "= ruid=%i euid=%i", getuid (), geteuid ());     

  if ((rc = setresuid (-1, uid, 0))) {
    logMisc (LOG_ERR, "setrsuid fails: %s", strerror (errno));
    goto error;
  }
  
  rc = TRUE;
 error:
  //logMisc (LOG_DEBUG, "> ruid=%i euid=%i", getuid (), geteuid ());
  if (!rc) {
    logMisc (LOG_ERR, "fails to hide eid");
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
  char *buf1 = 0;
  char *buf2 = 0;
  int belongs = 0;

  if (!label) goto error;
  logMisc (LOG_DEBUG, "check permissions to become %s user", label);

  if (env.noRegression) return TRUE;

  if (strncmp(label, env.confLabel, strlen(env.confLabel))) {
    logMisc (LOG_ERR, "the %s user account is not manage by mediatex", 
	     label);
    goto error;
  }

  // get current user name
  if (!getPasswdLine (0, getuid(), &pw, &buf1)) goto error;
  logMisc (LOG_DEBUG, "you are %s", pw.pw_name);

  // if current user is already label
  if (strlen(label) == strlen(pw.pw_name) &&
      !strcmp(label, pw.pw_name)) goto ok;

  // if current user is root
  if (pw.pw_uid == 0) goto ok;

  // if current user is mdtx 
  if (strlen(env.confLabel) == strlen(pw.pw_name) &&
      !strcmp(env.confLabel, pw.pw_name)) goto ok;

  // check if current user belongs to the mdtx group
  if (!getGroupLine (env.confLabel, -1, &gr, &buf2)) goto error;
  while (*(gr.gr_mem)) {
    if (!strcmp(*(gr.gr_mem), pw.pw_name)) {
      belongs=1;
      break;
    }
    ++(gr.gr_mem);
  }

  logMisc (LOG_DEBUG, "you do%s belongs to the %s group", 
	   belongs?"":" NOT", env.confLabel);

  if (!belongs) {
    logMisc (LOG_ERR, "%s user not alowed do switch to user %s",
	     pw.pw_name, label);
    goto error; 
  }
    
 ok:
  logMisc (LOG_INFO, "%s user allowed to switch to user %s",
	   pw.pw_name, label);
  rc = TRUE;
 error:
  if (buf1) free (buf1);
  if (buf2) free (buf2);
  if (!rc) {
    logMisc (LOG_ERR, "allowedUser fails");
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
  char *buf1 = 0;
  char *buf2 = 0;

  if (!label) goto error;
  logMisc (LOG_DEBUG, "becomeUser %s", label);

  if (env.noRegression) return TRUE;
  if (doCheck && !allowedUser (label)) goto error;

  // get current user name
  if (!getPasswdLine (0, getuid(), &pw, &buf1)) goto error;
  if (!strcmp(label, pw.pw_name)) goto ok;
  free (buf1);
  buf1 = 0;

  // get new user name and group labels
  if (!getGroupLine (label, -1, &gr, &buf2)) goto error;
  if (!getPasswdLine (label, -1, &pw, &buf1)) goto error;

  // get privileges  
  if ((rc = setresuid (0, 0, -1))) {
    logMisc (LOG_ERR, "setresuid fails: %s", strerror (errno));
    goto error;
  }
  
  // change the group list
  // when root became mdtx, I get open /dev/cdrom: Permission denied
  // => need to call initgroups("mdtx", 0) as root before setuid
  // to give group indirect access (like cdrom).
  if (initgroups(label, gr.gr_gid) == -1) {
    logMisc (LOG_ERR, "fails to reset groups list: %s", strerror (errno));
    goto error;
  }

  // change group
  if ((rc = setgid (gr.gr_gid))) {
    logMisc (LOG_ERR, "setgid fails: %s", strerror (errno));
    goto error;
  }

  // export the HOME environment variable
  if (setenv("HOME", pw.pw_dir, 1) == -1) {
    logMisc(LOG_ERR, "setenv home variable failed: ", strerror(errno));
    goto error;
  }
  
  // change to label user
  if ((rc = setresuid (pw.pw_uid, pw.pw_uid, 0))) {
    logMisc (LOG_ERR, "setrsuid fails: %s", strerror (errno));
    goto error;
  }

  logMisc (LOG_INFO, "have switched to %s user", label);
 ok:
  rc = TRUE;
 error:
  if (buf1) free (buf1);
  if (buf2) free (buf2);
  if (!rc) {
    logMisc (LOG_ERR, "becomeUser fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : logoutUser
 * Description: Change back to previous user (wrapper for becomeUser)
 * Synopsis   : int logoutUser (int uid)
 * Input      : int uid : uid before last user context switch
 * Output     : TRUE on success
 * Note       : This function trust on iud provided as parameter.
 =======================================================================*/
int
logoutUser (int uid)
{
  int rc = FALSE;
  struct passwd pw;
  char* buf = 0;

  //if (uid == 0) goto error; // needed by setConcurentAccessLock
  logMisc (LOG_DEBUG, "logoutUser %i", uid);

  if (env.noRegression) return TRUE;

  // nothing to do, maybe already done
  if (uid == getuid()) goto ok;

  // get current user name
  if (!getPasswdLine (0, uid, &pw, &buf)) goto error;
  if (!becomeUser (pw.pw_name, FALSE)) goto error;

 ok:
  rc = TRUE;
 error:
  if (!rc) {
    logMisc (LOG_ERR, "logoutUser fails");
  }
  if (buf) free (buf);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

