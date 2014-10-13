/*=======================================================================
 * Version: $Id: cache.c,v 1.1 2014/10/13 19:39:52 nroche Exp $
 * Project: MediaTeX
 * Module : cache
 *
 * Manage local cache directory and DB

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
#include "../misc/command.h"
#include "../misc/md5sum.h"
#include "../misc/perm.h"
#include "../misc/tcp.h"
#include "../memory/confTree.h"
#include "../common/openClose.h"
#include "../common/extractScore.h"
#include "cache.h"

#include <sys/types.h> // umask
#include <sys/stat.h>
#include <dirent.h>

/*=======================================================================
 * Function   : getAbsCachePath
 * Description: get an absolute path to the cache
 * Synopsis   : char* absCachePath(Collection* coll, char* path) 
 * Input      : Collection* coll = the related collection
 *              char* path = the relative path
 * Output     : absolute path, NULL on failure
 =======================================================================*/
char*
getAbsCachePath(Collection* coll, char* path) 
{
  char* rc = NULL;

  checkCollection(coll);
  checkLabel(path);
  logEmit(LOG_DEBUG, "getAbsCachePath %s", path);

  if (!(rc = createString(coll->cacheDir))) goto error;
  if (!(rc = catString(rc, "/"))) goto error;
  if (!(rc = catString(rc, path))) goto error;
  
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "getAbsCachePath fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : absoluteRecordPath
 * Description: return record path from /
 * Synopsis   : char* absoluteRecordPath(Collection* coll, Record* record) 
 * Input      : Collection* coll = the related collection
 *              Record* record = the related record
 * Output     : path where to find the record file, NULL on failure
 =======================================================================*/
char*
getAbsRecordPath(Collection* coll, Record* record) 
{
  char* rc = NULL;

  checkCollection(coll);
  checkRecord(record);
  logEmit(LOG_DEBUG, "getAbsRecordPath: %s", strRecordType(record));

  switch (getRecordType(record)) {
  case FINALE_SUPPLY:
    if (!(rc = createString(record->extra))) goto error;
    break;
  case LOCALE_SUPPLY:
    if (!(rc = getAbsCachePath(coll, record->extra))) goto error;
    break;
  default:
    logEmit(LOG_ERR, "cannot return path for %s", strRecordType(record));
  }
  
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "getAbsRecordPath fails");
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
  RG* archives = NULL;
  Archive* archive = NULL;
  Record *record = NULL;
  char* extra = NULL;
  RGIT* curr = NULL;
  struct stat statBuffer;
  Md5Data md5; 

  memset(&md5, 0, sizeof(Md5Data));

  if (isEmptyString(path)) {
    logEmit(LOG_ERR, "%s", "please provide a path for the entry");
    goto error;
  }

  logEmit(LOG_DEBUG, "scaning file: %s", path);

  // get file attributes (size)
  if (stat(path, &statBuffer)) {
    logEmit(LOG_ERR, "status error on %s: %s", path, strerror(errno));
    goto error;
  }

  // first try to look for file in cache
  archives = coll->cacheTree->archives;
  while((archive = rgNext_r(archives, &curr)) != NULL) {
    
    if (archive->size != statBuffer.st_size) continue;
    if ((record = archive->localSupply) == NULL) continue;
    if (strcmp(record->extra, relativePath)) continue;

    // if already there do not compute the md5sums
    logEmit(LOG_DEBUG, "%s match: md5sum skipped", relativePath);
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
    logEmit(LOG_DEBUG, "%s", "is read-only file (will never try to delete it)");
  }

  if (!addCacheEntry(coll, record)) goto error;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "error scanning file");
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
 * Output     : RecordTree* self = the record tree updated
 *              TRUE on success
 * Note       : readdir_r ?
 *              scandir garanty the order
 =======================================================================*/
int 
scanRepositoryBackup(Collection* coll, const char* path, int toKeep) 
{
  int rc = FALSE;
  DIR* dir  = NULL;
  struct dirent* entry;
  char* absolutePath = NULL;
  char* relativePath = NULL;
  char* absolutePath2= NULL;

  if (path == NULL) {
    logEmit(LOG_ERR, "%s", 
	    "please provide at least an empty string for path");
    goto error;
  }

  if ((absolutePath = createString(coll->cacheDir)) == NULL ||
      (absolutePath = catString(absolutePath, "/")) == NULL ||
      (absolutePath = catString(absolutePath, path)) == NULL)
    goto error;

  logEmit(LOG_INFO, "scaning directory: %s", absolutePath);

  if ((dir = opendir(absolutePath)) == NULL) {
    logEmit(LOG_ERR, "cannot open cache directory \'%s\'", absolutePath);
    goto error;
  }

  rc = FALSE;
  errno = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".")) continue;
    if (!strcmp(entry->d_name, "..")) continue;
    if (!strcmp(entry->d_name, "toKeep")) toKeep = TRUE;

    if ((relativePath = createString(path)) == NULL ||
	(relativePath = catString(relativePath, entry->d_name)) == NULL)
      goto error;
  
    //logEmit(LOG_INFO, "scaning '%s'", relativePath);

    switch (entry->d_type) {
    case DT_DIR: 
      if ((relativePath = catString(relativePath, "/")) == NULL ||
	  !scanRepositoryBackup(coll, relativePath, toKeep)) goto error;
      errno = 0;
      break;

    case DT_REG: 
      if ((absolutePath2 = createString(absolutePath)) == NULL ||
	  (absolutePath2 = catString(absolutePath2, entry->d_name)) 
	  == NULL) goto error;
      if (!scanFile(coll, absolutePath2, relativePath, toKeep)) goto error;
      break;

    case DT_UNKNOWN:
    default: 
      logEmit(LOG_INFO, "ignore not regular file \'%s%s\'", 
	      absolutePath, entry->d_name);
      break;
    }   
    errno = 0; // see man readdir: return values
  }
  if (errno) {
    logEmit(LOG_ERR, "cannot read directory %s: %s", 
	    absolutePath, strerror(errno));
    goto error;
  }
  
  rc = TRUE;
 error:
  if (dir != NULL && closedir(dir) != 0) {
    logEmit(LOG_ERR, "cannot close directory: %s", strerror(errno)); 
  }
  if (!rc) {
    logEmit(LOG_ERR, "fails scaning directory: %s", absolutePath);
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
 * Note       : readdir_r ?
 *              scandir garanty the order
 =======================================================================*/
int 
scanRepository(Collection* coll, const char* path, int toKeep) 
{
  int rc = FALSE;
  struct dirent** entries;
  struct dirent* entry;
  int nbEntries = 0;
  int n = 0;
  char* absolutePath = NULL;
  char* relativePath = NULL;
  char* absolutePath2= NULL;

  if (path == NULL) {
    logEmit(LOG_ERR, "%s", 
	    "please provide at least an empty string for path");
    goto error;
  }

  if ((absolutePath = createString(coll->cacheDir)) == NULL ||
      (absolutePath = catString(absolutePath, "/")) == NULL ||
      (absolutePath = catString(absolutePath, path)) == NULL)
    goto error;

  logEmit(LOG_INFO, "scaning directory: %s", absolutePath);

  entries = 0;
  if ((nbEntries 
       = scandir(absolutePath, &entries, NULL, alphasort)) == -1) {
    logEmit(LOG_ERR, "scandir fails on %s: %s", 
	    absolutePath, strerror(errno));
    goto error;
  }

  for (n=0; n<nbEntries; ++n) {
    entry = entries[n];
    if (!strcmp(entry->d_name, ".")) continue;
    if (!strcmp(entry->d_name, "..")) continue;
    if (!strcmp(entry->d_name, "toKeep")) toKeep = TRUE;

    if ((relativePath = createString(path)) == NULL ||
	(relativePath = catString(relativePath, entry->d_name)) == NULL)
      goto error;
    
    //logEmit(LOG_INFO, "scaning '%s'", relativePath);
    
      switch (entry->d_type) {

      case DT_DIR: 
	if ((relativePath = catString(relativePath, "/")) == NULL ||
	    !scanRepository(coll, relativePath, toKeep)) goto error;
	break;
	
      case DT_REG: 
	if ((absolutePath2 = createString(absolutePath)) == NULL ||
	    (absolutePath2 = catString(absolutePath2, entry->d_name)) 
	    == NULL) goto error;
	if (!scanFile(coll, absolutePath2, relativePath, toKeep)) goto error;
	break;
	
      case DT_LNK:
	logEmit(LOG_INFO, "%s", "do not handle simlink up today"); 
	/* todo: use symlinkTarget from device.c but scanRepository
	   is relative to the collection cache whichnis more secure */
	break;

      case DT_UNKNOWN:
      default: 
	logEmit(LOG_INFO, "ignore not regular file \'%s%s\'", 
		absolutePath, entry->d_name);
	break;
      }   
      
      relativePath = destroyString(relativePath);
      absolutePath2 = destroyString(absolutePath2);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "fails scaning directory: %s", absolutePath);
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
  Archive* archive = NULL;
  Archive* next = NULL;
  char* path = NULL;
  RGIT* curr = NULL;
  Record* supply = NULL;

  logEmit(LOG_DEBUG, "updating cache for %s collection", coll->label);
  checkCollection(coll);

  if (!loadCollection(coll, CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;

  // add archive if new ones into the physical cache
  if (!scanRepository(coll, "", FALSE)) goto error3;

  // del cache entry if no more into the physical cache
  if (!(archive = rgNext_r(coll->cacheTree->archives, &curr))) goto end;
  do {
    next = rgNext_r(coll->cacheTree->archives, &curr);
    if ((supply = archive->localSupply) == NULL) continue;

    // test if the file is really there 
    path = destroyString(path);
    if (!(path = getAbsRecordPath(coll, supply))) goto error3;
    if (access(path, R_OK) != -1) continue;

    // del the record from the cache
    logEmit(LOG_WARNING, "file not found in cache as expected: %s", path);
    if (!delCacheEntry(coll, supply)) goto error3;
    //    if (!delRecord(coll, supply)) goto error4;
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
    logEmit(LOG_INFO, "fails to update cache for %s collection%",
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
  Configuration* conf = NULL;
  Collection* coll = NULL;
  RGIT* curr = NULL;

  logEmit(LOG_DEBUG, "%s", "updating cache for all collections");

  // for all collection
  conf = getConfiguration();
  if (conf->collections != NULL) {
    if (!expandConfiguration()) goto error;
    while((coll = rgNext_r(conf->collections, &curr)) != NULL) {
      if (!quickScan(coll)) goto error;
      if (!cleanCacheTree(coll)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_INFO, "%s", "fails to update cache for all collections");
  } 
  return rc;
}

/*=======================================================================
 * Function   : freeAlloc
 * Description: try to make some place on cache
 * Synopsis   : static int 
 *              freeCache(Collection* coll, off_t need, int* success)
 * Input      : Collection* collection : collections to scan
 *              off_t need : the place we must found
 *              int* success : TRUE if there is room
 * Output     : FALSE on internal error
 =======================================================================*/
static int 
freeCache(Collection* coll, off_t need, int* success)
{
  int rc = FALSE;
  Configuration* conf = NULL;
  CacheTree* self = NULL;
  Archive* archive = NULL;
  //Archive* next = NULL;
  Record* record = NULL;
  RG* archives = NULL;
  off_t free = 0;
  off_t available = 0;
  time_t date = 0;
  char* path = NULL;
  RGIT* curr = NULL;

  logEmit(LOG_DEBUG, "free the %s cache", coll->label);
  *success = FALSE; 
  if (!(conf = getConfiguration())) goto error;
  if (!(date = currentTime())) goto error;
  self = coll->cacheTree;

  // cache sizes
  free = self->totalSize - self->useSize;
  available = self->totalSize - self->frozenSize;
  logEmit(LOG_INFO, "need   = %12llu", need);
  logEmit(LOG_INFO, "total  = %12llu", self->totalSize);
  logEmit(LOG_INFO, "use    = %12llu", self->useSize);
  logEmit(LOG_INFO, "free   = %12llu", free);
  logEmit(LOG_INFO, "frozen = %12llu", self->frozenSize);
  logEmit(LOG_INFO, "avail  = %12llu", available);

  // different cases :
  if (need > self->totalSize) {
    logEmit(LOG_WARNING, "%s's total cache size is to few", 
	    coll->label);
    goto end;
  }
  if (need > available) {
    logEmit(LOG_NOTICE, "%s's cache have no more room available now", 
	    coll->label);
    goto end;
  }
  if (need <= free) {
    logEmit(LOG_DEBUG, "%s's cache have room already available", 
	    coll->label);
    *success = TRUE;
    goto end;
  }

  // We will free some archive from the cache.
  // some optimisation shoule great here: maybe sort on size ...
  // but better sort on date (for tokeep...)
  //if (!rgSort(tree->records, cmpRecordDate)) goto error;

  // - compute scores (do not delete archive with bad score)
  if (!computeExtractScore(coll)) goto error;

  // loop on all archives
  archives = coll->cacheTree->archives;
  while (need > free && (archive = rgNext_r(archives, &curr))) {

    // - orphane LOCAL_SUPPLY (not to keep nor final)
    if (archive->state < AVAILABLE) continue;
    record = archive->localSupply;
    if (record->date > date) continue; // toKeep
    
    // - archive known in extract.txt and own a good score
    if (archive->extractScore <= coll->serverTree->scoreParam.badScore)
      continue;

    if (!(path = getAbsRecordPath(coll, record))) goto error;

    // un-index the record from cache (do not free the record)
    if (!delCacheEntry(coll, record)) goto error;

    // free file on disk
    logEmit(LOG_NOTICE, "unlink %s", path);
    if (!env.dryRun) {
      if (unlink(path) == -1) {
	logEmit(LOG_ERR, "error with unlink %s:", strerror(errno));
	goto error;
      }
      path = destroyString(path);
    }

    free = self->totalSize - self->useSize;    
  }
  free = self->totalSize - self->useSize;
 
  if (!(*success = (need <= free))) {
    logEmit(LOG_INFO, "*free* = %12llu", free);
    logEmit(LOG_NOTICE, "%s's cache have no more room available now", 
	    coll->label);
  }
 end:
  rc = TRUE;
 error:
  if (!rc) {
    *success = FALSE;
    logEmit(LOG_INFO, "no more room into %s cache", 
	    coll->label);
  } 
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : cacheAlloc
 * Description: try to make place on cache
 * Synopsis   : int cacheAlloc(Record** record, Collection* coll, Archive* archive)
 * Input      : Collection* collection : collections to scan
 *              Archive* archive : input archive to alloc
 *              int* success: TRUE on success
 * Output     : Record** record: the allocated record
 *              FALSE on internal error
 =======================================================================*/
int
cacheAlloc(Record** record, Collection* coll, Archive* archive)
{
  int rc = FALSE;
  CacheTree* cache = NULL;
  int success = FALSE;
  char* extra = NULL;
  int err = 0;

  checkCollection(coll);
  checkArchive(archive);
  cache = coll->cacheTree;
  logEmit(LOG_DEBUG, "allocate new record into %s cache", coll->label);
  *record = NULL; // default output (allocation fails)

  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_ALLOC]))) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  // look for already available record
  if (archive->state >= ALLOCATED) {
    logEmit(LOG_WARNING, "%s", "archive is already allocated");
    goto error2;
  }

  // look for place in cache
  if (!freeCache(coll, archive->size, &success)) goto error2;
  if (!success) goto error2;

  // add record into trees
  if (success) {

    // create the record object
    if (!(extra = createString("!malloc"))) goto error2;
    if (!(*record = addRecord(coll, coll->localhost, archive, 
			      SUPPLY, extra))) goto error2;
    extra = NULL;

    // add the record into the cache
    if (!addCacheEntry(coll, *record)) goto error2;
  }

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_ALLOC]))) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
 error:
  if (!rc) {
    logEmit(LOG_INFO, "fails add record into %s cache", coll->label);
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
  logEmit(LOG_INFO, "call access on %s", path);

  if (access(path, R_OK) != 0) {
    rc = errno;
    goto error;
  }

 error:
  if (rc) {
    logEmit(LOG_INFO, "access fails: %s", strerror(errno));
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
 =======================================================================*/
int
makeDir(char* base, char* path, mode_t mode) 
{
  int rc = FALSE;
  int i = 0;
  int l = 0;
  mode_t mask;

  checkLabel(path);
  logEmit(LOG_DEBUG, "makeDir %s", path);
  mask = umask(0000);

  // build the target directory into the cache
  i = strlen(base)+1;
  l = strlen(path);
  while (i<l) {
    while (i<l && path[i] != '/') ++i;
    if (path[i] == '/') {
      path[i] = (char)0;

      if (mkdir(path, mode)) { // (mode & ~umask & 0777)
	if (errno != EEXIST) {
	  logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
	  goto error;
	}
	else {
	  // already there. TODO: check mode
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
    logEmit(LOG_ERR, "%s", "makeDir fails");
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
  logEmit(LOG_DEBUG, "removeDir %s", path);

  // remove the temporary extraction directory into the cache
  i = strlen(path)-1;
  l = strlen(base);

  // eat filename
  while (i>l && path[i] != '/') --i; 
  if (i <= l) goto end;
    path[i] = (char)0;

  while (--i > l) {
    while (i>l && path[i] != '/') --i;

    logEmit(LOG_INFO, "rmdir %s", path);
    if (rmdir(path)) {
      if (errno != ENOTEMPTY || errno != EBUSY) {
	logEmit(LOG_ERR, "rmdir fails: %s", strerror(errno));
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
    logEmit(LOG_ERR, "%s", "removeDir fails");
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
  char* argv[] = {"/bin/cp", "-f", NULL, NULL, NULL};

  checkLabel(source);
  checkLabel(target);
  logEmit(LOG_DEBUG, "extractCp %s", target);

  argv[2] = source;
  argv[3] = target;

  if (!env.dryRun && !execScript(argv, NULL, NULL, FALSE)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "extractCp fails");
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
  char* basename = NULL;
  char targetRelDirname[24];
  char* targetRelPath = NULL;
  char* targetAbsDirname = NULL;
  char* targetAbsPath = NULL;
  Record* record2 = NULL;
  struct stat statBuffer;
  struct tm date;

  checkCollection(coll);
  if (isEmptyString(record->extra)) {
    logEmit(LOG_ERR, "%s", "please provide a file to upload");
    goto error;
  }

  logEmit(LOG_DEBUG, "upload file: %s", record->extra);

  // build target paths: .../upload/AAAAMM/basename
  if (localtime_r(&record->date, &date) == (struct tm*)0) {
    logEmit(LOG_ERR, "%s", "localtime_r returns on error");
    goto error;
  }
  basename = strrchr(record->extra, '/');
  basename = (basename == NULL) ? record->extra : (basename + 1);
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
    logEmit(LOG_WARNING, "target path already exists: %s", targetAbsPath);
    goto error;
  }

  // assert archive is new
  if ((record->archive->localSupply) != NULL) {
    logEmit(LOG_WARNING, "%s", "archive already exists");
    goto error;
  }

  // ask for place in cache and build record
  if (!cacheAlloc(&record2, coll, record->archive)) goto error;

  // copy file into the cache
  if (!makeDir(coll->cacheDir, targetAbsDirname, 0750)) goto error;
  if (!extractCp(record->extra, targetAbsPath)) goto error;

  // toggle !malloc record to local-supply...
  record2->extra = destroyString(record2->extra);
  record2->extra = targetRelPath;
  targetRelPath = NULL;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_WARNING, "%s", "fails to upload file");
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
  Collection* coll = NULL;
  Record* record = NULL;
  Archive* archive = NULL;
  (void) connexion;

  logEmit(LOG_DEBUG, "%s", "work on upload message");

  // get the archiveTree's collection
  if ((coll = recordTree->collection) == NULL) {
    logEmit(LOG_ERR, "%s", "unknown collection for archiveTree");
    goto error;
  }
  
  // check we get final supplies
  if (isEmptyRing(recordTree->records)) {
    logEmit(LOG_ERR, "%s", "please provide records for have query");
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
    logEmit(LOG_ERR, "%s", "remote extraction fails");
    tcpWrite(connexion->sock, "500 fails\n", 10);
  }
  return rc;
} 

/************************************************************************/

#ifdef utMAIN
#include "utFunc.h"

#include <sys/types.h> // 
#include <sys/stat.h>  // open 
#include <fcntl.h>     //
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
 * modif      : 2012/05/01
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
  Archive* archive = NULL;
  Record* record = NULL;
  off_t size = 0;
  char* extra = NULL;
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
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll = mdtxGetCollection("coll3"))) goto error;  

  utLog("%s", "Clean the cache:", NULL);
  if (!utCleanCaches()) goto error;

  utLog("%s", "Scan the cache directory :", NULL);
  if (!utCopyFileOnCache(coll, inputRep, "logo.png")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tgz")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.iso")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.iso")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan gives :", coll);

  utLog("%s", "Keep logoP1.iso and logoP2.iso:", NULL);
  if (!(archive =
  	getArchive(coll, "de5008799752552b7963a2670dc5eb18", 391168)))
    goto error;
  if (!keepArchive(coll, archive, 0)) goto error;
  if (!(archive =
  	getArchive(coll, "0a7ecd447ef2acb3b5c6e4c550e6636f", 374784)))
    goto error;
  if (!keepArchive(coll, archive, 0)) goto error;
  utLog("%s", "Now we have :", coll);

  /*--------------------------------------------------------*/
  utLog("%s", "Ask for too much place :", NULL);
  size = coll->cacheTree->totalSize - 2*KILO;
  if (!(archive = addArchive(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  			     size))) goto error;
  if (cacheAlloc(&record, coll, archive)) goto error;
  utLog("reply : %i", (void*)(record!=NULL), coll); // 0: too much
  
  /*--------------------------------------------------------*/
  utLog("%s", "Ask for little place so do not suppress anything :", NULL);
  size = coll->cacheTree->totalSize;
  size -= coll->cacheTree->useSize; // free size
  size -= 2*KILO;
  archive->size = size;
  record = NULL;
  if (!cacheAlloc(&record, coll, archive)) goto error;
  record->extra[0]='*';
  utLog("reply : %i", (void*)(record!=NULL), coll); // 1: already avail
  if (!delCacheEntry(coll, record)) goto error;
  if (!delRecord(coll, record)) goto error;

  /*--------------------------------------------------------*/
  utLog("%s", "Ask for place so as we need to suppress files :", NULL);
  size = coll->cacheTree->totalSize;
  size -= coll->cacheTree->frozenSize; // available size
  size -= 2*KILO;
  if (!(archive = addArchive(coll, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
  			    size))) goto error;
  record = NULL;
  if (!cacheAlloc(&record, coll, archive)) goto error;
  record->extra[0]='*';
  utLog("reply : %i", (void*)(record!=NULL), coll); // 1: del some entries

  /*--------------------------------------------------------*/
  utLog("%s", "API to upload files :", NULL);
  if (!(extra = createString(inputRep))) goto error;
  if (!(extra = catString(extra, "/../../examples/"))) goto error;
  if (!(extra = catString(extra, "README"))) goto error;
  if (!(archive = addArchive(coll, "3f18841537668dcf4fafd1471c64d52d",
  			    1937))) goto error;
  if (!(record = addRecord(coll, coll->localhost, archive, SUPPLY, extra)))
    goto error;
  extra = NULL;
  if (!cacheUpload(coll, record)) goto error;
  utLog("reply : %s", "upload ok", coll);

  /*--------------------------------------------------------*/
  utLog("%s", "Clean the cache:", NULL);
  if (!utCleanCaches()) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  extra = destroyString(extra);
  record = destroyRecord(record);
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
