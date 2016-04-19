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
#include <acl/libacl.h>

// directories managed by mediatex client
Perm perm[] = {
  _ETC_M,
  _VAR_RUN_M,
  _VAR_LIB_M,
  _VAR_LIB_M_MDTX,
  _VAR_LIB_M_MDTX_MDTX,
  _VAR_LIB_M_MDTX_CVSROOT,
  _VAR_LIB_M_MDTX_COLL,
  _VAR_CACHE_M,
  _VAR_CACHE_M_MDTX,
  _VAR_CACHE_M_MDTX_CACHE,
  _VAR_CACHE_M_MDTX_CACHE_M,
  _VAR_CACHE_M_MDTX_CACHE_COLL,
  _VAR_CACHE_M_MDTX_HTML,
  _VAR_CACHE_M_MDTX_CVS,
  _VAR_CACHE_M_MDTX_CVS_MDTX,
  _VAR_CACHE_M_MDTX_CVS_COLL,
  _VAR_CACHE_M_MDTX_TMP,
  _VAR_CACHE_M_MDTX_TMP_COLL,
  _VAR_CACHE_M_MDTX_HOME,
  _VAR_CACHE_M_MDTX_HOME_COLL,
  _VAR_CACHE_M_MDTX_HOME_COLL_SSH,
  _VAR_CACHE_M_MDTX_HOME_COLL_HTML,
  _VAR_CACHE_M_MDTX_HOME_COLL_VIEWVC,
  _VAR_CACHE_M_MDTX_MD5SUMS,
  _VAR_CACHE_M_MDTX_JAIL 
};

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
 * Function   : logAcl
 * Description: Print an acl
 * Synopsis   : static int logAcl(acl_t acl)
 * Input      : char* text: text to log
 *              acl_t acl: the acl to log
 * Output     : TRUE on success
 =======================================================================*/
int
logAcl(char* text, acl_t acl) 
{
  int rc = FALSE;

  acl_entry_t entry;
  acl_tag_t tag;
  void *idp;
  acl_permset_t permset;
  int entryId, permValR, permValW, permValX;
  char buffer[255] = "";
  int len = 0;
 
  if (acl == NULL) goto error;
 
  // Walk through each entry in this ACL
  for (entryId = ACL_FIRST_ENTRY; ; entryId = ACL_NEXT_ENTRY) {

    // Exit on error or no more entries
    if (acl_get_entry(acl, entryId, &entry) != 1) break;
     
    // Retrieve and display tag type
    if (acl_get_tag_type(entry, &tag) == -1) goto error;
    len += sprintf(buffer+len, "%s", 
		   (tag == ACL_USER_OBJ) ?  "u::" :
		   (tag == ACL_USER) ?      "u:" :
		   (tag == ACL_GROUP_OBJ) ? "g::" :
		   (tag == ACL_GROUP) ?     "g:" :
		   (tag == ACL_MASK) ?      "m::" :
		   (tag == ACL_OTHER) ?     "o::" : "???");
 
    // Retrieve and display optional tag qualifier
    if (tag == ACL_USER || tag == ACL_GROUP) {
      idp = acl_get_qualifier(entry);
      if (idp == NULL) goto error;
      len += sprintf(buffer+len, "%d:", *(int*)idp);
      if (acl_free(idp) == -1) goto error;
    }

    // Retrieve and display permissions
    if (acl_get_permset(entry, &permset) == -1) goto error;
    permValR = acl_get_perm(permset, ACL_READ);
    if (permValR == -1) goto error;
    permValW = acl_get_perm(permset, ACL_WRITE);
    if (permValW == -1) goto error;
    permValX = acl_get_perm(permset, ACL_EXECUTE);
    if (permValX == -1) goto error;
    len += sprintf(buffer+len, "%c%c%c", 
		   (permValR == 1) ? 'r' : '-',
		   (permValW == 1) ? 'w' : '-',
		   (permValX == 1) ? 'x' : '-');
    len += sprintf(buffer+len, " ");
  }

  logMisc(LOG_NOTICE, "%s%s", text, buffer);
  rc = TRUE;
 error:
  return rc;
}

/*=======================================================================
 * Function   : cmpAcl
 * Description: compare an acl
 * Synopsis   : static int cmpDirectoryAcl(char* path, char* aclValue,
 *                                         char* coll, int *res)
 * Input      : char* path: directory to check
 *              char* aclValue: default acl string to compare
 *              char* collUser: collection' user label
 *              int *res: TRUE if equal
 * Output     : TRUE on success
 =======================================================================*/
static int
cmpDirectoryAcl(char* path, char* aclVal, char* collUser, int *res)
{
  int rc = FALSE;
  char noAccessAcl[] = "u::rwx g::r-x o::r-x";
  char noDefaultAcl[] = "";
  char string1[128] = "";
  char string2[128] = "";
  acl_t acl1=0, acl2=0;
  *res = 0;

  logMisc(LOG_DEBUG, "cmpDirectoryAcl %s acl='%s' coll=%s", 
	  path, aclVal, collUser);

  // add common acl part
  if (strcmp(aclVal, "NO ACL")) {
    // same access and default acls
    sprintf(string1, "%s %s", BASE_ACL, aclVal);
    sprintf(string2, string1, env.confLabel, env.confLabel, 
	    collUser?collUser:"");
  }
  else {
    // access acl is not empty, default acl is empty
    strcpy(string2, noAccessAcl);
  }

  // build acl to compare
  if (!(acl1 = acl_from_text(string2))) {
    logMisc(LOG_ERR, "fails to build '%s' acl: %s",
	    string2, strerror(errno));
    goto error;
  }
   
  // valid acl to compare
  if (acl_valid(acl1) != 0) {
    logMisc(LOG_ERR, "invalid/incomplete '%s' acl: %s",
	    string2, strerror(errno));
    goto error;
  }

  // retrieve existing access acl
  if (!(acl2 = acl_get_file(path, ACL_TYPE_ACCESS))) {
    logMisc(LOG_ERR, "fails getting default acl of %s: %s",
	    path, strerror(errno));
    goto error;
  }

  if ((rc =  acl_cmp(acl1, acl2)) == -1) {
    logMisc(LOG_ERR, "cmp_acl fails: %s", strerror(errno));
    goto error;
  }

  // result
  *res = (rc == 0)? TRUE : FALSE;
  if (!*res) {
    logMisc(LOG_ERR, "access acl differ");
    //logMisc(LOG_NOTICE, "-> %s", string2);
    if (!logAcl("expected: ", acl1)) goto error;
    if (!logAcl("get:      ", acl2)) goto error;
    goto end;
  }

  // build acl to compare
  if (!strcmp(aclVal, "NO ACL")) {
    acl_free(acl1);
    if (!(acl1 = acl_from_text(noDefaultAcl))) {
      logMisc(LOG_ERR, "fails to build empty default acl: %s",
	      strerror(errno));
      goto error;
    }
  }

  // retrieve existing default acl (only on directories)
  if (!(acl2 = acl_get_file(path, ACL_TYPE_DEFAULT))) {
    logMisc(LOG_ERR, "fails getting default acl of %s: %s",
	    path, strerror(errno));
    goto error;
  }

  if ((rc =  acl_cmp(acl1, acl2)) == -1) {
    logMisc(LOG_ERR, "cmp_acl fails: %s", strerror(errno));
    goto error;
  }

  // result
  *res = (rc == 0)? TRUE : FALSE;
  if (!*res) {
    logMisc(LOG_ERR, "default acl differ");
    //logMisc(LOG_NOTICE, "-> %s", string2);
    if (!logAcl("expected: ", acl1)) goto error;
    if (!logAcl("get:      ", acl2)) goto error;
    goto error;
  }

 end:
  rc = TRUE;
 error:
  if (acl1) acl_free(acl1);
  if (acl2) acl_free(acl2);
  return rc;
} 

/*=======================================================================
 * Function   : checkDirectoryPerm
 * Description: check existence, owner and permission of a directory.
 * Synopsis   : static int checkDirectory(char* path, int index)
 * Input      : char* collUser: collection user's label
 *              char* path: the path of the directory
 *              int index: the index of the permission to check
 * Output     : TRUE on success
 =======================================================================*/
int 
checkDirectoryPerm(char* collUser, char* path, int index)
{
  int rc = FALSE;
  char* user = perm[index].user;
  char* group = perm[index].group;
  mode_t mode = perm[index].mode;
  char* aclVal = perm[index].acl;
  struct stat statBuffer;
  struct passwd pw;
  struct group gr;
  char *buf = 0;
  mode_t mask = 07777;
  int res;

  logMisc(LOG_DEBUG, "checkDirectoryPerm %s %s %s %o '%s'", 
	  path, user, group, mode, aclVal);

  if (path == 0 || *path == (char)0) {
    logMisc(LOG_ERR, "cannot check empty directory path");
    goto error;
  }

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

  // we bypass checks if we are running a unit test
  if (env.noRegression) {
    goto noRegression;
  }
  
  // check user
  if (!strcmp(user, "%s")) user=collUser;
  if (!getPasswdLine (0, statBuffer.st_uid, &pw, &buf)) goto error;
  if (strcmp(user, pw.pw_name)) {
    logMisc(LOG_ERR, "%s should be owned by %s user, not %s",
	    path, user, pw.pw_name);
    goto error;
  }
  
  // check group  
  if (buf) free (buf);
  if (!strcmp(group, "%s")) group=collUser;
  if (!getGroupLine (0, statBuffer.st_gid, &gr, &buf)) goto error;
  if (strcmp(group, gr.gr_name)) {
    logMisc(LOG_ERR, "%s should be owned by %s group, not %s",
	    path, group, gr.gr_name);
    goto error;
  }

  if (!strcmp(aclVal, "NO ACL")) {
    // check permissions if no acl is provided
    if ((statBuffer.st_mode & mask) != (mode & mask)) {
      logMisc(LOG_ERR, "%s should be share as %o, not %o",
	      path, mode & mask, statBuffer.st_mode & mask);
      goto error;
    }
  }
  else {
    // check acl if provided
    if (!cmpDirectoryAcl(path, aclVal, collUser, &res) || !res)
      goto error;
  }

 noRegression:
  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "checkDirectoryPerm fails on %s", path);
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
