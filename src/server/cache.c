/*=======================================================================
 * Version: $Id: cache.c,v 1.16 2015/08/23 23:39:16 nroche Exp $
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
 * Function   : getAbsoluteCachePath
 * Description: get an absolute path to the cache
 * Synopsis   : char* absCachePath(Collection* coll, char* path) 
 * Input      : Collection* coll = the related collection
 *              char* path = the relative path
 * Output     : absolute path, 0 on failure
 =======================================================================*/
char*
getAbsoluteCachePath(Collection* coll, char* path) 
{
  char* rc = 0;

  checkCollection(coll);
  checkLabel(path);
  logMain(LOG_DEBUG, "getAbsoluteCachePath %s", path);

  if (!(rc = createString(coll->cacheDir))) goto error;
  if (!(rc = catString(rc, "/"))) goto error;
  if (!(rc = catString(rc, path))) goto error;
  
 error:
  if (!rc) {
    logMain(LOG_ERR, "getAbsoluteCachePath fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getFinalSupplyInPath
 * Description: Get final supply's source path
 * Synopsis   : char* getFinalSupplyInPath(char* path) 
 * Input      : Collection* coll = the related collection
 *              char* path = the relative path
 * Output     : NULL on failure
 =======================================================================*/
char*
getFinalSupplyInPath(char* path) 
{
  char* rc = 0;
  char* ptr = 0;

  checkLabel(path);
  logMain(LOG_DEBUG, "getFinalSupplyInputPath %s", path);

  if (!(rc = createString(path))) goto error;

  // remove ending ':supportName' if there
  for (ptr=rc; *ptr && *ptr != ':'; ++ptr);
  if (*ptr) *ptr = 0;

 error:
  if (!rc) {
    logMain(LOG_ERR, "getFinalSupplyInputPath fails");
  }
  return rc;
}
/*=======================================================================
 * Function   : getFinalSupplyOutPath
 * Description: remove ending ':supportName' if there
 * Synopsis   : char* getFinalSupplyOutPath(char* path) 
 * Input      : char* path = the relative path
 * Output     : NULL on failure
 =======================================================================*/
char*
getFinalSupplyOutPath(char* path) 
{
  char* rc = 0;
  char* ptr = 0;
  char* tmp = 0;

  checkLabel(path);
  logMain(LOG_DEBUG, "getFinalSupplyOutPath %s", path);

  // look for ending ':supportName' if there
  for (ptr=path; *ptr && *ptr != ':'; ++ptr);
  if (*ptr) {
    if (!(rc = createString(ptr+1))) goto error;
  }

  // get the filename on first part
  while (ptr > path && *ptr-- != '/');
  if (!(tmp = createString(ptr))) goto error;
   
  // no second part or second part is a directory
  if (!tmp || tmp[strlen(tmp)] == '/') {  
    rc = catString(rc, tmp);
    tmp = 0;
  }

 error:
  if (!rc) {
    logMain(LOG_ERR, "getFinalSupplyOutPath fails");
  }
  destroyString(tmp);
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
getAbsoluteRecordPath(Collection* coll, Record* record) 
{
  char* rc = 0;

  checkCollection(coll);
  checkRecord(record);
  logMain(LOG_DEBUG, "getAbsoluteRecordPath: %s", strRecordType(record));

  switch (getRecordType(record)) {
  case FINAL_SUPPLY:
    if (!(rc = getFinalSupplyInPath(record->extra))) goto error;
    break;
  case LOCAL_SUPPLY:
    if (!(rc = getAbsoluteCachePath(coll, record->extra))) goto error;
    break;
  default:
    logMain(LOG_ERR, "cannot return path for %s", strRecordType(record));
  }
  
 error:
  if (!rc) {
    logMain(LOG_ERR, "getAbsoluteRecordPath fails");
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
    logMain(LOG_ERR, "please provide a path for the entry");
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
  while((archive = rgNext_r(archives, &curr))) {
    
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
    logMain(LOG_DEBUG, 
	    "is read-only file (will never try to delete it)");
  }

  // add file as local supply and compute its archive status
  if (!addCacheEntry(coll, record)) goto error;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "error scanning file");
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
    logMain(LOG_ERR, 
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
      logMain(LOG_INFO, "do not handle simlink up today"); 
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
  if (entries) {
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
    if (!(path = getAbsoluteRecordPath(coll, supply))) goto error3;
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

  logMain(LOG_DEBUG, "updating cache for all collections");

  // for all collection
  conf = getConfiguration();
  if (conf->collections) {
    if (!expandConfiguration()) goto error;
    while((coll = rgNext_r(conf->collections, &curr))) {
      if (!quickScan(coll)) goto error;
      if (!cleanCacheTree(coll)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_INFO, "fails to update cache for all collections");
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

    if (!(path = getAbsoluteRecordPath(coll, record))) goto error;

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
    logMain(LOG_WARNING, "archive is already allocated");
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


/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
