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

static int 
isSafeArchive(Collection* coll, Archive* self, 
	      int(*callback)(Collection*, Archive*), int* isSafe);

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
 * Function   : removeFile
 * Description: unlink file from cache
 * Synopsis   : int removeFile(char* absolutePath, char* relativePath, 
 *                             char* message)
 * Input      : char* absolutePath: path to unlink
 *              char* relativePath: part of the log message
 *              char* message: part of the log message
 * Output     : TRUE on success
 =======================================================================*/
static int 
removeFile(char* absolutePath, char* relativePath, char* message)
{
  // do not unlink final supplies
  if (*relativePath == '/') return TRUE;

  logMain(LOG_NOTICE, "remove %s from cache (%s)",
	  relativePath, message);

  if (!env.dryRun) {
    if (unlink(absolutePath) == -1) {
      logMain(LOG_ERR, "error with unlink %s:", strerror(errno));
      return FALSE;
    }
  }
  return TRUE;
}

/*=======================================================================
 * Function   : addFile
 * Description: index a new local-supply into the cache
 * Synopsis   : int addFile(Collection* coll, char* relativePath, 
 *                          Archive* archive)
 * Input      : Collection* coll
 *              char* relativePath: path of the local-supply
 *              Archive* archive: related archive
 * Output     : TRUE on success
 =======================================================================*/
static int 
addFile(Collection* coll, char* relativePath, Archive* archive)
{
  int rc = FALSE;
  Record *record = 0;
  char* extra = 0;

  // assert we have the localhost server object
  if (!getLocalHost(coll)) goto error;
    
  // add a new cache entry
  if (!(extra = createString(relativePath))) goto error;

  // notice dedicated cache directory where to extract final supplies
  if (*relativePath == '/') {
    if (!(extra = catString(extra, CONF_SUPPD))) goto error;
  }

  if (!(record = 
	addRecord(coll, coll->localhost, archive, SUPPLY, extra)))
    goto error;

  // add file as local supply and compute its archive status
  if (!addCacheEntry(coll, record)) goto error;
  record = 0;

  rc = TRUE;
 error:
  if (record) delRecord(coll, record);
  return rc;
}

/*=======================================================================
 * Function   : scanFile
 * Description: Add, keep or remove a file into a collection cache
 * Synopsis   : int scanFile(char* coll, char* absolutePath,
 *                          char* relativePath, AVLTree* localSuppliesOk) 
 * Input      : char* coll
 *              char* absolutePath
 *              char* relativePath
 *              AVLTree* localSuppliesOk: known files
 * Output     : TRUE on success
 =======================================================================*/
static int 
scanFile(Collection* coll, char* absolutePath, char* relativePath, 
	 AVLTree* localSuppliesOk) 
{
  int rc = FALSE;
  Archive* archive = 0;
  AVLNode* node = 0;
  struct stat statBuffer;
  CheckData md5;
  Record searchRecord;

  logMain(LOG_DEBUG, "scanFile");
  checkLabel(absolutePath);
  checkLabel(relativePath);
  logMain(LOG_INFO, "scan: %s", relativePath);
  memset(&md5, 0, sizeof(CheckData));

  // get file attributes (size)
  if (stat(absolutePath, &statBuffer)) {
    logMain(LOG_ERR, "stat fails on %s: %s", 
	    absolutePath, strerror(errno));
    goto error;
  }

  // remove empty file from cache
  if (!statBuffer.st_size && *relativePath != '/') {
    if (!removeFile(absolutePath, relativePath, "empty file")) 
      goto error;
    goto end;
  }

  // match already loaded record from md5sums file
  searchRecord.extra = relativePath;
  if ((node = avl_search(localSuppliesOk, &searchRecord)))
    goto end;

  // unknown file: compute hash
  md5.path = absolutePath;
  md5.size = statBuffer.st_size;
  md5.opp = CHECK_CACHE_ID;
  if (!doChecksum(&md5)) goto error;

  // archive should adready exists on extraction metadata
  if (!(archive = getArchive(coll, md5.fullMd5sum, statBuffer.st_size))) {
    if (!removeFile(absolutePath, relativePath, "no extract rule")) 
      goto error;
    goto end;
  } 

  // remove doublons
  if (archive->localSupply && 
      !(archive->localSupply->type & REMOVE)) {
    if (!removeFile(absolutePath, relativePath, "doublon")) goto error;
    goto end;
  }

  // get new record
  if (!addFile(coll, relativePath, archive)) goto error;

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
 * Synopsis   : static int 
 *              scanRepository(Collection* collection, const char* path,
 *                             AVLTree* localSuppliesOk)
 * Input      : Collection* collection = the related collection
 *              const char* path = the directory path
 *              AVLTree* localSuppliesOk: known files
 * Output     : TRUE on success
 * Note       : scandir assert the order (readdir_r do not),
 *              this make non-regression tests easier.
 =======================================================================*/
static int 
scanRepository(Collection* coll, const char* path, 
	       AVLTree* localSuppliesOk)
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
	  !scanRepository(coll, relativePath, localSuppliesOk)) 
	goto error;
      break;
	
    case DT_REG:
      if (!strcmp(entry->d_name, ".htaccess")) {
	  logMain(LOG_DEBUG, "ignore .htaccess files");
	  break;
	}
	  
      if ((absolutePath2 = createString(absolutePath)) == 0 ||
	  (absolutePath2 = catString(absolutePath2, entry->d_name)) 
	  == 0) goto error;
      if (!scanFile(coll, absolutePath2, relativePath, localSuppliesOk))
	goto error;
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
 * Function   : scanMd5sumFiles
 * Description: scan files provided by md5sums.txt
 * Synopsis   : int scanMd5sumFiles(Collection* coll,
 *                                 AVLTree* localSuppliesOk, int doQuick)
 * Input      : Collection* coll : collections to scan
 *              AVLTree* localSuppliesOk: known files
 *              int doQuick: trust on recorded checksums
 * Output     : TRUE on success
 * Note       : - on quick only check size, else check md5sum too  
 *              - add path into a btree
 =======================================================================*/
static int 
scanMd5sumFiles(Collection* coll, AVLTree* localSuppliesOk, int doQuick)
{
  int rc = FALSE;
  Archive* archive = 0;
  Record* supply = 0;
  AVLNode* node = 0;
  char* path = 0;
  CheckData md5;
  struct stat statBuffer;

  logMain(LOG_DEBUG, "%sscanMd5sumFiles", doQuick?"quick ":"");

  // check cache entries
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    if (archive->state < AVAILABLE ||
	!(supply = archive->localSupply) ||
	supply->type & REMOVE) continue;

    logMain(LOG_INFO, "check: %s", supply->extra);
    
    // test if the file is really there 
    path = destroyString(path);
    if (!(path = getAbsoluteRecordPath(coll, supply))) goto error;

    // get file size
    if (stat(path, &statBuffer)) {
      logMain(LOG_WARNING, "stat fails on %s: %s", path, strerror(errno));
      goto unIndex;
    }

    if (statBuffer.st_size != supply->archive->size) {
      logMain(LOG_WARNING, "bad size %s: %lli vs %lli expected",
	    supply->extra, (long long int)statBuffer.st_size,
	      (long long int)supply->archive->size);
      goto unIndex;
    }

    if (!doQuick) {
      // compute checksum
      memset(&md5, 0, sizeof(CheckData));
      md5.path = path;
      md5.opp = CHECK_CACHE_ID;
      if (!doChecksum(&md5)) goto error;
      if (strncmp(md5.fullMd5sum, supply->archive->hash, MAX_SIZE_MD5)) {
	logMain(LOG_WARNING, "bad md5sum %s: %s vs %s expected",
		supply->extra, md5.fullMd5sum, supply->archive->hash);
	goto unIndex;
      }
    }

    // add file to localSuppliesOk tree for scanRepository
    if (!avl_insert(localSuppliesOk, supply)) goto error;
    continue;

  unIndex:
    // del the record from the cacheTree
    if (!delCacheEntry(coll, supply)) goto error;
  } 
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%sscanMd5sumFiles fails", doQuick?"quick ":"");
  } 
  destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : scanSupportFiles
 * Description: scan files provided by support.txt
 * Synopsis   : static int scanSupportFiles(Collection* coll)
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

  if (!loadConfiguration(CFG|SUPP)) goto error;

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
    if (!(archive = getArchive(coll, supp->fullMd5sum, supp->size)))
      goto error;
    if (!addFile(coll, supp->name, archive)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "scanSupportFiles fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : scanCollection
 * Description: scan caches
 * Synopsis   : int scanCollection(Collection* coll, int doQuick)
 * Input      : Collection* coll: collections to scan
 *              int doQuick: do not compute checksums
 * Output     : TRUE on success
 =======================================================================*/
int 
scanCollection(Collection* coll, int doQuick)
{
  int rc = FALSE;
  AVLTree* localSuppliesOk;

  checkCollection(coll);
  logMain(LOG_DEBUG, "%sscanCollection %s",
	  doQuick?"quick ":"", coll->label);

  if (!(localSuppliesOk =
	avl_alloc_tree(cmpRecordPathAvl, (avl_freeitem_t)0)))
    goto error;


  if (!loadCollection(coll, EXTR|CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;

  // check archive are really here as expected
  if (!scanMd5sumFiles(coll, localSuppliesOk, doQuick)) goto error3;

  // add archive if new ones founded
  if (!scanRepository(coll, "", localSuppliesOk)) goto error3;

  rc = TRUE;
 error3:
  if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, EXTR|CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%sscanCollection fails", doQuick?"quick ":"");
  } 
  // do not free archives (freeitem callback = NULL)
  avl_free_tree(localSuppliesOk);
  return rc;
}

/*=======================================================================
 * Function   : loadCache
 * Description: load cache index and update local supplies into
 * Synopsis   : int loadCache(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int 
loadCache(Collection* coll)
{
  int rc = FALSE;

  checkCollection(coll);
  logMain(LOG_DEBUG, "load %s cache", coll->label);

  // parse md5sum file (what this function really do)
  if (!loadCollection(coll, EXTR|CACH)) goto error;

  // force quick scan if md5sum file was empty
  if (!avl_count(coll->cacheTree->recordTree->records)) {
    if(!scanCollection(coll, FALSE)) goto error2;
  }

  // add file-supports as local supplies
  if (!scanSupportFiles(coll)) goto error2;

  rc = TRUE;
 error2:
  if (!releaseCollection(coll, EXTR|CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "loadCache fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : cacheSizes
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
  *free = self->totalSize - self->useSize;
  *available = self->totalSize - self->frozenSize;

  logMain(LOG_INFO, "cache sizes:");
  logMain(LOG_INFO, "  total  %12llu", self->totalSize);
  logMain(LOG_INFO, "- used   %12llu", self->useSize);
  logMain(LOG_INFO, "= free   %12llu", *free);
  logMain(LOG_INFO, "  total  %12llu", self->totalSize);
  logMain(LOG_INFO, "- frozen %12llu", self->frozenSize);
  logMain(LOG_INFO, "= avail  %12llu", *available);
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
  // - suppress older files first
  // - if same date, suppress smallest first
#warning to test as do not seems like that (->utcacheTree)

  // - compute scores (do not delete archive with bad score)
  //if (!computeExtractScore(coll)) goto error;

  // loop on all archives
  for (node = coll->cacheTree->archives->head;
       need > free && node; node = node->next) {
    archive = node->item;
    
    // looking for archive that resides into the cache
    if (archive->state < AVAILABLE) continue;
    record = archive->localSupply;

    // looking for perenial archive we can safely delete from the cache
    if (archive->state >= TOKEEP) continue;

    // un-index the record from cache (do not free the record)
    if (!delCacheEntry(coll, record)) goto error;

    // free file on disk
    if (!(path = getAbsoluteRecordPath(coll, record))) goto error;
    if (!removeFile(path, record->extra, "free")) goto error;
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
    logMain(LOG_ERR, "fails to add new record into %s cache",
	    coll->label);
    *record = destroyRecord(*record);
  } 
  extra = destroyString(extra);
  return rc;
}

/*=======================================================================
 * Function   : saveCache
 * Description: save cache
 * Synopsis   : int saveCache(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int saveCache(Collection* coll)
{
  int rc = FALSE;
  Configuration* conf = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  logMain(LOG_DEBUG, "saveCache on collection %s", coll->label);
  if (!(conf = getConfiguration())) goto error;

  // force writing
  while ((coll = rgNext_r(conf->collections, &curr))) {
    coll->fileState[iCACH] = MODIFIED;
  }

  if (!serverSaveAll()) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "saveCache fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : statusCache
 * Description: display the cache status into the logs
 * Synopsis   : int statusCache(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int statusCache(Collection* coll)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  off_t free = 0;
  off_t available = 0;
  char max[30];
  char froz[30];
  char avail[30];
  
  checkCollection(coll);
  cache = coll->cacheTree;
  logMain(LOG_DEBUG, "statusCache on collection %s", coll->label);

  // cache sizes
  cacheSizes(cache, &free, &available);
  sprintSize(max, (long long int)cache->totalSize);
  sprintSize(froz, (long long int)cache->frozenSize);
  sprintSize(avail, (long long int)available);
  
  logMain(LOG_NOTICE, "%s cache's sizes: max, frozen, available",
	  coll->label);
  logMain(LOG_NOTICE, "%11s%11s%11s", max, froz, avail);
  logMain(LOG_NOTICE, "---");
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "statusCache fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : quickScanCache
 * Description: scan cache quickly
 * Synopsis   : int quickScanCache(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int quickScanCache(Collection* coll)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  int err = 0;

  checkCollection(coll);
  cache = coll->cacheTree;
  logMain(LOG_DEBUG, "quickScanCache on collection %s", coll->label);

  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  if (!scanCollection(coll, TRUE)) goto error2;
  if (!statusCache(coll)) goto error;

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logMain(LOG_ERR, "quickScanCache fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : scanCache
 * Description: scan cache
 * Synopsis   : int scanCache(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int scanCache(Collection* coll)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  int err = 0;

  checkCollection(coll);
  cache = coll->cacheTree;
  logMain(LOG_DEBUG, "scanCache on collection %s", coll->label);

  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  if (!scanCollection(coll, FALSE)) goto error2;
  if (!statusCache(coll)) goto error;

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logMain(LOG_ERR, "scanCache fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : isSafeTrim
 * Description: looking for content available from the cache
 * Synopsis   : int isSafeTrim(Collection* coll, Archive* archive)
 * Input      : Collection* coll
 *              Archive* archive
 * Output     : TRUE if safe
 =======================================================================*/
static int
isSafeTrim(Collection* coll, Archive* archive)
{
  return (archive->state >= AVAILABLE);
}

/*=======================================================================
 * Function   : isSafeClean
 * Description: looking for available content or a local image
 * Synopsis   : int isSafeClean(Collection* coll, Archive* archive)
 * Input      : Collection* coll
 *              Archive* archive
 * Output     : TRUE if safe
 =======================================================================*/
static int 
isSafeClean(Collection* coll, Archive* archive)
{
  return (archive->state >= AVAILABLE ||
	  getImage(coll, coll->localhost, archive));
}

/*=======================================================================
 * Function   : isSafeContainer
 * Description: generic recursive check from container
 * Synopsis   : ...
 * Input      : Collection* coll: need by callback function
 *              Container* self
 *              int(*callback)(Collection*, Archive*): final check
 *              int* isSafe: TRUE if safe
 * Output     : TRUE on success
 =======================================================================*/
static int 
isSafeContainer(Collection* coll, Container* self, 
		int(*callback)(Collection*, Archive*), int* isSafe)
{
  int rc = FALSE;
  Archive* archive = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "isSafeContainer %s/%s:%lli",
	  strEType(self->type), self->parent->hash,
	  (long long int)self->parent->size);

  if (isEmptyRing(self->parents)) goto error;

  // all container's parts must be safe
  *isSafe = TRUE;
  while (*isSafe && (archive = rgNext_r(self->parents, &curr))) {
    if (!isSafeArchive(coll, archive, callback, isSafe)) 
      goto error;
  }

  logMain(LOG_INFO, "%ssafe container %s/%s:%lli", *isSafe?"":"not ", 
	  strEType(self->type), self->parent->hash,
	  (long long int)self->parent->size);
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "isSafeContainer fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : isSafeArchive
 * Description: generic recursive check from archive
 * Synopsis   : ...
 * Input      : Collection* coll: need by callback function
 *              Archive* self
 *              int(*callback)(Collection*, Archive*): final check
 *              int* isSafe: TRUE if safe
 * Output     : TRUE on success
 =======================================================================*/
static int 
isSafeArchive(Collection* coll, Archive* self, 
		   int(*callback)(Collection*, Archive*), int* isSafe)
{
  int rc = FALSE;
  FromAsso* asso = 0;
  RGIT* curr = 0;
  
  logMain(LOG_DEBUG, "isSafeArchive %s:%lli", self->hash, self->size);

  if (isEmptyRing(self->fromContainers)) {
    *isSafe = callback(coll, self);
    goto end;
  }

  // look for at less one safe container
  *isSafe = FALSE;
  while (!*isSafe &&
	 (asso = rgNext_r(self->fromContainers, &curr))) {
    if (!isSafeContainer(coll, asso->container, callback, isSafe))
      goto error;
  }
  
 end:
  logMain(LOG_INFO, "%ssafe archive %s:%lli", 
	  *isSafe?"":"not ", self->hash, self->size);
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "isSafeArchive fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : trimCache
 * Description: trim cache: remove files safe and extractable from a
 *              containers available into the cache
 * Synopsis   : int trimCache(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int 
trimCache(Collection* coll)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  Archive* archive = 0;
  Record* record = 0;
  FromAsso* asso = 0;
  AVLNode* node = 0;
  RGIT* curr = 0;
  char* path = 0;
  int err = 0;
  int isSafe = FALSE;

  checkCollection(coll);
  cache = coll->cacheTree;
  logMain(LOG_DEBUG, "trimCache on collection %s", coll->label);

  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error2;
  }

  if (!scanCollection(coll, TRUE)) goto error2;

  // loop on all archives
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    
    // looking for archive that resides into the cache
    if (archive->state < AVAILABLE) continue;
    record = archive->localSupply;

    // looking for perenial archive we can safely delete from the cache
    if (archive->state >= TOKEEP) continue;

    // looking for contents having its container into the cache
    logMain(LOG_INFO, "try to trim: %s", record->extra);
    isSafe = FALSE;
    while (!isSafe &&
	   (asso = rgNext_r(archive->fromContainers, &curr))) {
      if (!isSafeContainer(coll, asso->container, isSafeTrim, &isSafe)) 
	goto error;
    }
    if (!isSafe) continue;

    // un-index the record from cache (do not free the record)
    if (!delCacheEntry(coll, record)) goto error2;

    // remove file from disk
    if (!(path = getAbsoluteRecordPath(coll, record))) goto error2;
    if (!removeFile(path, record->extra, "trim")) goto error2;
    path = destroyString(path);
  }

  if (!statusCache(coll)) goto error2;
  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logMain(LOG_ERR, "trimCache fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : cleanCache
 * Description: clean cache: remove files safe and extractable from 
 *              supports locally available
 * Synopsis   : int cleanCache(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int cleanCache(Collection* coll)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  Archive* archive = 0;
  Record* record = 0;
  FromAsso* asso = 0;
  AVLNode* node = 0;
  RGIT* curr = 0;
  char* path = 0;
  int err = 0;
  int isSafe = FALSE;

  checkCollection(coll);
  cache = coll->cacheTree;
  logMain(LOG_DEBUG, "cleanCache on collection %s", coll->label);

  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error2;
  }

  if (!scanCollection(coll, TRUE)) goto error2;  
  if (!getLocalHost(coll)) goto error2;

  // loop on all archives
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    
    // looking for archive that resides into the cache
    if (archive->state < AVAILABLE) continue;
    record = archive->localSupply;

    // looking for perenial archive we can safely delete from the cache
    if (archive->state >= TOKEEP) continue;

    // looking for contents extractable from local final-supplies
    logMain(LOG_INFO, "try to clean: %s", record->extra);
    isSafe = (getImage(coll, coll->localhost, archive) != 0);
    while (!isSafe &&
	   (asso = rgNext_r(archive->fromContainers, &curr))) {
      if (!isSafeContainer(coll, asso->container, isSafeClean, &isSafe)) 
	goto error;
    }
    if (!isSafe) continue;

    // un-index the record from cache (do not free the record)
    if (!delCacheEntry(coll, record)) goto error2;

    // remove file from disk
    if (!(path = getAbsoluteRecordPath(coll, record))) goto error2;
    if (!removeFile(path, record->extra, "clean")) goto error2;
    path = destroyString(path);
  }

  if (!statusCache(coll)) goto error2;
  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logMain(LOG_ERR, "cleanCache fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : purgeCache
 * Description: purge cache: remove all safe
 * Synopsis   : int purgeCache(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int purgeCache(Collection* coll)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  Archive* archive = 0;
  Record* record = 0;
  AVLNode* node = 0;
  char* path = 0;
  int err = 0;

  checkCollection(coll);
  cache = coll->cacheTree;
  logMain(LOG_DEBUG, "purgeCache on collection %s", coll->label);

  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error2;
  }

  if (!scanCollection(coll, TRUE)) goto error2;

  // loop on all archives
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    
    // looking for archive that resides into the cache
    if (archive->state < AVAILABLE) continue;
    record = archive->localSupply;

    // looking for perenial archive we can safely delete from the cache
    if (archive->state >= TOKEEP) continue;

    // un-index the record from cache (do not free the record)
    if (!delCacheEntry(coll, record)) goto error2;

    // remove file from disk
    if (!(path = getAbsoluteRecordPath(coll, record))) goto error2;
    if (!removeFile(path, record->extra, "purge")) goto error2;
    path = destroyString(path);
  }

  if (!statusCache(coll)) goto error2;
  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_ALLOC]))) {
    logMain(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logMain(LOG_ERR, "purgeCache fails");
  }
  path = destroyString(path);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
