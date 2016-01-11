/*=======================================================================
 * Version: $Id: perm.c,v 1.9 2015/08/23 23:39:16 nroche Exp $
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

#include "mediatex-config.h"

/*=======================================================================
 * Function   : callAccess
 * Description: check a path accessible (readable file or directory)
 * Synopsis   : int callAcess(char* path, int* isThere)
 * Input      : char* path = the path to check
 * Output     : int* isThere: state if file is readable or not
 *              TRUE on success
 =======================================================================*/
int
callAccess(char* path, int* isThere) 
{
  int rc = FALSE;

  logMain(LOG_DEBUG, "callAccess %s", path);
  checkLabel(path);
  *isThere = FALSE;

  if (!access(path, R_OK)) {
    *isThere = TRUE;
  }
  else {
    if (!errno == ENOENT) {
      logMain(LOG_ERR, "unexpected error code (%i): %s", 
	      errno, strerror(errno));
      goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "callAccess fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : checkDirectory
 * Description: check if a file is a directory
 * Synopsis   : int isDirectory(char* path)
 * Input      : char* path = the path to check
 * Output     : int* isDirectory: state if file is a directory or not
 *              TRUE on success
 =======================================================================*/
int 
checkDirectory(char* path, int *isDirectory)
{
  int rc = FALSE;
  struct stat statBuffer;

  logMain(LOG_DEBUG, "checkDirectory %s", path);
  checkLabel(path);
  
  // get file attributes
  if (lstat(path, &statBuffer)) {
    logMain(LOG_ERR, "status error on %s: %s", path, strerror(errno));
    goto error;
  }
  
  *isDirectory = S_ISDIR(statBuffer.st_mode);
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "checkDirectory fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : makeDir
 * Description: build directories recursively
 * Synopsis   : int makeDir(char* path) 
 * Input      : char* base = directory path already there
 *              char* path = directory path to build (including base)
 * Output     : TRUE on success
 * Note       : path may be a filename so last directory must provide
 *              an ending '/'
 =======================================================================*/
int
makeDir(char* base, char* path, mode_t mode) 
{
  int rc = FALSE;
  int i = 0;
  int l = 0;
  mode_t mask;
  int isThere = FALSE;

  logMain(LOG_DEBUG, "makeDir %s - %s", path, base);
  mask = umask(0000);
  checkLabel(path);

  // build the target directory into the cache
  i = strlen(base)+1;
  l = strlen(path);
  while (i<l) {
    while (i<l && path[i] != '/') ++i;
    if (path[i] == '/') { // this will skip eventual ending file
      path[i] = (char)0;

      if (!mkdir(path, mode)) { // (mode & ~umask & 0777)
	logMain(LOG_INFO, "mkdir %s", path);
      }
      else {
	if (errno != EEXIST) {
	  logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
	  goto error;
	}
	else {
	  // already there (should be better to check mode too)
	  if (!callAccess(path, &isThere)) goto error;
	  if (!isThere) goto error;
	}
      }
      
      path[i] = '/';
       ++i;
    }
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "makeDir fails");
  }
  umask(mask);
  return rc;
}

/*=======================================================================
 * Function   : removeDir
 * Description: remove directories recursively if not empty
 * Synopsis   : int removeDir(char* path) 
 * Input      : char* base = directory path to not remove
 *              char* path = directory path to build (including base)
 * Output     : path is modified: '/' are replaced by '\0'
 *              TRUE on success
 =======================================================================*/
int
removeDir(char* base, char* path)
{
  int rc = FALSE;
  int i = 0;
  int l = 0;
  int lastSlash = 0;

  checkLabel(path);
  logMain(LOG_DEBUG, "removeDir %s - %s", path, base);

  // remove the temporary extraction directory into the cache
  i = strlen(path)-1;
  l = strlen(base);

  // eat filename
  while (i>l && path[i] != '/') --i; 
  if (i <= l) goto end;
  path[lastSlash = i] = (char)0;

  while (--i > l) {
    while (i>l && path[i] != '/') --i;

    if (!rmdir(path)) {
      logMain(LOG_INFO, "rmdir %s", path);
    }
    else {
      if (errno != ENOTEMPTY && errno != EBUSY) {
	logMain(LOG_ERR, "rmdir fails: (%i) %s", errno, strerror(errno));
	goto error;
      }
      else {
	// still used
	logMain(LOG_INFO, "keep not empty directory: %s", path);
	goto end;
      }
    }
    path[lastSlash] = '/';
    path[lastSlash = i] = (char)0;
  }
  
 end:
  path[lastSlash] = '/';
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "removeDir fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getUnusedPath
 * Description: return an unused postfix path
 * Synopsis   : static int getUnusedPath(char output[MAX_SIZE_STRING],
 *                                       char* path, char* filename)
 * Input      : char* path: the whole path
 *              char* filename: the end of the path to change
 * Output     : char* path: updated path
 *              TRUE on success
 =======================================================================*/
int
getUnusedPath(char* path, int lenFilename) 
{
  int rc = FALSE;
  int isAlreadyThere = 0;
  int lenPath = 0;
  int lenExtension = 0;
  char* pFilename = 0;
  char* pExtension = 0;
  char* pNumbers = 0;
  int i = 0;
  int j = 0;

  logMain(LOG_DEBUG, "getUnusedPath %s / %i", path, lenFilename);
  checkLabel(path);
  if ((lenPath = strlen(path)) + 3 > MAX_SIZE_STRING) goto error;

  // build the path with _NN added into (ex logo_00.png)
  pFilename = pExtension = path + lenPath;
  pFilename -= lenFilename;

  // search for an extension
  while (pExtension > pFilename && *pExtension != '.') --pExtension;

  if ((lenExtension = pExtension - pFilename)) {
    // push extension on the right
    for (i = lenExtension + 1; i >=0; --i) {
      pExtension[i+3] = pExtension[i];
    }
    // insert numbers to the path
    pNumbers = pExtension;
    strncpy(pNumbers, "_NN", 3);
  }
  else {
    // simply concatenate
    pNumbers = path + lenPath;
    strcpy(pNumbers, "_NN");
  }

  // loop on numbers while the path is already accessible
  for (i='0'; i<='9'; ++i) {
    pNumbers[1] = i;
    for (j='0'; j<='9'; ++j) {
      pNumbers[2]  = j;
      if (!callAccess(path, &isAlreadyThere)) goto error;
      if (!isAlreadyThere) goto end;
    }
  }
  goto error;
  
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "getUnusedPath fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : buildAbsoluteTargetPath
 * Description: return an absolute path not already used
 * Synopsis   : int buildAbsoluteTargetPath(char** newAbsolutePath, 
 *                                  char* prefix, char* postfix)
 * Input      : char* prefix: directory path we know it is accessible 
 *               (without the leading '/')
 *              char* postfix: path to work on (directory without the 
 *               heading '/') + filename
 *               without the heading '/')
 * Output     : char** newAbsolutePath
 *              TRUE on success
 * Note       : newPath must be lower then MAX_SIZE_STRING*2+2 chars
 =======================================================================*/
int
buildAbsoluteTargetPath(char** newAbsolutePath, char* prefix, char* postfix)
{
  int rc = FALSE;
  int isAlreadyThere = 0;
  int isDirectory = 0;
  char path[MAX_SIZE_STRING*2+2] = ""; // +'/', +'\0'
  char* pOut = path;
  char* pInput1 = 0;
  char* pInput2 = 0;
  int lenInput = 0;
  int lastFileOne = 0;
  int collision = 0;

  logMain(LOG_DEBUG, "buildAbsoluteTargetPath %s / %s", prefix, postfix);

  checkLabel(prefix);
  if (strlen(prefix) > MAX_SIZE_STRING) {
    logMain(LOG_ERR, "prefix is too long");
    goto error;
  }

  checkLabel(postfix);
  if (strlen(postfix) > MAX_SIZE_STRING) {
    logMain(LOG_ERR, "postfix is too long");
    goto error;
  }

  // copy prefix
  strcpy(pOut, prefix);
  pOut = path + strlen(prefix);
  strcpy(pOut++, "/");

  pInput2 = pInput1 = postfix;
  do {++pInput2;} while (*pInput2 && *pInput2 != '/');

  // loop on each directory on postfix to build the output path
  while ((lenInput = pInput2 - pInput1)) {

    // stat if we reach the a filename (not ending by /)
    lastFileOne = (*pInput2 == 0);

    // push next file or directory name
    *pInput2 = 0;
    strcpy(pOut, pInput1);
    if (!lastFileOne) *pInput2 = '/';

    // stat if the provided path can be use or not
    collision = 0;
    if (!callAccess(path, &isAlreadyThere)) goto error;
    if (isAlreadyThere) {
      if (lastFileOne) {
	collision = 1;
      }
      else {
	if (!checkDirectory(path, &isDirectory)) goto error;
	if (!isDirectory) collision = 1;
      }
    }

    // provide a usable outpout path
    if (collision) {
      if (!getUnusedPath(path, lenInput)) goto error;
      pOut += 3; 
    }

    pOut += lenInput;
    pInput1 = pInput2;
    if (!lastFileOne) {
      do {++pInput2;} while (*pInput2 && *pInput2 != '/');
    }
  }

  if (!(*newAbsolutePath = createString(path))) goto error;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "buildAbsoluteTargetPath fails");
    *newAbsolutePath = 0;
  }
  return rc;
}

/*=======================================================================
 * Function   : checkDirectoryPerm
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
checkDirectoryPerm(char* path, char* user, char* group, mode_t mode)
{
  int rc = FALSE;
  struct stat statBuffer;
  struct passwd pw;
  struct group gr;
  char *buf = 0;
  mode_t mask = 07777;

  if (path == 0 || *path == (char)0) {
    logMisc(LOG_ERR, "cannot check empty directory path");
    goto error;
  }

  logMisc(LOG_DEBUG, "checkDirectoryPerm %s %s %s %o", 
	  path, user, group, mode);

  if (stat(path, &statBuffer) == -1) {
    logMisc(LOG_ERR, "stat fails on path %s", path);
    logMisc(LOG_ERR, "stat: %s", strerror(errno));
    goto error;
  }
  
  // check directory
  if (!S_ISDIR(statBuffer.st_mode)) {
    logMisc(LOG_ERR, "%s is not adirectory\n", path)
    goto error;
  }

  // check permissions
  if (env.noRegression) mask = 00777;
  if ((statBuffer.st_mode & mask) != (mode & mask)) {
    logMisc(LOG_ERR, "%s should be share as %o, not %o",
	    path, mode & mask, statBuffer.st_mode & mask);
    goto error;
  }

  // we bypass owner checks if we are running a unit test
  if (env.noRegression) {
    rc = TRUE;
    goto error;
  }

  // check user
  if (!getPasswdLine (0, statBuffer.st_uid, &pw, &buf)) goto error;
  if (strcmp(user, pw.pw_name)) {
    logMisc(LOG_ERR, "%s should be owne by %s user, not %s",
	    path, user, pw.pw_name);
    goto error;
  }
  
  // check group  
  if (buf) free (buf);
  if (!getGroupLine (0, statBuffer.st_gid, &gr, &buf)) goto error;
  if (strcmp(group, gr.gr_name)) {
    logMisc(LOG_ERR, "%s should be owne by %s group, not %s",
	    path, group, gr.gr_name);
    goto error;
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "checkDirectoryPerm fails");
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
