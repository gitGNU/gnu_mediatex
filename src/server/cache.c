/*=======================================================================
 * Project: MediaTeX
 * Module : cache
 *
 * Manage local cache directory and DB

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
 * Synopsis   : char* getFinalSupplyInPath(char* extra) 
 * Input      : Collection* coll = the related collection
 *              char* extra = the extra record's attribute
 * Output     : Allocated string with first part or NULL on failure
 =======================================================================*/
char*
getFinalSupplyInPath(char* extra) 
{
  char* rc = 0;
  char* ptrColon = 0;
  char car = 0;

  checkLabel(extra);
  logMain(LOG_DEBUG, "getFinalSupplyInputPath %s", extra);

  // look for ':' that limit part1 and part2, if there
  for (ptrColon=extra; *ptrColon && *ptrColon != ':'; ++ptrColon);

  // get first part
  car = *ptrColon;
  *ptrColon = 0;
  if (!(rc = createString(extra))) goto error;
  *ptrColon = car;

 error:
  if (!rc) {
    logMain(LOG_ERR, "getFinalSupplyInputPath fails");
  }
  return rc;
}
/*=======================================================================
 * Function   : getFinalSupplyOutPath
 * Description: remove ending ':supportName' if there
 * Synopsis   : char* getFinalSupplyOutPath(char* extra) 
 * Input      : char* extra = the extra record's attribute
 * Output     : Allocated string based on second part or NULL on failure
 =======================================================================*/
char*
getFinalSupplyOutPath(char* extra) 
{
  char* rc = 0;
  char* ptrOut = 0;
  char* ptrColon = 0;
  char* ptrPart2 = 0;
  char* ptrFilename = 0;
  char car = 0;

  checkLabel(extra);
  logMain(LOG_DEBUG, "getFinalSupplyOutPath %s", extra);

  // look for ':' that limit part1 and part2, if there
  for (ptrColon=extra; *ptrColon && *ptrColon != ':'; ++ptrColon);

  // get original part2
  if (*ptrColon) ptrPart2 = ptrColon+1;

  // split parts
  car = *ptrColon;
  *ptrColon = 0;

  // set output to "PART2"
  if (!isEmptyString(ptrPart2)) {
    if (!(ptrOut = createString(ptrPart2))) goto error;
  }

  // if no part2 or part2 is a directory
  if (isEmptyString(ptrPart2) || ptrPart2[strlen(ptrPart2)-1] == '/') {

    // get the filename on first part
    for (ptrFilename = ptrColon -1; 
	 ptrFilename > extra && *ptrFilename != '/';
	 --ptrFilename);
    
    // set output to "/supports/PART2/FILE_NAME"
    if (!(ptrOut = catString(ptrOut, ptrFilename+1))) goto error;
  }

  rc = ptrOut;
  ptrOut = 0;
 error:
  if (!rc) {
    logMain(LOG_ERR, "getFinalSupplyOutPath fails");
  }
  *ptrColon = car; // unsplit parts
  destroyString(ptrOut);
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
 * Synopsis   : static int scanFile(char* coll, 
 *                               char* absolutePath, char* relativePath) 
 * Input      : char* coll
 *              char* absolutePath
 *              char* relativePath
 * Output     : TRUE on success
#warning DO NO unlink support files !
 =======================================================================*/
static int 
scanFile(Collection* coll, char* absolutePath, char* relativePath) 
{
  int rc = FALSE;
  Archive* archive = 0;
  Record *record = 0;
  AVLNode* node = 0;
  struct stat statBuffer;
  CheckData md5; 
  char* extra = 0;

  logMain(LOG_DEBUG, "scaning file: %s", relativePath);
  checkLabel(absolutePath);
  checkLabel(relativePath);
  memset(&md5, 0, sizeof(CheckData));

  // get file attributes (size)
  if (stat(absolutePath, &statBuffer)) {
    logMain(LOG_ERR, "status error on %s: %s", 
	    absolutePath, strerror(errno));
    goto error;
  }

  // remove empty file from cache
  if (!statBuffer.st_size &&
      *relativePath != '/') { // do not unlink final supplies
    logMain(LOG_WARNING, "remove %s from cache (empty file)", relativePath);
    if (!env.dryRun) {
      if (unlink(absolutePath) == -1) {
	logMain(LOG_ERR, "error with unlink %s:", strerror(errno));
	goto error;
      }
    }
    goto end;
  }

  // try to match archive with them loaded from md5sums file
  //  O(n2) : not so much better than to compute the md5sum !!
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    if (archive->size == statBuffer.st_size &&
	(record = archive->localSupply) &&
	!strcmp(record->extra, relativePath)) {
      
      // we guess it is already there: do not compute the md5sums
      logMain(LOG_INFO, "%s match: md5sum skipped", relativePath);
      record = 0;
      goto end;
    }
  }
  record = 0;

  // unknown file: compute hash
  md5.path = absolutePath;
  md5.size = statBuffer.st_size;
  md5.opp = CHECK_CACHE_ID;
  if (!doChecksum(&md5)) goto error;

  // archive should adready exists on extraction metadata
  if (!(archive = getArchive(coll, md5.fullMd5sum, statBuffer.st_size))
      && *relativePath != '/') { // do not unlink final supplies
    
    // remove file from cache if not having extraction rule
    logMain(LOG_WARNING, "remove %s from cache (having no extract rule)", 
	    relativePath);
    if (!env.dryRun) {
      if (unlink(absolutePath) == -1) {
	logMain(LOG_ERR, "error with unlink %s:", strerror(errno));
	goto error;
      }
    }
    goto end;
  } 

  // remove doublons
  if (archive->localSupply && 
      !(archive->localSupply->type & REMOVE)
      && *relativePath != '/') { // do not unlink final supplies
    logMain(LOG_WARNING, "remove %s from cache (doublon)", relativePath);
    if (!env.dryRun) {
      if (unlink(absolutePath) == -1) {
	logMain(LOG_ERR, "error with unlink %s:", strerror(errno));
	goto error;
      }
    }
    goto end;
  }

  // assert we have the localhost server object
  if (!getLocalHost(coll)) goto error;
    
  // add a new cache entry
  if (!(extra = createString(relativePath))) goto error;

  // cat ":CONF_SUPPD/" to extra for final supplies
  if (*relativePath == '/') {
    if (!(extra = catString(extra, CONF_SUPPD))) goto error;
  }

  if (!(record = 
	addRecord(coll, coll->localhost, archive, SUPPLY, extra)))
    goto error;

  // add file as local supply and compute its archive status
  if (!addCacheEntry(coll, record)) goto error;
  record = 0;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "error scanning file");
  }
  if (record) delRecord(coll, record);
  return rc;
}

/*=======================================================================
 * Function   : scanRepository
 * Description: Recursively scan a cache directory 
 * Synopsis   : static int 
 *              scanRepository(Collection* collection, const char* path) 
 * Input      : Collection* collection = the related collection
 *              const char* path = the directory path
 * Output     : TRUE on success
 * Note       : scandir assert the order (readdir_r do not),
 *              this make non-regression tests easier.
 =======================================================================*/
static int 
scanRepository(Collection* coll, const char* path) 
{
  int rc = FALSE;
  struct dirent** entries;
  struct dirent* entry;
  int nbEntries = 0;
  int n = 0;
  char* absolutePath = 0;
  char* relativePath = 0;
  char* absolutePath2= 0;

  logMain(LOG_DEBUG, "scanRepository %s", path);

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

    if ((relativePath = createString(path)) == 0 ||
	(relativePath = catString(relativePath, entry->d_name)) == 0)
      goto error;
    
    // loop on sub directories
    switch (entry->d_type) {

    case DT_DIR: 
      if ((relativePath = catString(relativePath, "/")) == 0 ||
	  !scanRepository(coll, relativePath)) goto error;
      break;
	
    case DT_REG:
      if (!strcmp(entry->d_name, ".htaccess")) {
	  logMain(LOG_DEBUG, "ignore .htaccess files");
	  break;
	}
	  
      if ((absolutePath2 = createString(absolutePath)) == 0 ||
	  (absolutePath2 = catString(absolutePath2, entry->d_name)) 
	  == 0) goto error;
      if (!scanFile(coll, absolutePath2, relativePath)) goto error;
      break;
	
    case DT_LNK:
      logMain(LOG_DEBUG, "ignore symbolic link"); 
      break;

    case DT_UNKNOWN:
    default: 
      logMain(LOG_WARNING, "ignore irregular file \'%s%s\'", 
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
 * Function   : scanSupportFiles
 * Description: scan files provided by support.txt
 * Synopsis   : static int quickScan(Collection* coll)
 * Input      : Collection* coll : collections to scan
 * Output     : TRUE on success
 =======================================================================*/
static int 
scanSupportFiles(Collection* coll)
{
  int rc = FALSE;
  Archive* archive = 0;
  Record* supply = 0;
  Support* supp = 0;
  AVLNode* node = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "scanSupportFiles");
  checkCollection(coll);

  if (!loadConfiguration(CFG)) goto error;

  // start by removing all final-supplies from the cache
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    curr = 0;
    while ((supply = rgNext_r(archive->finalSupplies, &curr))) {
      if (!delCacheEntry(coll, supply)) goto error;
    }
  } 

  // loop on support files to (re-)add final supplies
  curr = 0;
  while ((supp = rgNext_r(coll->supports, &curr))) {
    if (*supp->name != '/') continue;
    if (!scanFile(coll, supp->name, supp->name)) goto error;    
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_INFO, "scanSupportFiles fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : quickScan
 * Description: scan caches and update local supply in md5sumsDB
 * Synopsis   : int quickScan(Collection* coll)
 * Input      : Collection* coll : collections to scan
 * Output     : TRUE on success
 =======================================================================*/
int 
quickScan(Collection* coll)
{
  int rc = FALSE;
  Archive* archive = 0;
  char* path = 0;
  Record* supply = 0;
  AVLNode* node = 0;

  logMain(LOG_DEBUG, "quickScan");
  checkCollection(coll);

  if (!loadCollection(coll, EXTR|CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;

  // add archive if new ones founded
  if (!scanRepository(coll, "")) goto error3;
  if (!scanSupportFiles(coll)) goto error3;

  // del cache entry if no more into the physical cache
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    if ((supply = archive->localSupply) == 0) continue;

    // test if the file is really there 
    path = destroyString(path);
    if (!(path = getAbsoluteRecordPath(coll, supply))) goto error3;
    if (access(path, R_OK) != -1) continue;

    // del the record from the cache
    logMain(LOG_WARNING, "file not found in cache as expected: %s", path);
    if (!delCacheEntry(coll, supply)) goto error3;
  } 

  rc = TRUE;
 error3:
  if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, EXTR|CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_INFO, "quickScan fails");
  } 
  destroyString(path);
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
    while ((coll = rgNext_r(conf->collections, &curr))) {
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
 * Synopsis   : static void cacheSize(CacheTree* self, 
 *                                    off_t* free, off_t* available)
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
  AVLNode* node = 0;
  off_t free = 0;
  off_t available = 0;
  time_t date = 0;
  char* path = 0;

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
  // some optimisation should be great here: sort on size or date
  //if (!rgSort(tree->records, cmpRecordDate)) goto error;

  // - compute scores (do not delete archive with bad score)
  if (!computeExtractScore(coll)) goto error;

  // loop on all archives
  for (node = coll->cacheTree->archives->head;
       need > free && node; node = node->next) {
    archive = node->item;
    
    // looking for archive that resides into the cache
    if (archive->state < AVAILABLE) continue;
    record = archive->localSupply;
    logMain(LOG_DEBUG, "check file: %s", record->extra);

    // looking for perenial archive we can safely delete from the cache
    if (!computeArchiveStatus(coll, archive)) goto error;
    if (archive->state >= TOKEEP) continue;

    // un-index the record from cache (do not free the record)
    if (!delCacheEntry(coll, record)) goto error;

    // free file on disk
    if (!(path = getAbsoluteRecordPath(coll, record))) goto error;
    logMain(LOG_NOTICE, "unlink %s", path);
    if (!env.dryRun) {
      if (unlink(path) == -1) {
	logMain(LOG_ERR, "error with unlink %s:", strerror(errno));
	goto error;
      }
    }

    path = destroyString(path);
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
