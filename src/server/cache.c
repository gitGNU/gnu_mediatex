/*=======================================================================
 * Version: $Id: cache.c,v 1.8 2015/08/05 12:12:02 nroche Exp $
 * Project: MediaTeX
 * Module : cache
 *
 * Manage local cache directory and DB

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
#include "server/mediatex-server.h"
#include <dirent.h> // alphasort

/*=======================================================================
 * Function   : getAbsCachePath
 * Description: get an absolute path to the cache
 * Synopsis   : char* absCachePath(Collection* coll, char* path) 
 * Input      : Collection* coll = the related collection
 *              char* path = the relative path
 * Output     : absolute path, 0 on failure
 =======================================================================*/
char*
getAbsCachePath(Collection* coll, char* path) 
{
  char* rc = 0;

  checkCollection(coll);
  checkLabel(path);
  logMain(LOG_DEBUG, "getAbsCachePath %s", path);

  if (!(rc = createString(coll->cacheDir))) goto error;
  if (!(rc = catString(rc, "/"))) goto error;
  if (!(rc = catString(rc, path))) goto error;
  
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", "getAbsCachePath fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : absoluteRecordPath
 * Description: return record path from /
 * Synopsis   : char* absoluteRecordPath(Collection* coll, Record* record) 
 * Input      : Collection* coll = the related collection
 *              Record* record = the related record
 * Output     : path where to find the record file, 0 on failure
 =======================================================================*/
char*
getAbsRecordPath(Collection* coll, Record* record) 
{
  char* rc = 0;

  checkCollection(coll);
  checkRecord(record);
  logMain(LOG_DEBUG, "getAbsRecordPath: %s", strRecordType(record));

  switch (getRecordType(record)) {
  case FINALE_SUPPLY:
    if (!(rc = createString(record->extra))) goto error;
    break;
  case LOCALE_SUPPLY:
    if (!(rc = getAbsCachePath(coll, record->extra))) goto error;
    break;
  default:
    logMain(LOG_ERR, "cannot return path for %s", strRecordType(record));
  }
  
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", "getAbsRecordPath fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : scanFile
 * Description: Add, keep or remove a file into a collection cache
 * Synopsis   : int scanFile(RecordTree* self, char* coll, 
 *                            time_t now, char* path, char* relativePath) 
 * Input      : RecordTree* self = the record tree to update
 *              char* coll = the related collection
 *              char* path = where the file is
 *              char* relativePath = use for logging only
 *              int toKeep = tels the directory is to read-only
 * Output     : TRUE on success
 * Note       : Only one path for 2 files with same content is stored ?
 =======================================================================*/
int 
scanFile(Collection* coll, char* path, char* relativePath, int toKeep) 
{
  int rc = FALSE;
  RG* archives = 0;
  Archive* archive = 0;
  Record *record = 0;
  char* extra = 0;
  RGIT* curr = 0;
  struct stat statBuffer;
  Md5Data md5; 

  memset(&md5, 0, sizeof(Md5Data));

  if (isEmptyString(path)) {
    logMain(LOG_ERR, "%s", "please provide a path for the entry");
    goto error;
  }

  logMain(LOG_DEBUG, "scaning file: %s", path);

  // get file attributes (size)
  if (stat(path, &statBuffer)) {
    logMain(LOG_ERR, "status error on %s: %s", path, strerror(errno));
    goto error;
  }

  // first try to look for file in cache
  archives = coll->cacheTree->archives;
  while((archive = rgNext_r(archives, &curr)) != 0) {
    
    if (archive->size != statBuffer.st_size) continue;
    if ((record = archive->localSupply) == 0) continue;
    if (strcmp(record->extra, relativePath)) continue;

    // if already there do not compute the md5sums
    logMain(LOG_DEBUG, "%s match: md5sum skipped", relativePath);
    goto end;
  }

  // unknown file: compute hash
  md5.path = path;
  md5.size = statBuffer.st_size;
  md5.opp = MD5_CACHE_ID;
  if (!doMd5sum(&md5)) goto error;

  // assert we have the localhost server object
  if (!getLocalHost(coll)) goto error;
  
  // build and add a new cache entry
  if (!(archive = 
	addArchive(coll, md5.fullMd5sum, statBuffer.st_size))) 
    goto error;

  if (!(extra = createString(relativePath))) goto error;
  if (!(record = 
	addRecord(coll, coll->localhost, archive, SUPPLY, extra)))
    goto error;

  if (toKeep) {
    record->date = 0x7FFFFFFF; // maximum date: server will restart before
    logMain(LOG_DEBUG, "%s", 
	    "is read-only file (will never try to delete it)");
  }

  // add file as local supply and compute its archive status
  if (!addCacheEntry(coll, record)) goto error;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", "error scanning file");
  }
  return rc;
}

/*=======================================================================
 * Function   : scanRepository
 * Description: Recursively scan a cache directory 
 * Synopsis   : int scanRepository(RecordTree* self, 
 *                  Collection* collection, time_t now, const char* path) 
 * Input      : RecordTree* self = the record tree to update
 *              Collection* collection = the related collection
 *              const char* path = the directory path
 *              int toKeep = tels the directory is to read-only
 * Output     : RecordTree* self = the record tree updated
 *              TRUE on success
 * Note       : scandir assert the order (readdir_r do not)

 * MAYBE       : use symlinkTarget from device.c but scanRepository
	        is relative to the collection cache which is more secure
 =======================================================================*/
int 
scanRepository(Collection* coll, const char* path, int toKeep) 
{
  int rc = FALSE;
  struct dirent** entries;
  struct dirent* entry;
  int nbEntries = 0;
  int n = 0;
  char* absolutePath = 0;
  char* relativePath = 0;
  char* absolutePath2= 0;

  if (path == 0) {
    logMain(LOG_ERR, "%s", 
	    "please provide at least an empty string for path");
    goto error;
  }

  if ((absolutePath = createString(coll->cacheDir)) == 0 ||
      (absolutePath = catString(absolutePath, "/")) == 0 ||
      (absolutePath = catString(absolutePath, path)) == 0)
    goto error;

  logMain(LOG_INFO, "scaning directory: %s", absolutePath);

  entries = 0;
  if ((nbEntries 
       = scandir(absolutePath, &entries, 0, alphasort)) == -1) {
    logMain(LOG_ERR, "scandir fails on %s: %s", 
	    absolutePath, strerror(errno));
    goto error;
  }

  for (n=0; n<nbEntries; ++n) {
    entry = entries[n];
    if (!strcmp(entry->d_name, ".")) continue;
    if (!strcmp(entry->d_name, "..")) continue;
    if (!strcmp(entry->d_name, "toKeep")) toKeep = TRUE;

    if ((relativePath = createString(path)) == 0 ||
	(relativePath = catString(relativePath, entry->d_name)) == 0)
      goto error;
    
    //logMain(LOG_INFO, "scaning '%s'", relativePath);
    
      switch (entry->d_type) {

      case DT_DIR: 
	if ((relativePath = catString(relativePath, "/")) == 0 ||
	    !scanRepository(coll, relativePath, toKeep)) goto error;
	break;
	
      case DT_REG: 
	if ((absolutePath2 = createString(absolutePath)) == 0 ||
	    (absolutePath2 = catString(absolutePath2, entry->d_name)) 
	    == 0) goto error;
	if (!scanFile(coll, absolutePath2, relativePath, toKeep)) goto error;
	break;
	
      case DT_LNK:
	logMain(LOG_INFO, "%s", "do not handle simlink up today"); 
	break;

      case DT_UNKNOWN:
      default: 
	logMain(LOG_INFO, "ignore not regular file \'%s%s\'", 
		absolutePath, entry->d_name);
	break;
      }   
      
      relativePath = destroyString(relativePath);
      absolutePath2 = destroyString(absolutePath2);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails scaning directory: %s", absolutePath);
  }
  if (entries != 0) {
    for (n=0; n<nbEntries; ++n) {
      remind(entries[n]);
      free(entries[n]);
    }
    remind(entries);
    free(entries);
  }
  absolutePath = destroyString(absolutePath);
  return rc;
}

/*=======================================================================
 * Function   : updateLocalSupply
 * Description: scan caches and update local supply in md5sumsDB
 * Synopsis   : int updateLocalSupply(Collection* coll)
 * Input      : Collection* coll : collections to scan
 * Output     : TRUE on success
 =======================================================================*/
int 
quickScan(Collection* coll)
{
  int rc = FALSE;
  Archive* archive = 0;
  Archive* next = 0;
  char* path = 0;
  RGIT* curr = 0;
  Record* supply = 0;

  logMain(LOG_DEBUG, "updating cache for %s collection", coll->label);
  checkCollection(coll);

  if (!loadCollection(coll, CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;

  // add archive if new ones into the physical cache
  if (!scanRepository(coll, "", FALSE)) goto error3;

  // del cache entry if no more into the physical cache
  if (!(archive = rgNext_r(coll->cacheTree->archives, &curr))) goto end;
  do {
    next = rgNext_r(coll->cacheTree->archives, &curr);
    if ((supply = archive->localSupply) == 0) continue;

    // test if the file is really there 
    path = destroyString(path);
    if (!(path = getAbsRecordPath(coll, supply))) goto error3;
    if (access(path, R_OK) != -1) continue;

    // del the record from the cache
    logMain(LOG_WARNING, "file not found in cache as expected: %s", path);
    if (!delCacheEntry(coll, supply)) goto error3;
  } 
  while ((archive = next));
 end:
  rc = TRUE;
 error3:
  if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_INFO, "fails to update cache for %s collection%",
	    coll?coll->label:"?");
  } 
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : updateAllLocalSupply
 * Description: scan caches and update local supply in md5sumsDB
 * Synopsis   : int updateAllLocalSupply()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
quickScanAll(void)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "%s", "updating cache for all collections");

  // for all collection
  conf = getConfiguration();
  if (conf->collections != 0) {
    if (!expandConfiguration()) goto error;
    while((coll = rgNext_r(conf->collections, &curr)) != 0) {
      if (!quickScan(coll)) goto error;
      if (!cleanCacheTree(coll)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_INFO, "%s", "fails to update cache for all collections");
  } 
  return rc;
}

/*=======================================================================
 * Function   : cacheStatus
 * Description: log cache status
 * Synopsis   : static void cacheStatus(Collection* coll)
 * Input      : CacheTree* self
 * Output     : off_t* free : free place on cache
 *              off_t* available : free place if we remove perenials
 *                                 archives
 =======================================================================*/
static void cacheSizes(CacheTree* self, off_t* free, off_t* available)
{
  /*
  RG* archives = 0;
  Archive* archive = 0;
  Record* record = 0;
  RGIT* curr = 0;
  off_t used = 0;
  off_t frozen = 0;
  */

  *free = self->totalSize - self->useSize;
  *available = self->totalSize - self->frozenSize;

  logMain(LOG_INFO, "cache sizes:");
  logMain(LOG_INFO, "  total  %12llu", self->totalSize);
  logMain(LOG_INFO, "- used   %12llu", self->useSize);
  logMain(LOG_INFO, "= free   %12llu", *free);
  logMain(LOG_INFO, "  total  %12llu", self->totalSize);
  logMain(LOG_INFO, "- frozen %12llu", self->frozenSize);
  logMain(LOG_INFO, "= avail  %12llu", *available);

  // was usefull to debug cache content:
  // -----------------------------------
  /* archives = self->archives; */
  /* while ((archive = rgNext_r(archives, &curr))) { */
  /*   //if (archive->state < ) continue; */
  /*   record = archive->localSupply; */
  /*   logMain(LOG_INFO, "%-10s %20lli %s",  */
  /* 	    strAState(archive->state), */
  /* 	    archive->size, */
  /* 	    record?record->extra:"-"); */
  /*   if (archive->state >= ALLOCATED) used += archive->size; */
  /*   if (archive->state == TOKEEP) frozen += archive->size; */
  /* } */
  /* logMain(LOG_INFO, "used   = %12llu", used); */
  /* logMain(LOG_INFO, "frozen = %12llu", frozen); */
}

/*=======================================================================
 * Function   : freeAlloc
 * Description: try to make some place on cache
 * Synopsis   : static int 
 *              freeCache(Collection* coll, off_t need, int* success)
 * Input      : Collection* coll : collections to scan
 *              off_t need : the place we must found
 *              int* success : TRUE if there is room
 * Output     : FALSE on internal error
 =======================================================================*/
static int 
freeCache(Collection* coll, off_t need, int* success)
{
  int rc = FALSE;
  Configuration* conf = 0;
  CacheTree* self = 0;
  Archive* archive = 0;
  Record* record = 0;
  RG* archives = 0;
  off_t free = 0;
  off_t available = 0;
  time_t date = 0;
  char* path = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  logMain(LOG_DEBUG, "free the %s cache", coll->label);
  *success = FALSE; 
  if (!(conf = getConfiguration())) goto error;
  if (!(date = currentTime())) goto error;
  self = coll->cacheTree;

  // cache sizes
  cacheSizes(self, &free, &available);
  logMain(LOG_INFO, "need     %12llu", need);

  // different cases :
  if (need > self->totalSize) {
    logMain(LOG_WARNING, "%s's total cache size is to few", 
	    coll->label);
    goto end;
  }
  if (need > available) {
    logMain(LOG_NOTICE, "%s's cache have no more room available now", 
	    coll->label);
    goto end;
  }
  if (need <= free) {
    logMain(LOG_DEBUG, "%s's cache have room already available", 
	    coll->label);
    *success = TRUE;
    goto end;
  }

  // We will free some archive from the cache.
  // some optimisation should be great here: maybe sort on size or
  // date
#warning to optimize
  //if (!rgSort(tree->records, cmpRecordDate)) goto error;

  // - compute scores (do not delete archive with bad score)
  if (!computeExtractScore(coll)) goto error;

  // loop on all archives
  archives = coll->cacheTree->archives;
  while (need > free && (archive = rgNext_r(archives, &curr))) {

    // looking for archive that resides into the cache
    if (archive->state < AVAILABLE) continue;
    record = archive->localSupply;
    logMain(LOG_DEBUG, "check file: %s", record->extra);

    // looking for perenial archive we can safely delete from the cache
    if (!computeArchiveStatus(coll, archive)) goto error;
    if (archive->state >= TOKEEP) continue;

    if (!(path = getAbsRecordPath(coll, record))) goto error;

    // un-index the record from cache (do not free the record)
    if (!delCacheEntry(coll, record)) goto error;

    // free file on disk
    logMain(LOG_NOTICE, "unlink %s", path);
    if (!env.dryRun) {
      if (unlink(path) == -1) {
	logMain(LOG_ERR, "error with unlink %s:", strerror(errno));
	goto error;
      }
      path = destroyString(path);
    }

    free = self->totalSize - self->useSize;
  }
  
  cacheSizes(self, &free, &available);
 
  if (!(*success = (need <= free))) {
    logMain(LOG_NOTICE, "%s's cache have no more room available", 
	    coll->label);
  }
 end:
  rc = TRUE;
 error:
  if (!rc) {
    *success = FALSE;
    logMain(LOG_INFO, "no more room into %s cache", 
	    coll->label);
  } 
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : cacheAlloc
 * Description: try to make place on cache
 * Synopsis   : int cacheAlloc(Record** record, Collection* coll, 
 *                             Archive* archive)
 * Input      : Collection* collection : collections to scan
 *              Archive* archive : input archive to alloc
 *              int* success: TRUE on success
 * Output     : Record** record: the allocated record
 *              FALSE on internal error
 *
 * Note       : cacheAlloc use the MUTEX_ALLOC to be threads-safe
 *              and share this mutex with keepArchive/unKeepArchive
 *              (from cacheTree.c)
 =======================================================================*/
int
cacheAlloc(Record** record, Collection* coll, Archive* archive)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  int success = FALSE;
  char* extra = 0;
  int err = 0;

  checkCollection(coll);
  checkArchive(archive);
  cache = coll->cacheTree;
  logMain(LOG_DEBUG, "allocate new record into %s cache", coll->label);
  *record = 0; // default output (allocation fails)

  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  // look for already available record
  if (archive->state >= ALLOCATED) {
    logMain(LOG_WARNING, "%s", "archive is already allocated");
    goto error2;
  }

  // look for place in cache
  if (!freeCache(coll, archive->size, &success)) goto error2;
  if (!success) goto error2;

  // add record into trees
  if (success) {

    // assert we have the localhost server object
    if (!getLocalHost(coll)) goto error;

    // create the record object
    if (!(extra = createString("!malloc"))) goto error2;
    if (!(*record = addRecord(coll, coll->localhost, archive, 
			      SUPPLY, extra))) goto error2;
    extra = 0;

    // add the record into the cache
    if (!addCacheEntry(coll, *record)) goto error2;
  }

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
 error:
  if (!rc) {
    logMain(LOG_INFO, "fails add record into %s cache", coll->label);
    *record = destroyRecord(*record);
  } 
  extra = destroyString(extra);
  return rc;
}

/*=======================================================================
 * Function   : call access
 * Description: check a path access
 * Synopsis   : int callAcess(char* path)
 * Input      : char* path = the path to check
 * Output     : O on success, errno on failure
 * Note       : may be used to check if path IS NOT there
 =======================================================================*/
int
callAccess(char* path) 
{
  int rc = 0;

  checkLabel(path);
  logMain(LOG_INFO, "call access on %s", path);

  if (access(path, R_OK) != 0) {
    rc = errno;
    goto error;
  }

 error:
  if (rc) {
    logMain(LOG_INFO, "access fails: %s", strerror(errno));
  }
  return rc;
}

/*=======================================================================
 * Function   : makeDir
 * Description: build directories recursively
 * Synopsis   : int makeDir(char* path) 
 * Input      : char* base = directory path already there
 *              char* path = directory path to build
 * Output     : TRUE on success

 * TODO       : check mode of new created directory
 =======================================================================*/
int
makeDir(char* base, char* path, mode_t mode) 
{
  int rc = FALSE;
  int i = 0;
  int l = 0;
  mode_t mask;

  mask = umask(0000);
  checkLabel(path);
  logMain(LOG_DEBUG, "makeDir %s", path);

  // build the target directory into the cache
  i = strlen(base)+1;
  l = strlen(path);
  while (i<l) {
    while (i<l && path[i] != '/') ++i;
    if (path[i] == '/') {
      path[i] = (char)0;

      if (mkdir(path, mode)) { // (mode & ~umask & 0777)
	if (errno != EEXIST) {
	  logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
	  goto error;
	}
	else {
	  // already there. We should check mode.
	  if (callAccess(path)) goto error;
	}
      }
      
      path[i] = '/';
       ++i;
    }
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", "makeDir fails");
  }
  umask(mask);
  return rc;
}

/*=======================================================================
 * Function   : removeDir
 * Description: build directories recursively
 * Synopsis   : int removeDir(char* path) 
 * Input      : char* base = directory path already there
 *              char* path = directory path to build
 * Output     : path is modified: '/' are replaced by '\0'
 *              TRUE on success
 =======================================================================*/
int
removeDir(char* base, char* path) 
{
  int rc = FALSE;
  int i = 0;
  int l = 0;

  checkLabel(path);
  logMain(LOG_DEBUG, "removeDir %s", path);

  // remove the temporary extraction directory into the cache
  i = strlen(path)-1;
  l = strlen(base);

  // eat filename
  while (i>l && path[i] != '/') --i; 
  if (i <= l) goto end;
    path[i] = (char)0;

  while (--i > l) {
    while (i>l && path[i] != '/') --i;

    logMain(LOG_INFO, "rmdir %s", path);
    if (rmdir(path)) {
      if (errno != ENOTEMPTY || errno != EBUSY) {
	logMain(LOG_ERR, "rmdir fails: %s", strerror(errno));
	goto error;
      }
      else {
	// still used
	goto end;
      }
    }
    path[i] = (char)0;
  }
  
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", "removeDir fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : extractCp
 * Description: call cp
 * Synopsis   : int extractCp(char* source, char* target)
 * Input      : char* source
 *              char* target
 * Output     : TRUE on success
 =======================================================================*/
int 
extractCp(char* source, char* target)
{
  int rc = FALSE;
  char* argv[] = {"/bin/cp", "-f", 0, 0, 0};

  checkLabel(source);
  checkLabel(target);
  logMain(LOG_DEBUG, "extractCp %s", target);

  argv[2] = source;
  argv[3] = target;

  if (!env.dryRun && !execScript(argv, 0, 0, FALSE)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", "extractCp fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : cacheUpload
 * Description: try to upload a file on cache
 * Synopsis   : int cacheUpload(Collection* coll, off_t need, int* rcode)
 * Input      : Collection* collection : collections to use
 *              char* sourcePath: file to upload
 * Output     : TRUE on success
 =======================================================================*/
int
cacheUpload(Collection* coll, Record* record)
{
  int rc = FALSE;
  char* basename = 0;
  char targetRelDirname[24];
  char* targetRelPath = 0;
  char* targetAbsDirname = 0;
  char* targetAbsPath = 0;
  Record* record2 = 0;
  struct stat statBuffer;
  struct tm date;

  checkCollection(coll);
  if (isEmptyString(record->extra)) {
    logMain(LOG_ERR, "%s", "please provide a file to upload");
    goto error;
  }

  logMain(LOG_DEBUG, "upload file: %s", record->extra);

  // build target paths: .../upload/AAAAMM/basename
  if (localtime_r(&record->date, &date) == (struct tm*)0) {
    logMain(LOG_ERR, "%s", "localtime_r returns on error");
    goto error;
  }
  basename = strrchr(record->extra, '/');
  basename = (basename == 0) ? record->extra : (basename + 1);
  sprintf(targetRelDirname, "/incoming/%04i-%02i/",
	  date.tm_year + 1900, date.tm_mon+1);
  if (!(targetRelPath = createString(targetRelDirname + 1)) ||
      !(targetRelPath = catString(targetRelPath, basename)) ||
      !(targetAbsDirname = createString(coll->cacheDir)) ||
      !(targetAbsDirname = catString(targetAbsDirname, targetRelDirname))|| 
      !(targetAbsPath = createString(targetAbsDirname)) ||
      !(targetAbsPath = catString(targetAbsPath, basename))) 
    goto error;

  // assert target path is not already used
  if (stat(targetAbsPath, &statBuffer) == 0) {
    logMain(LOG_WARNING, "target path already exists: %s", targetAbsPath);
    goto end;
  }

  // assert archive is new
  if ((record->archive->localSupply) != 0) {
    logMain(LOG_WARNING, "%s", "archive already exists");
    goto end;
  }

  // ask for place in cache and build record
  if (!cacheAlloc(&record2, coll, record->archive)) goto error;

  // copy file into the cache
  if (!makeDir(coll->cacheDir, targetAbsDirname, 0750)) goto error;
  if (!extractCp(record->extra, targetAbsPath)) goto error;

  // toggle !malloc record to local-supply...
  record2->extra = destroyString(record2->extra);
  record2->extra = targetRelPath;
  targetRelPath = 0;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_WARNING, "%s", "fails to upload file");
    if (record2) delCacheEntry(coll, record2);
  } 
  targetRelPath = destroyString(targetRelPath);
  targetAbsDirname = destroyString(targetAbsDirname);
  targetAbsPath = destroyString(targetAbsPath);

  return rc;
}

/*=======================================================================
 * Function   : uploadFinaleArchive
 * Description: upload archive provided
 * Synopsis   : int uploadFinaleArchive(ArchiveTree* finalSupplies)
 * Input      : ArchiveTree* finalSupplies = support/colls we provide
 * Output     : TRUE on success
 =======================================================================*/
int 
uploadFinaleArchive(RecordTree* recordTree, Connexion* connexion)
{
  int rc = FALSE;
  Collection* coll = 0;
  Record* record = 0;
  Archive* archive = 0;
  (void) connexion;

  logMain(LOG_DEBUG, "%s", "work on upload message");

  // get the archiveTree's collection
  if ((coll = recordTree->collection) == 0) {
    logMain(LOG_ERR, "%s", "unknown collection for archiveTree");
    goto error;
  }
  
  // check we get final supplies
  if (isEmptyRing(recordTree->records)) {
    logMain(LOG_ERR, "%s", "please provide records for have query");
    goto error;
  }

  if (!loadCollection(coll, EXTR | CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;

  if (!(record = (Record*)recordTree->records->head->it)) goto error3;
  if (!(archive = record->archive)) goto error3;
  if (!(cacheUpload(coll, record))) goto error3;

  rc = TRUE;
  tcpWrite(connexion->sock, "200 ok\n", 7);
 error3:
  if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, EXTR | CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", "remote extraction fails");
    tcpWrite(connexion->sock, "500 fails\n", 10);
  }
  return rc;
} 

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
