/*=======================================================================
 * Project: MediaTeX
 * Module : cvs print
 *
 * cvs files producer interface

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
 * Function   : cvsClose
 * Description: Cut a big file into several ones
 * Synopsis   : int cvsClose(CvsFile* fd)
 * Input      : CvsFile* fd
 * Output     : TRUE on success
 =======================================================================*/
int cvsClose(CvsFile* fd)
{
  int rc = FALSE;

  if (!fd) goto error;
  logMemory(LOG_DEBUG, "cvsClose %s", fd->path);
  if (!(fd->fd)) goto end;

  fprintf(fd->fd, "\n# Local Variables:\n"
	  "# mode: conf\n"
	  "# mode: font-lock\n"
	  "# End:\n");

  if (fd->fd == stdout) {
    fflush(stdout);
    goto end;
  }

  if (!unLock(fileno(fd->fd))) rc = FALSE;
  if (fclose(fd->fd)) {
    logMemory(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error;
  }

  fd->fd = 0;
 end:
  rc = TRUE;
error:
  if (!rc) {
    logMemory(LOG_ERR, "cvsClose fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : cvsCutOpen
 * Description: Cut a big file into several ones
 * Synopsis   : int cvsCutOpen(CvsFile* fd)
 * Input      : CvsFile* fd
 * Output     : TRUE on success
 =======================================================================*/
int cvsCutOpen(CvsFile* fd)
{
  int rc = FALSE;
  char* path = 0;
  int l = 0;
  int i = 0;

  if (!fd) goto error;
  if (isEmptyString(fd->path)) goto error; 
  logMemory(LOG_DEBUG, "cvsCutOpen %s %i", fd->path, fd->nb);

  l = strlen(fd->path);
  if (!(path = createString(fd->path))
      || !(path = catString(path, "000.txt"))) goto error;

  // first call: empty existing files
  if (fd->nb == 0 && fd->fd != stdout) {
    // empty part files
    do {
      if (!sprintf(path+l, "%03i.txt", i)) goto error;
      if (access(path, R_OK)) break;
      if (!env.dryRun) {
	logMemory(LOG_INFO, "empty %s", path);
	if ((fd->fd = fopen(path, "w")) == 0) {
	  logMemory(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno));
	  goto error;
	}
	if (fclose(fd->fd)) {
	  logMemory(LOG_ERR, "fclose fails: %s", strerror(errno));
	  goto error;
	}
	fd->fd = 0;
      }
    }
    while (++i < 1000);
    
    // unlink last addon
    if (!sprintf(path+l, "%s", "NNN.txt")) goto error;
    if (access(path, R_OK) == 0) {
      logMemory(LOG_INFO, "unlink %s", path);
      if (unlink(path) == -1) {
	logMemory(LOG_ERR, "unlink fails %s:", strerror(errno));
	goto error;
      }
    }
  }

  // not first call
  if (fd->nb > 0) {
    if (!cvsClose(fd)) goto error;
  }

  // open file
  if (!sprintf(path+l, "%03i.txt", fd->nb)) goto error;
  logMemory(LOG_INFO, "serialize into %s", path);
  if (fd->fd == stdout) goto end;
  if ((fd->fd = fopen(path, "w")) == 0) {
    logMemory(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno));
    goto error;
  }

  if (!lock(fileno(fd->fd), F_WRLCK)) goto error;
 end:
  ++fd->nb;
  fd->offset = 0;
  rc = TRUE;
error:
  if (!rc) {
    logMemory(LOG_ERR, "cvsCutOpen fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : cvsCatOpen
 * Description: Open metadata file for concatenation
 * Synopsis   : int cvsCatOpen(CvsFile* fd)
 * Input      : CvsFile* fd
 * Output     : TRUE on success
 =======================================================================*/
int cvsCatOpen(CvsFile* fd)
{
  int rc = FALSE;
  char* path = 0;

  if (!fd) goto error;
  if (isEmptyString(fd->path)) goto error; 
  logMemory(LOG_DEBUG, "cvsCatOpen %s %i", fd->path, fd->nb);

  if (!(path = createString(fd->path))
      || !(path = catString(path, "NNN.txt"))) goto error;

  // open file
  logMemory(LOG_INFO, "serialize into %s", path);
  if (fd->fd == stdout) goto end;
  if ((fd->fd = fopen(path, "a")) == 0) {
    logMemory(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno));
    goto error;
  }

  if (!lock(fileno(fd->fd), F_WRLCK)) goto error;
 end:
  ++fd->nb;
  rc = TRUE;
error:
  if (!rc) {
    logMemory(LOG_ERR, "cvsCatOpen fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : cvsCutPrint
 * Description: Cut a big file into several ones
 * Synopsis   : int cvsCutPrint(CvsFile* fd, const char* format, ...)
 * Input      : CvsFile* fd
 *              const char* format : as printf
 *              ... : as printf
 * Output     : TRUE on success
 =======================================================================*/
int cvsCutPrint(CvsFile* fd, const char* format, ...)
{
  int rc = FALSE;
  va_list args;

  if (!fd) goto error;

  if (fd->doCut && fd->offset > env.cvsprintMax) {
    if (!cvsCutOpen(fd)) goto error;
  }

  va_start(args, format);
  fd->offset += vfprintf(fd->fd, format, args);
  va_end(args);
  rc = TRUE;
error:
  if (!rc) {
    logMemory(LOG_ERR, "cvsCutPrint fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : cvsCatPrint
 * Description: fprintf like
 * Synopsis   : int cvsCatPrint(CvsFile* fd, const char* format, ...)
 * Input      : CvsFile* fd
 *              const char* format : as printf
 *              ... : as printf
 * Output     : TRUE on success
 =======================================================================*/
int cvsCatPrint(CvsFile* fd, const char* format, ...)
{
  int rc = FALSE;
  va_list args;

  if (!fd) goto error;

  va_start(args, format);
  vfprintf(fd->fd, format, args);
  va_end(args);
  rc = TRUE;
error:
  if (!rc) {
    logMemory(LOG_ERR, "cvsCatPrint fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

