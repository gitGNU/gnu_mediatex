/*=======================================================================
 * Version: $Id: locks.c,v 1.4 2015/06/30 17:37:32 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
 *
 * locks on files

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
#include <fcntl.h> // fcntl


/*=======================================================================
 * Function   : lock
 * Description: lock file descriptor
 * Synopsis   : int lock(int fd, int mode)
 * Input      : fd: file descriptor to lock
 *              mode: F_RDLCK or F_WRLCK
 * Output     : TRUE on success
 =======================================================================*/
int lock(int fd, int mode)
{
  int rc = FALSE;
  struct flock lock;
  
  logEmit(LOG_DEBUG, "lock file for %s",
	  mode == F_RDLCK?"read":
	  mode == F_WRLCK?"write":"??");

  if (fd == -1) {
    logEmit(LOG_ERR, "%s", "please provide a file descriptor to lock");
    goto error;
  }

  if (mode != F_RDLCK && mode != F_WRLCK) {
    logEmit(LOG_ERR, "please provide %i or %i mode, not %i",
	    F_RDLCK, F_WRLCK, mode);
    goto error;
  }

  lock.l_type = mode;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  if (fcntl(fd, F_SETLK, &lock)) {
    logEmit(LOG_ERR, "fcntl(F_SETLK) lock fails: %s", strerror(errno));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "lock fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : unLock
 * Description: unlock file descriptor
 * Synopsis   : int unLock(int fd)
 * Input      : fd: file descriptor to unlock
 * Output     : TRUE on success
 =======================================================================*/
int unLock(int fd)
{
  int rc = FALSE;
  struct flock lock;

  logEmit(LOG_DEBUG, "%s", "unlock file");
  
  if (fd == -1) {
    logEmit(LOG_ERR, "%s", "please provide a file descriptor to lock");
    goto error;
  }

  lock.l_type = F_UNLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  if (fcntl(fd, F_SETLK, &lock)) {
    logEmit(LOG_ERR, "fcntl(F_SETLK) unlock fails: %s", strerror(errno));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "lock fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
