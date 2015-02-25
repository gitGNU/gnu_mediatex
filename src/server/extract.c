/*=======================================================================
 * Version: $$
 * Project: MediaTeX
 * Module : mdtx-extract
 *
 * Manage remote extraction

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

#include "../mediatex.h"
#include "../misc/log.h"
#include "../misc/device.h"
#include "../misc/command.h"
#include "../common/connect.h"
#include "../common/extractScore.h"
#include "../common/openClose.h"
#include "cache.h"
#include "deliver.h"
#include "extract.h"

#include <pthread.h>
#include <libgen.h> // basename

#include <sys/socket.h> //
#include <netinet/in.h> // inet_ntoa
#include <arpa/inet.h>  //

#include "utFunc.h"

int extractArchive(ExtractData* data, Archive* archive);

/*=======================================================================
 * Function   : mdtxCall
 * Description: call mdtx client
 * Synopsis   : int mdtxCall()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxCall(int nbArgs, ...)
{ 
  int rc = FALSE;
  va_list args;
  char *argv[] = {
    CONF_BINDIR CONF_MEDIATEXDIR,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
  int i = 0;

  logEmit(LOG_DEBUG, "%s", "call mdtx client");

  va_start(args, nbArgs);
  while (--nbArgs >= 0) {
    argv[++i] = va_arg(args, char *);
  }
  va_end(args);

  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, NULL, NULL, TRUE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "mdtx client call fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getAbsExtractPath
 * Description: get an absolute path to the temp extraction directory
 * Synopsis   : char* getAbsExtractPath(Collection* coll, char* path) 
 * Input      : Collection* coll = the related collection
 *              char* path = the relative path
 * Output     : absolute path, NULL on failure
 =======================================================================*/
char*
getAbsExtractPath(Collection* coll, char* path) 
{
  char* rc = NULL;

  checkCollection(coll);
  checkLabel(path);
  logEmit(LOG_DEBUG, "%s", "absCachePath");

  if (!(rc = createString(coll->extractDir))) goto error;
  if (!(rc = catString(rc, "/"))) goto error;
  if (!(rc = catString(rc, path))) goto error;
  
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "absExtractPath fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : thereIs
 * Description: check path
 * Synopsis   : int thereIs(char* path) 
 * Input      : char* path = directory path to check
 * Output     : TRUE on success
 =======================================================================*/
int
thereIs(Collection* coll, char* path) 
{
  int rc = FALSE;

  checkLabel(path);
  logEmit(LOG_DEBUG, "thereIs: check %s path", path);

  if (access(path, R_OK) != 0) {
    logEmit(LOG_ERR, "access fails: %s", strerror(errno));
    goto error;
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "thereIs fails");    
  }
  return rc;
}

/*=======================================================================
 * Function   : uniqueCachePath
 * Description: return a path not alrady used in cache
 * Synopsis   : char* uniqueCachePath(Collection* coll, FromAsso* asso) 
 * Input      : Collection* coll = the related collection
 *              char* path = the relative path
 * Output     : path not use into the cache, NULL on failure
 =======================================================================*/
char*
getUniqCachePath(Collection* coll, char* path) 
{
  char* rc = NULL;
  char* res = NULL;
  int err = 0;
  char i=(char)0, j=(char)0;
  int l;

  checkCollection(coll);
  checkLabel(path);
  logEmit(LOG_DEBUG, "%s", "uniqueCachePath");

  // try with the native path
  if (!(res = getAbsCachePath(coll, path))) goto error;
  if ((err = callAccess(res))) {
    if (err == ENOENT) goto end;
    logEmit(LOG_ERR, "access %s: %s", res, strerror(err));
  }

  // add "_00" to the path and loop on number
  if (!(res = catString(res, "_-01-"))) goto error;
  l = strlen(res) - 3;
  for (i='0'; i<='9'; ++i) {
    for (j='0'; j<='9'; ++j) {
      res[l] = i;
      res[l+1] = j;
      if (callAccess(res) == ENOENT) goto end;
    }
  }
  goto error;

 end:
  rc = res;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "uniqueCachePath fails");
    res = destroyString(res);
  }
  return rc;
}

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : Collection* coll = the related collection
 *              Archive* archive = the releted archive
 * Output     : absolute path, NULL on failure
 =======================================================================*/
char*
getArchivePath(Collection* coll, Archive* archive) 
{
  char* rc = NULL;
  Record* record = NULL;

  checkCollection(coll);
  checkArchive(archive);
  logEmit(LOG_DEBUG, "%s", "getArchivePath");

  // local supply
  if (archive->state >= AVAILABLE) {
    if (!(record = archive->localSupply)) goto error;
    if (isEmptyString(record->extra)) goto error;
  }
  else {
     if (archive->finaleSupplies->nbItems > 0) {
       if (!(record = (Record*)archive->finaleSupplies->head->it)) 
	 goto error;
       if (isEmptyString(record->extra)) goto error;
     }
  }

  rc = getAbsRecordPath(coll, record);
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "getArchivePath fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : extractDd
 * Description: call dd
 * Synopsis   : int extractDd(Collection* coll, FromAsso* asso)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 =======================================================================*/
int 
extractDd(Collection* coll, char* device, char* target, off_t size)
{
  int rc = FALSE;
  char* argv[] = {"/bin/dd", NULL, NULL, "bs=2048", NULL, NULL};
  char count[MAX_SIZE_SIZE+7];

  checkCollection(coll);
  checkLabel(device);
  checkLabel(target);
  logEmit(LOG_DEBUG, "extractDd %s", target);
  
  if (!(argv[1] = createString("if="))
      || !(argv[1] = catString(argv[1], device)))
    goto error;

  if (!(argv[2] = createString("of="))
      || !(argv[2] = catString(argv[2], target)))
    goto error;
  
  // divide size by bs=2048 (count needed, else we get an IO error)
  sprintf(count, "count=%lli", (long long int)size>>11);
  argv[4] = count;

  if (!env.dryRun && !execScript(argv, NULL, NULL, FALSE)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "extractDd fails");
  }
  argv[1] = destroyString(argv[1]);
  argv[2] = destroyString(argv[2]);
  return rc;
}

/*=======================================================================
 * Function   : extractScp
 * Description: call scp
 * Synopsis   : int extractScp(Collection* coll, FromAsso* asso)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 =======================================================================*/
int 
extractScp(Collection* coll, Record* record)
{
  int rc = FALSE;

  checkCollection(coll);
  checkRecord(record);
  logEmit(LOG_DEBUG, "extractScp %s", record->extra);
  
  if (!env.dryRun && 
      !mdtxCall(7, "adm", "get", record->extra, 
		"as", coll->label, 
		"on", record->server->fingerPrint))
    goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "extractScp fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : extractIso
 * Description: call mdtx (that call mount)
 * Synopsis   : iso extractIso(Collection* coll, FromAsso* asso)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 =======================================================================*/
int 
extractIso(Collection* coll, FromAsso* asso, char* tmpPath)
{
  int rc = FALSE;
  Archive* archive = (Archive*)0;
  char* iso = NULL;
  char* mnt = NULL;
  char* path = NULL;
  int i,j,l;

  logEmit(LOG_DEBUG, "%s", "isoExtract");

  // should have one and only one parent
  if (!(archive = (Archive*)asso->container->parents->head->it)) 
    goto error;

  // mount the iso
  if (!(iso = getArchivePath(coll, archive))) goto error;

  // try with the native path
  if (!(mnt = createString(coll->extractDir))
      || !(mnt = catString(mnt, "/mnt"))) 
    goto error;

  if (callAccess(mnt) == ENOENT) goto next;
  // add "_00" to the path and loop on number
  if (!(mnt = catString(mnt, "_01"))) goto error;
  l = strlen(mnt) - 2;
  for (i='0'; i<='9'; ++i) {
    for (j='0'; j<='9'; ++j) {
      mnt[l] = i;
      mnt[l+1] = j;
      if (callAccess(mnt) == ENOENT) goto next;
    }
  }
  goto error;

 next:
  if (!mdtxCall(6, "adm", "mount", iso, "on", mnt, "-X")) goto error;

  // copy the asso file
  if (!(path = createString(mnt))
      || !(path = catString(path, "/"))
      || !(path = catString(path, asso->path)))
    goto error2;
  if (!extractCp(path, tmpPath)) goto error2;
  
  rc = TRUE;
 error2:
  // umount the iso
  if (!mdtxCall(3, "adm", "umount", mnt)) rc = FALSE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "isoExtract fails");
  }
  iso = destroyString(iso);
  mnt = destroyString(mnt);
  path = destroyString(path);
  return(rc);
}

/*=======================================================================
 * Function   : extractCat
 * Description: call cat
 * Synopsis   : int extractCat(Collection* coll, FromAsso* asso)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 * Note       : we need to load bash in order to do the '>' redirection
 =======================================================================*/
int 
extractCat(Collection* coll, FromAsso* asso, char* path)
{
  int rc = FALSE;
  char *cmd = NULL;
  Archive* archive = NULL;
  RGIT* curr = NULL;
  char* argv[] = {"/bin/bash", "-c", NULL, NULL};
  char* path2 = NULL;

  logEmit(LOG_DEBUG, "%s", "do cat");
  if (!(cmd = createString("/bin/cat "))) goto error;
 
  // all parts
  while((archive = rgNext_r(asso->container->parents, &curr))) {
    if (!(path2 = getArchivePath(coll, archive))) goto error;
    if (!(cmd = catString(cmd, path2))) goto error;
    if (!(cmd = catString(cmd, " "))) goto error;
    path2 = destroyString(path2);
  }
  
  // concatenation resulting file
  if (!(cmd = catString(cmd, ">"))) goto error;
  if (!(cmd = catString(cmd, path))) goto error;
  argv[2] = cmd;

  if (!env.dryRun && !execScript(argv, NULL, NULL, FALSE)) goto error;

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to do cat");
  }
  path2 = destroyString(path2);
  cmd = destroyString(cmd);
  return(rc);
}

/*=======================================================================
 * Function   : extractTar
 * Description: call tar -x
 * Synopsis   : int extractTar(Collection* coll, FromAsso* asso,
 *                                                   char* options)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 =======================================================================*/
int 
extractTar(Collection* coll, FromAsso* asso, char* options)
{
  int rc = FALSE;
  char *argv[] = {"/bin/tar", "-C", NULL, NULL, NULL, NULL, NULL};
  Archive* archive = NULL;

  logEmit(LOG_DEBUG, "do tar %s", options);

  if (!env.dryRun) {
    argv[2] = coll->extractDir; 
    argv[3] = options;
    if (asso->container->parents->nbItems <= 0) goto error;
    if (!(archive = (Archive*)asso->container->parents->head->it)) 
      goto error;
    if (!(argv[4] = getArchivePath(coll, archive))) goto error;
    argv[5] = asso->path;

    if (!execScript(argv, NULL, NULL, FALSE)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to do tar");
  }
  destroyString(argv[4]);
  return(rc);
}

/*=======================================================================
 * Function   : extractXzip
 * Description: call Xunzip
 * Synopsis   : int extractXzip(Collection* coll, FromAsso* asso,
 *                                         char* tmpPath, char* bin)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 * Note       : we need to load bash in order to do the '>'redirection
 *              use redirection so as to not delete original file and
 *              to decompress into another directory
 =======================================================================*/
int 
extractXzip(Collection* coll, FromAsso* asso, char* tmpPath, char* bin)
{
  int rc = FALSE;
  char* argv[] = {"/bin/bash", "-c", NULL, NULL};
  char *cmd = NULL;
  char* path = NULL;
  Archive* archive = NULL;

  logEmit(LOG_DEBUG, "do Xunzip %s", bin);

  if (!(cmd = createString(bin))) goto error;

  if (asso->container->parents->nbItems <= 0) goto error;
  if (!(archive = (Archive*)asso->container->parents->head->it)) 
    goto error;
  if (!(path = getArchivePath(coll, archive))) goto error;

  if (!(cmd = catString(cmd, path))) goto error;
  if (!(cmd = catString(cmd, " >"))) goto error;
  if (!(cmd = catString(cmd, tmpPath))) goto error;
  argv[2] = cmd;

  if (!env.dryRun && !execScript(argv, NULL, NULL, FALSE)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to do Xzip");
  }
  path = destroyString(path);
  cmd = destroyString(cmd);
  return(rc);
}

/*=======================================================================
 * Function   : extractAfio
 * Description: call afio -i
 * Synopsis   : int extractAfio(Collection* coll, FromAsso* asso)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 =======================================================================*/
int 
extractAfio(Collection* coll, FromAsso* asso)
{
  int rc = FALSE;
  char *argv[] = {"/bin/afio", "-i", "-y", NULL, NULL, NULL};
  Archive* archive = NULL;

  logEmit(LOG_DEBUG, "%s", "do afio -i");

  if (!env.dryRun) {
    argv[3] = asso->path;
    if (asso->container->parents->nbItems <= 0) goto error;
    if (!(archive = (Archive*)asso->container->parents->head->it)) 
      goto error;
    if (!(argv[4] = getArchivePath(coll, archive))) goto error;

    if (!execScript(argv, NULL, coll->extractDir, FALSE)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to do afio");
  }
  argv[4] = destroyString(argv[4]);
  return(rc);
}

/*=======================================================================
 * Function   : extractCpio
 * Description: call cpio -i
 * Synopsis   : int extractCpio(Collection* coll, FromAsso* asso)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 =======================================================================*/
int 
extractCpio(Collection* coll, FromAsso* asso)
{
  int rc = FALSE;
  char *argv[] = {"/bin/cpio", "-i", "--file", NULL, 
		  "--make-directories", NULL, NULL};
  Archive* archive = NULL;

  logEmit(LOG_DEBUG, "%s", "do cpio -i");

  if (!env.dryRun) {
    if (asso->container->parents->nbItems <= 0) goto error;
    if (!(archive = (Archive*)asso->container->parents->head->it)) 
      goto error;
    if (!(argv[3] = getArchivePath(coll, archive))) goto error;
    argv[5] = asso->path;

    if (!execScript(argv, NULL, coll->extractDir, TRUE)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to do cpio");
  }
  argv[3] = destroyString(argv[3]);
  return(rc);
}

/*=======================================================================
 * Function   : extractZip
 * Description: call unzip
 * Synopsis   : int extractZip(Collection* coll, FromAsso* asso)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 =======================================================================*/
int 
extractZip(Collection* coll, FromAsso* asso)
{
  int rc = FALSE;
  char *argv[] = {"/usr/bin/unzip", "-d", NULL, NULL, NULL, NULL};
  Archive* archive = NULL;

  logEmit(LOG_DEBUG, "%s", "do unzip");

  if (!env.dryRun) {
    argv[2] = coll->extractDir; 
    if (asso->container->parents->nbItems <= 0) goto error;
    if (!(archive = (Archive*)asso->container->parents->head->it)) 
      goto error;
    if (!(argv[3] = getArchivePath(coll, archive))) goto error;
    argv[4] = asso->path; 

    if (!execScript(argv, NULL, NULL, FALSE)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to do unzip");
  }
  destroyString(argv[3]);
  return(rc);
}

/*=======================================================================
 * Function   : extractRar
 * Description: call rar x
 * Synopsis   : int extractRar(Collection* coll, FromAsso* asso)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 =======================================================================*/
int 
extractRar(Collection* coll, FromAsso* asso)
{
  int rc = FALSE;
  char *argv[] = {"/usr/bin/rar", "x", NULL, NULL, NULL, NULL};
  Archive* archive = NULL;

  logEmit(LOG_DEBUG, "%s", "do rar x");

  if (!env.dryRun) {
    if (asso->container->parents->nbItems <= 0) goto error;
    if (!(archive = (Archive*)asso->container->parents->head->it)) 
      goto error;
    if (!(argv[2] = getArchivePath(coll, archive))) goto error;
    argv[3] = asso->path; 
    argv[4] = coll->extractDir; 

    if (!execScript(argv, NULL, NULL, FALSE)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to do rar e");
  }
  destroyString(argv[2]);
  return(rc);
}

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : TRUE on success
 =======================================================================*/
int
extractAddToKeep(ExtractData* data, Archive* archive)
{
  int rc = FALSE;
  Collection* coll = NULL;

  coll = data->coll;
  checkCollection(coll);
  checkArchive(archive);
  logEmit(LOG_DEBUG, "add to-keep on %s:%lli",   
	  archive->hash, archive->size); 

  if (!keepArchive(coll, archive, UNDEF_RECORD)) goto error;
  if (!rgInsert(data->toKeeps, archive)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "extractAddToKeep fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : TRUE on success
 =======================================================================*/
int
extractDelToKeeps(Collection* coll, RG* toKeeps)
{
  int rc = FALSE;
  Archive* archive = NULL;
  RGIT* curr = NULL;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "%s", "del toKeeps"); 

  while ((archive = rgNext_r(toKeeps, &curr))) {
    logEmit(LOG_INFO, "del to-keep on %s:%lli", 
	    archive->hash, archive->size);     

    if (!unKeepArchive(coll, archive)) goto error;
  }
  
  rc = TRUE;
 error:
    if (!rc) {
    logEmit(LOG_ERR, "%s", "extractDelToKeeps fails");
  }
  destroyOnlyRing(toKeeps);
  return rc;
}

/*=======================================================================
 * Function   : moveIntoCache
 * Description: move file into the cache
 * Synopsis   : char* moveIntoCache(Collection* coll, char* path) 
 * Input      : Collection* coll = the related collection
 *              char* path = the relative path
 * Output     : TRUE on success
 * Note       : as tar cannot rename a file, and in order not to have
 *              collisions, we extract files in a temporary place 
 *              and next rename them

 * TODO       : remove extract dirs ~/tmp/... (if empty)
 =======================================================================*/
int
cacheSet(ExtractData* data, Record* record, char* path) 
{
  int rc = FALSE;
  char* source = NULL;
  char* target = NULL;
  Collection* coll = NULL;

  coll = data->coll;
  checkCollection(coll);
  checkRecord(record);
  checkLabel(path);
  logEmit(LOG_DEBUG, "%s", "cacheSet");  

  // move from extract dir into the cache dir
  if (!(source = getAbsExtractPath(coll, path))) goto error;
  if (!(target = getUniqCachePath(coll, path))) goto error;
  logEmit(LOG_INFO, "move %s to %s", source, target);  
  
  if (!makeDir(coll->cacheDir, target, 0750)) goto error;  
  if (rename(source, target)) {
    logEmit(LOG_ERR, "rename fails: %s", strerror(errno));
    goto error;
  }

  // here we should remove extract dirs ~/tmp/... (if empty)

  // toggle !malloc record to local-supply...
  record->extra = destroyString(record->extra);
  if (!(record->extra = createString(path))) goto error;
  if (path[0] == '/') {
    logEmit(LOG_WARNING, "%s", "please remove absolute in the extract file");
    logEmit(LOG_WARNING, "%s", "current extract will fails due to that");
  }

  // and add a toKepp on the new extracted archive
  if (!extractAddToKeep(data, record->archive)) goto error;

  logEmit(LOG_NOTICE, "extract %s:%lli", 
	  record->archive->hash, record->archive->size);
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "cacheSet fails");
  }
  source = destroyString(source);
  target = destroyString(target);
  return rc;
}

/*=======================================================================
 * Function   : extractRecord
 * Description: extract a record (cp and scp)
 * Synopsis   : int extractRecord(Collection* coll, Record* record)
 * Input      : Collection* coll
 *              Record* record
 * Output     : TRUE on success
 =======================================================================*/
int 
extractRecord(ExtractData* data, Record* record)
{
  int rc = FALSE;
  Collection* coll = NULL;
  char* tmpBasename = NULL;
  char* tmpPath = NULL;
  FromAsso* asso = NULL;
  Record* record2 = NULL;
  char* device = NULL;
  char* source = NULL;
  int isBlockDev = FALSE;
  int i = 0;

  coll = data->coll;
  checkCollection(coll);
  checkRecord(record);
  logEmit(LOG_DEBUG, "extractRecord %s:%lli", 
	  record->archive->hash, record->archive->size);

  // allocate place on cache
  if (!cacheAlloc(&record2, coll, record->archive)) goto error;

  // retrieve the target name to use
  if (record->archive && 
      !isEmptyRing(record->archive->fromContainers) &&
      (asso = (FromAsso*)record->archive->fromContainers->head->it)) {

    // if it exists, use the first asso name
    tmpBasename = asso->path;
  }
  else {
    // else continue to use the source name 
    if (record->extra[0] != '/') {
      tmpBasename = record->extra;
    }
    else {
      // or only the basename for final supply
      for (i = strlen(record->extra); i>=0 && record->extra[i] != '/'; --i);
      tmpBasename = record->extra + i + 1;
    }

  }
  
  if (!(tmpPath = getAbsExtractPath(coll, tmpBasename))) goto error;
  if (!makeDir(coll->extractDir, tmpPath, 0770)) goto error;

  switch (getRecordType(record)) {
  case FINALE_SUPPLY:
    // may be a block device to dump
    if (!getDevice(record->extra, &device)) goto error;
    if (!isBlockDevice(device, &isBlockDev)) goto error;
    if (isBlockDev) {
      if (!extractDd(coll, device, tmpPath, record->archive->size)) 
	goto error;
    }
    else {
      if (!(source = getAbsRecordPath(coll, record))) goto error;
      if (!extractCp(source, tmpPath)) goto error;
    }
    break;
  case REMOTE_SUPPLY:
    if (!extractScp(coll, record)) goto error;
    break;
  default:
    logEmit(LOG_ERR, "cannot extract %s record", strRecordType(record));
    goto error;
  }
  
  // toggle !malloc record to local supply
  if (!cacheSet(data, record2, tmpBasename)) goto error;
  if (!removeDir(coll->extractDir, tmpPath)) goto error;

  tmpBasename = NULL;
  record2 = NULL;
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "extractRecord fails");
  }
  if (record2) delCacheEntry(coll, record2);
  tmpPath  = destroyString(tmpPath);
  device = destroyString(device);
  source = destroyString(source);
  return rc;
}

/*=======================================================================
 * Function   : extractFromAsso
 * Description: extract a FromAsso (iso, cat and tgz)
 * Synopsis   : int extractFromAsso(Collection* coll, Record* record)
 * Input      : Collection* coll
 *              Record* record
 * Output     : TRUE on success
 =======================================================================*/
int 
extractFromAsso(ExtractData* data, FromAsso* asso)
{
  int rc = FALSE;
  Collection* coll = NULL;
  Record* record = NULL;
  char* tmpPath = NULL;

  coll = data->coll;
  checkCollection(coll);
  logEmit(LOG_DEBUG, "extractFromAsso %s:%lli", 
	  asso->archive->hash, asso->archive->size);

  // allocate place on cache
  if (!cacheAlloc(&record, coll, asso->archive)) goto error;

  if (!(tmpPath = getAbsExtractPath(coll, asso->path))) goto error;
  if (!makeDir(coll->extractDir, tmpPath, 0770)) goto error;

  switch (asso->container->type) {
  case ISO:
    if (!extractIso(coll, asso, tmpPath)) goto error;
    break;
    
  case CAT:
    if (!extractCat(coll, asso, tmpPath)) goto error;
    break;
    
  case TGZ:
    if (!extractTar(coll, asso, "-zxf")) goto error;
    break;

  case TBZ:
    if (!extractTar(coll, asso, "-jxf")) goto error;
    break;

  case AFIO:
    if (!extractAfio(coll, asso)) goto error;
    break;

  case TAR:
    if (!extractTar(coll, asso, "-xf")) goto error;
    break;

  case CPIO:
    if (!extractCpio(coll, asso)) goto error;
    break;

  case GZIP:
    if (!extractXzip(coll, asso, tmpPath, "/bin/zcat ")) goto error;
    break;

  case BZIP:
    if (!extractXzip(coll, asso, tmpPath, "/bin/bzcat ")) goto error;
    break;

  case ZIP:
    if (!extractZip(coll, asso)) goto error;
    break;

  case RAR:
    if (!extractRar(coll, asso)) goto error;
    break;
    
  default:
    logEmit(LOG_ERR, "container %s not manage", 
	    strEType(asso->container->type));
    goto error;
  }
  
  // toggle !malloc record to local supply
  if (!cacheSet(data, record, asso->path)) goto error;
  if (!removeDir(coll->extractDir, tmpPath)) goto error;

  record = NULL;
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "extractFromAsso fails");
  }
  if (record) delCacheEntry(coll, record);
  // destroyRecord(record); !! do not destroy it (done by cacheTree)
  tmpPath = destroyString(tmpPath);
  return rc;
}

/*=======================================================================
 * Function   : extractContainer
 * Description: Single container extraction
 * Synopsis   : int extractContainer
 * Input      : Collection* coll = context
 *              Container* container = what to extract
 * Output     : int data->found = TRUE if all parts are extracted
 *              FALSE on error
 * Note       : Recursive calls to extract container(s)
 =======================================================================*/
int extractContainer(ExtractData* data, Container* container)
{
  int rc = FALSE;
  Archive* archive = NULL;
  Record* record = NULL;
  RGIT* curr = NULL;
  RGIT* curr2 = NULL;
  int found = FALSE;

  checkCollection(data->coll);
  logEmit(LOG_DEBUG, "extract container %s/%s:%lli", 
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size);

  if (isEmptyRing(container->parents)) goto end;

  // we extract all parents (only one for TGZ, several for CAT...)
  curr = NULL;
  found = TRUE;
  while((archive = rgNext_r(container->parents, &curr))) {
    if (!extractArchive(data, archive)) goto error;
    found = found && data->found;
  }

  // postfix: copy container parts from final supplies
  if (!found) {

    // for each part we have on final supply
    curr = NULL;
    while((archive = rgNext_r(container->parents, &curr))) {
      if (archive->state < AVAILABLE &&
  	  archive->finaleSupplies->nbItems > 0) {
  	logEmit(LOG_INFO, "%s", "extract part from support");

  	// stop on the first final-supply extraction that success
	data->found = FALSE;
  	while (!data->found &&
  	       (record = rgNext_r(archive->finaleSupplies, &curr2))) {
  	  if (!extractRecord(data, record)) goto error;
  	}
      }
    }
    // here we have only copy parts for next try
    data->found = FALSE;
  }

 end:
  data->found = found;
  logEmit(LOG_INFO, "%sfound container %s/%s:%lli", data->found?"":"not ", 
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size);
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "container extraction error");
    data->found = FALSE;
  }
  return rc;
} 

/*=======================================================================
 * Function   : extractArchive
 * Description: Single archive extraction
 * Synopsis   : int extractArchive
 * Input      : Collection* coll = context
 *              Archive* archive = what to extract
 * Output     : int data->found = TRUE if extracted
 *              FALSE on error
 * Note       : Recursive call
 =======================================================================*/
int extractArchive(ExtractData* data, Archive* archive)
{
  int rc = FALSE;
  FromAsso* asso = NULL;
  Record* record = NULL;
  char* path = NULL;
  RGIT* curr = NULL;
  RG* toKeepsBck = NULL;
  RG* toKeepsTmp = NULL;

  logEmit(LOG_DEBUG, "extract an archive: %s:%lli", 
	  archive->hash, archive->size);
  data->found = FALSE;
  checkCollection(data->coll);

  if (archive->state >= AVAILABLE) {
    // test if the file is really there 
    if (!(path = getAbsRecordPath(data->coll, archive->localSupply))) 
      goto error;
    if (!thereIs(data->coll, path)) goto error;
    if (!extractAddToKeep(data, archive)) goto error;
    data->found = TRUE;
    goto end;
  }

  // final supply
  if (archive->finaleSupplies->nbItems > 0) {
    // test if the file is really there 
    if (!(record = (Record*)archive->finaleSupplies->head->it)) goto error;
    if (!thereIs(data->coll, record->extra)) goto error;

    // copy archive if wanted as final demand
    if (data->target == archive) {
      if (!extractRecord(data, record)) goto error;
    }

    data->found = TRUE;
    goto end;
  }

  // prefix: scp. Else (postfix) we will scp containers first.
  if (data->context != X_CGI) {
    curr = NULL;
    while(!data->found && 
	  (record = rgNext_r(archive->remoteSupplies, &curr))) {
      if (record->type & REMOVE) continue;

      // scp supply from nat clients on remote demand (may be optimized)
      if (data->context == X_STEP &&
	  !rgShareItems(data->coll->localhost->gateways,
			record->server->networks)) 
	continue;

      if (!extractRecord(data, record)) goto error;
      data->found = TRUE;
      goto end; 
    }
  }

  // Recursive call: we stop on the first container we have extract

  // backup toKeeps
  toKeepsBck = data->toKeeps;
  if (!(data->toKeeps = createRing())) goto error;

  curr = NULL;
  while(!data->found && (asso = rgNext_r(archive->fromContainers, &curr))) {
    if (!extractContainer(data, asso->container)) goto error;
  }

  // backup temporary toKeeps
  toKeepsTmp = data->toKeeps;
  data->toKeeps = toKeepsBck;

  if (data->found) {
    if (!extractFromAsso(data, asso)) goto error;
  }

  // release temporary toKeeps
  if (!extractDelToKeeps(data->coll, toKeepsTmp)) goto error;

 end:
  logEmit(LOG_INFO, "%sfound archive %s:%lli", 
	  data->found?"":"not ", archive->hash, archive->size);
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "archive extraction error");
  }
  path = destroyString(path);
  return rc;
} 

/*=======================================================================
 * Function   : getWantedArchives
 * Description: copy all wanted archives into a ring
 * Synopsis   : RG* getWantedArchives(Collection* coll)
 * Input      : Collection* coll
 * Output     : a ring of arhives, NULL o error
 =======================================================================*/
RG* 
getWantedArchives(Collection* coll)
{
  RG* rc = NULL;
  RG* ring = NULL;
  Archive* archive = NULL;
  RGIT* curr = NULL;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "%s", "getWantedArchives");

  if (!(ring = createRing())) goto error;

  while((archive = rgNext_r(coll->cacheTree->archives, &curr)) 
	!= NULL) {
    if (archive->state != WANTED) continue;
    if (!rgInsert(ring, archive)) goto error;
  }

  // add top archives with bad score
  if (!computeExtractScore(coll)) goto error;
   while((archive = rgNext_r(coll->cacheTree->archives, &curr)) 
	!= NULL) {
     if (archive->state < USED) continue;
     if (archive->state > WANTED) continue;
     if (archive->fromContainers->nbItems != 0) continue;
     if (archive->remoteSupplies->nbItems == 0) continue;
     if (archive->extractScore > 10 
	 /*coll->serverTree->scoreParam.badScore*/)
       continue;
    if (!rgInsert(ring, archive)) goto error;
  }

  rc = ring;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "getWantedArchives fails");
    destroyOnlyRing(ring);
  }
  return rc;
} 

/*=======================================================================
 * Function   : extractArchives
 * Description: Try to extract all the archives one by one
 * Synopsis   : int extractRemoteArchives(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int extractArchives(Collection* coll)
{
  int rc = FALSE;
  RG* archives = NULL;
  Archive* archive = NULL;
  Record* record = NULL;
  RGIT* curr = NULL;
  RGIT* curr2 = NULL;
  ExtractData data;
  int deliver = FALSE;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "extractArchives %s collection", coll->label);
  memset(&data, 0, sizeof(ExtractData));
  data.coll = coll;

  if (!loadCollection(coll, SERV|EXTR|CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;
  if (!(archives = getWantedArchives(coll))) goto error3;

  // for each cache entry
  while((archive = rgNext_r(archives, &curr)) != NULL) {

    // define extraction context : X_MAIN or X_STEP (ie: scp or not)
    data.context = X_STEP;

    // we scp only if bad score...
    if (archive->extractScore <= 10) data.context = X_MAIN;

    // ... or locally wanted
    curr2 = NULL;
    while ((record = rgNext_r(archive->demands, &curr2))) {
      if (getRecordType(record) == FINALE_DEMAND) {
	data.context = X_MAIN;
	break;
      }
    }

    if (!(data.toKeeps = createRing())) goto error3;
    data.target = archive;
    if (!extractArchive(&data, archive)) {
      logEmit(LOG_WARNING, "%s", "need more place ?");
      continue;
    }
    if (data.found) {

      // adjuste to-keep date
      curr2 = NULL;
      while((record = rgNext_r(archive->demands, &curr2)) != NULL) {
	if (!keepArchive(coll, archive, getRecordType(record)))
	  goto error3;
	if (getRecordType(record) == FINALE_DEMAND &&
	    !(record->type & REMOVE)) deliver = TRUE;
      }

      if (deliver && !deliverMail(coll, archive)) goto error3;
      deliver = FALSE;
    }
  }

  rc = TRUE;
 error3:
  if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, SERV|EXTR|CACH)) goto error;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "remote extraction fails");
  }
  curr = NULL;
  destroyOnlyRing(archives);
  destroyOnlyRing(data.toKeeps);
  return rc;
} 

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "utFunc.h"
GLOBAL_STRUCT_DEF;

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually argv[0].
 * Output     : N/A
 =======================================================================*/
static void 
usage(char* programName)
{
  mdtxUsage(programName);
  fprintf(stderr, " [ -d repository ]");

  mdtxOptions();
  fprintf(stderr, "  ---\n"
	  "  -d, --input-rep\trepository with logo files\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2013/01/10
 * Description: Unit test for cache module.
 * Synopsis   : ./utcache
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
  Collection* coll = NULL;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"input-rep", required_argument, NULL, 'd'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
    case 'd':
      if(optarg == NULL || *optarg == (char)0) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input repository\n",
		programName);
	rc = EINVAL;
	break;
      }
      strncpy(inputRep, optarg, strlen(optarg)+1);
      break; 

      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  env.dryRun = FALSE;
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll = mdtxGetCollection("coll3"))) goto error;

  utLog("%s", "Clean the cache:", NULL);
  if (!utCleanCaches()) goto error;
  
  utLog("%s", "add a demand for logo.png:", NULL);
  if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075, 
		"test@test.com")) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "try to extract the demand (1)", NULL);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "Put some files:", NULL);
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.cat")) goto error;
  
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan gives :", coll);
  
  utLog("%s", "try to extract the demand (2)", NULL);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  /////////////////////////////////////////////////////////////

  utLog("%s", "Try tar-bzip2 container:", NULL);
  if (!utCleanCaches()) goto error;
  if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075, 
		"test@test.com")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tar.bz2")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before tar-bzip:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  /* utLog("%s", "Try afio container:", NULL); */
  /* if (!utCleanCaches()) goto error; */
  /* if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075,  */
  /* 		"test@test.com")) goto error; */
  /* if (!utCopyFileOnCache(coll, inputRep, "logo.afio")) goto error; */
  /* if (!quickScan(coll)) goto error; */
  /* utLog("%s", "Scan before afio:", coll); */
  /* if (!extractArchives(coll)) goto error; */
  /* utLog("%s", "Now we have :", coll); */

  utLog("%s", "Try zip container:", NULL);
  if (!utCleanCaches()) goto error;
  if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075, 
		"test@test.com")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.zip")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before unzip:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  /* utLog("%s", "Try rar container:", NULL); */
  /* if (!utCleanCaches()) goto error; */
  /* if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075,  */
  /* 		"test@test.com")) goto error; */
  /* if (!utCopyFileOnCache(coll, inputRep, "logo.part1.rar")) goto error; */
  /* if (!utCopyFileOnCache(coll, inputRep, "logo.part2.rar")) goto error; */
  /* if (!quickScan(coll)) goto error; */
  /* utLog("%s", "Scan before unrar:", coll); */
  /* if (!extractArchives(coll)) goto error; */
  /* utLog("%s", "Now we have :", coll); */

  utLog("%s", "Try tar container:", NULL);
  if (!utCleanCaches()) goto error;
  if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075, 
		"test@test.com")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tar")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before tar:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  utLog("%s", "Try cpio+gz container:", NULL);
  if (!utCleanCaches()) goto error;
  if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075,
  		"test@test.com")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.cpio.gz")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before tar:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  utLog("%s", "Try cpio+bz2 container:", NULL);
  if (!utCleanCaches()) goto error;
  if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075,
  		"test@test.com")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.cpio.bz2")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before tar:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  utLog("%s", "Clean the cache:", NULL);
  if (!utCleanCaches()) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  freeConfiguration();
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
