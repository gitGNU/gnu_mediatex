/*=======================================================================
 * Version: $Id: perm.c,v 1.7 2015/08/07 17:50:32 nroche Exp $
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

#include "mediatex-config.h"


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
    logMisc(LOG_ERR, "cannot check empty directory path");
    goto error;
  }

  logMisc(LOG_DEBUG, "checkDirectory %s %s %s %o", 
	  path, user, group, mode);

  if (stat(path, &sb) == -1) {
    logMisc(LOG_ERR, "stat fails on path %s", path);
    logMisc(LOG_ERR, "stat: %s", strerror(errno));
    goto error;
  }
  
  // check permissions
  if (env.noRegression) mask = 00777;
  if ((sb.st_mode & mask) != (mode & mask)) {
    logMisc(LOG_ERR, "%s should be share as %o, not %o",
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
    logMisc(LOG_ERR, "%s should be owne by %s user, not %s",
	    path, user, pw.pw_name);
    goto error;
  }
  
  // check group  
  if (buf) free (buf);
  if (!getGroupLine (0, sb.st_gid, &gr, &buf)) goto error;
  if (strcmp(group, gr.gr_name) != 0) {
    logMisc(LOG_ERR, "%s should be owne by %s group, not %s",
	    path, group, gr.gr_name);
    goto error;
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "checkDirectory fails");
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
    logMisc(LOG_ERR, "time error: %s", strerror(errno));
  }
 end:
 return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
