/*=======================================================================
 * Version: $Id: cacheTree.c,v 1.10 2015/08/16 20:35:10 nroche Exp $
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


/*=======================================================================
 * Function   : lockCacheRead
 * Description: lock cache for read (use and insert records)
 * Synopsis   : int lockCacheRead(CacheTree* self)
 * Input      : CacheTree* self
 * Output     : TRUE on success
 *
 * Note       : Server's threads use the cache for reading
 *              simultaneously (using 3 mutexes)
 =======================================================================*/
int lockCacheRead(Collection* coll)
{
  int rc = FALSE;
  int err = 0;
 
  checkCollection(coll);
  logMemory(LOG_DEBUG, "lock cache for read");
 
  // lock R
  if ((err = pthread_rwlock_rdlock(coll->cacheTree->rwlock))) {
    logMemory(LOG_ERR, "pthread_rdlock_rdlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "lockCacheRead fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : lockCacheWrite
 * Description: lock cache for write (remove records marked 'removed')
 * Synopsis   : int lockCacheWrite(CacheTree* self)
 * Input      : CacheTree* self
 * Output     : TRUE on success
 =======================================================================*/
int lockCacheWrite(Collection* coll)
{
  int rc = FALSE;
  int err = 0;
 
  checkCollection(coll);
  logMemory(LOG_DEBUG, "lock cache for write");
 
  // lock W
  if ((err = pthread_rwlock_wrlock(coll->cacheTree->rwlock))) {
    logMemory(LOG_ERR, "pthread_rwlock_wrlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "lockCacheWrite fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : unLockCache
 * Description: unlock the cache
 * Synopsis   : int unLockCache(CacheTree* self)
 * Input      : CacheTree* self
 * Output     : TRUE on success
 * Note       : to call after either read or write lock call
 =======================================================================*/
int unLockCache(Collection* coll)
{
  int rc = FALSE;
  int err = 0;
 
  checkCollection(coll);
  logMemory(LOG_DEBUG, "unlock Cache");
 
  // lock W
  if ((err = pthread_rwlock_unlock(coll->cacheTree->rwlock))) {
    logMemory(LOG_ERR, "pthread_rwlock_unlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "unLockCache fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : createCacheTree
 * Description: Create, by memory allocation a md5 merger
 * Synopsis   : CacheTree* createCacheTree(void)
 * Input      : N/A
 * Output     : The address of the create empty merger.
 =======================================================================*/
CacheTree*
createCacheTree(void)
{
  CacheTree* rc = 0;
  int i = 0;
  int err = 0;

  if ((rc = (CacheTree*) malloc (sizeof (CacheTree))) == (CacheTree*) 0)
    goto error;
   
  memset(rc, 0, sizeof(CacheTree));
  
  if ((rc->archives = createRing()) == 0) goto error;
  if ((rc->recordTree = createRecordTree()) == 0)
    goto error;

  // init the locks

  if ((rc->attr = malloc(sizeof(pthread_rwlockattr_t))) == 0) {
    logMemory(LOG_ERR, "cannot allocate pthread_rwlockattr_t");
    goto error;
  }

  if ((rc->rwlock = malloc(sizeof(pthread_rwlock_t))) == 0) {
    logMemory(LOG_ERR, "cannot allocate pthread_rwlock_t");
    goto error;
  }
  
  if (pthread_rwlockattr_setpshared(rc->attr, PTHREAD_PROCESS_SHARED)) {
    logMemory(LOG_ERR, "pthread_rwlockattr_setpshared fails: %s", 
	    strerror(errno));
    goto error;
  }
  
  if (pthread_rwlock_init(rc->rwlock, rc->attr)) {
    logMemory(LOG_ERR, "pthread_rwlock_init fails: %s", strerror(errno));
    goto error;
  }

  for (i=0; i<MUTEX_MAX; ++i) {
    if ((err = pthread_mutex_init(&rc->mutex[i], (pthread_mutexattr_t*)0))
	!= 0) {
      logMemory(LOG_INFO, "pthread_mutex_init: %s", strerror(err));
      goto error;
    }
  }

  return rc;
 error:
  logMemory(LOG_ERR, "malloc: cannot create Record");
  rc = destroyCacheTree(rc);
  return rc;
}

/*=======================================================================
 * Function   : destroyRecord
 * Description: Destroy a merger by freeing all the allocate memory.
 * Synopsis   : CacheTree* destroyCacheTree(CacheTree* self)
 * Input      : CacheTree* self = the address of the merger to destroy.
 * Output     : Nil address of a merger.
 =======================================================================*/
CacheTree* 
destroyCacheTree(CacheTree* self)
{
  int i = 0;
  int err = 0;

  if(self == 0) goto error;

  // do not free archives
  self->archives = destroyOnlyRing(self->archives); 
  self->recordTree = destroyRecordTree(self->recordTree);
  pthread_rwlock_destroy(self->rwlock);
  pthread_rwlockattr_destroy(self->attr);
  
  // free the locks
  free(self->rwlock);
  free(self->attr);
  
  for (i=0; i<MUTEX_MAX; ++i) {
    if ((err = pthread_mutex_destroy(&self->mutex[i]))) {
      logMemory(LOG_INFO, "pthread_mutex_init: %s", strerror(err));
      goto error;
    }
  }
  
  free(self);
 error:
  return (CacheTree*)0;
}

/*=======================================================================
 * Function   : haveRecords
 * Description: int haveRecords(RG* ring)
 * Synopsis   : check if there is record into the input ring
 * Input      : RG* ring
 * Output     : TRUE if there is at less one record
 =======================================================================*/
int 
haveRecords(RG* ring) 
{
  int rc = FALSE;
  Record* record = 0;
  RGIT* curr = 0;

  while ((record = rgNext_r(ring, &curr))) {
    if (!(record->type & REMOVE)) {
      rc = TRUE;
      break;
    }
  }

  return rc;
}	

/*=======================================================================
 * Function   : computeArchiveStatus
 * Description: Compute archive status
 * Synopsis   : int computeArchiveStatus(Archive* self)
 * Input      : Archive* self = the archive
 * Output     : TRUE on success
 *
 * Note       : This function and only this one use the
 *              MUTEX_COMPUTE mutex so as to be threads-safe
 =======================================================================*/
int 
computeArchiveStatus(Collection* coll, Archive* archive)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  AState previousState = UNUSED;
  time_t date = -1;
  int err = 0;

  checkCollection(coll);
  checkArchive(archive);
  cache = coll->cacheTree;
  previousState = archive->state;
  logMemory(LOG_DEBUG, "computeArchiveStatus %s:%lli",   
	  archive->hash, archive->size);

  // compute scores (only once) so as to keep archive with bad score
  if (!computeExtractScore(coll)) goto error;

  if ((date = currentTime()) == -1) goto error; 

  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_COMPUTE]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  // default state: unused
  archive->state = UNUSED;

  // state1: used
  if (haveRecords(archive->finalSupplies) ||
      haveRecords(archive->remoteSupplies))
    archive->state = USED;

  // state 2: wanted
  if (haveRecords(archive->demands)) archive->state = WANTED;
  if (archive->localSupply == 0 ||
      (archive->localSupply->type & REMOVE)) goto end;

  // state 3: allocated (archive->localSupply)
  switch (getRecordType(archive->localSupply)) {
  case MALLOC_SUPPLY:
    archive->state = ALLOCATED;
    goto end;
    break;

  case LOCAL_SUPPLY:
    // state 4: available
    archive->state = AVAILABLE;
    break;

  default:
    logMemory(LOG_ERR, "bad state for local supply: %s", 
	    strRecordType(archive->localSupply));
    goto error2;
  }

  // state 5: to keep archive having a bad score or 
  // still in use for extraction
  if (archive->extractScore <= coll->serverTree->scoreParam.maxScore /2
      || archive->localSupply->date > date) {
    archive->state = TOKEEP;
  }
    
 end:
  logMemory(LOG_INFO, "archive status %s%lli: %s -> %s", 
	    archive->hash, (long long int)archive->size,
	    strAState(previousState), strAState(archive->state));

  // become allocated
  if (previousState < ALLOCATED && archive->state >= ALLOCATED)
    coll->cacheTree->useSize += archive->size;

  // become unavailable
  if (previousState >= ALLOCATED && archive->state < ALLOCATED)
    coll->cacheTree->useSize -= archive->size;

  // become to be keept
  if (previousState < TOKEEP && archive->state >= TOKEEP)
     coll->cacheTree->frozenSize += archive->size;

  // become not to keept anymore
  if (previousState >= TOKEEP && archive->state < TOKEEP)
    coll->cacheTree->frozenSize -= archive->size;

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_COMPUTE]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
 error:
  if (!rc) {
     logMemory(LOG_ERR, "computeArchiveStatus fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : addCacheEntry
 * Description: Add a record to the collection's cache tree
 * Synopsis   : int addCacheEntry(Collection* coll, Record* record)
 * Input      : Collection* coll: the collection we use
 *              Record* record: record to add
 * Output     : TRUE on success
 * Note       : Server's records are not managed here
                Threads must alway call it using cacheAlloc()
 =======================================================================*/
int addCacheEntry(Collection* coll, Record* record) 
{
  int rc = FALSE;
  Archive* archive = 0;
  RG* ring = 0;
  //int err = 0;

  checkCollection(coll);
  checkRecord(record);
  archive = record->archive;
  checkArchive(archive);
  logMemory(LOG_DEBUG, "addCacheEntry %s, %s %s:%lli",
	  strRecordType(record), record->server->fingerPrint, 
	  record->archive->hash, (long long int)record->archive->size);

  // add archive to cache ring if not already there
  ring = coll->cacheTree->archives;
  if (!rgHaveItem(ring, archive)) {
    archive->state = UNUSED;
    if (!rgInsert(ring, archive)) goto error;
  }

  // add record is not already there
  ring = coll->cacheTree->recordTree->records;
  if (!rgMatchItem(ring, record, cmpRecord)) {
    if (!rgInsert(ring, record)) goto error;
  }

  if (record->type & REMOVE) {
    logMemory(LOG_ERR, "cannot add a record marked 'removed'");
    goto error;
  }

  // index record into the archive
  switch (getRecordType(record)) {

  case MALLOC_SUPPLY:
  case LOCAL_SUPPLY:
    if (archive->localSupply == 0 ||
	(archive->localSupply->type & REMOVE)) {
      archive->localSupply = record;
    }
    else {
      logMemory(LOG_NOTICE, 
	      "we only record one local supply by archive");
    }
    break;
      
  case FINAL_SUPPLY:
     if (!rgInsert(archive->finalSupplies, record)) goto error;
     break;

  case REMOTE_SUPPLY:
     if (!rgInsert(archive->remoteSupplies, record)) goto error;
     break;

  case FINAL_DEMAND:
  case LOCAL_DEMAND:
  case REMOTE_DEMAND:
     if (!rgInsert(archive->demands, record)) goto error;
     break;

  default:
    logMemory(LOG_ERR, "cannot index %s record", strRecordType(record));
    goto error;
  }

  // update archive status
  if (!computeArchiveStatus(coll, archive)) goto error;

  /*
  // openClose mutex
  if ((err = pthread_mutex_lock(&coll->mutex[iCACH]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  */

  coll->fileState[iCACH] = MODIFIED;

  /*
  if ((err = pthread_mutex_unlock(&coll->mutex[iCACH]))) {
    logMemory(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    goto error;
  }
  */

  rc = TRUE;
 error:
  if (!rc) {
     logMemory(LOG_DEBUG, "fails to add a cache entry");
     delCacheEntry(coll, record);
  }
  return rc;
}

/*=======================================================================
 * Function   : delCacheEntry
 * Description: Del a record to the collection's cache tree
 * Synopsis   : int delCacheEntry(Collection* coll, Record* record)
 * Input      : Collection* coll: the collection we use
 *              Record* record: record to del
 * Output     : TRUE on success
 * Note       : Server's records are not managed here
 =======================================================================*/
int delCacheEntry(Collection* coll, Record* record)
{
  int rc = FALSE;
  Archive* archive = 0;
  //int err = 0;

  checkCollection(coll);
  checkRecord(record);
  archive = record->archive;
  checkArchive(archive);
  logMemory(LOG_DEBUG, "delCacheEntry %s, %s %s:%lli",
	  strRecordType(record), record->server->fingerPrint, 
	  record->archive->hash, (long long int)record->archive->size);

  record->type |= REMOVE;

  // update archive state
  if (!computeArchiveStatus(coll, record->archive)) goto error;

  /*
  if ((err = pthread_mutex_lock(&coll->mutex[iCACH]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  */

  coll->fileState[iCACH] = MODIFIED;

  /*
  if ((err = pthread_mutex_unlock(&coll->mutex[iCACH]))) {
    logMemory(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    goto error;
  }
  */

  rc = TRUE;
 error:
  if (!rc) {
     logMemory(LOG_ERR, "fails to del a cache entry");
  }
  return rc;
}

/*=======================================================================
 * Function   : keepArchive
 * Description: keep archive on cache
 * Synopsis   : int keepArchive(Collection* coll, Archive* archive, 
 *                                  RecordType type)
 * Input      : Collection* coll = related collection
 *              Archive* archive = archive to keep
 *              RecordType type = use to compute time to keep
 * Output     : TRUE on success
 *
 * Note       : keepArchive and unKeepArchive are using MUTEX_KEEP
 *              manage the concurents calls.
 *              Keeping archive is only related to extraction delay, 
 *              but it is not related with scores 
 =======================================================================*/
int
keepArchive(Collection* coll, Archive* archive)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  Record* record = 0;
  time_t date = -1;
  int err = 0;

  checkCollection(coll);
  checkArchive(archive);
  record = archive->localSupply;
  checkRecord(record);
  cache = coll->cacheTree;
  logMemory(LOG_DEBUG, "keep %s:%lli", archive->hash, archive->size); 

  if (record->type & REMOVE) {
      logMemory(LOG_ERR, 
	      "cannot keep as local supply is already removed");
      goto error;
  }
  
  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_ALLOC]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  
  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_KEEP]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error2;
  }

  if (archive->nbKeep == 0) archive->backupDate = record->date;
  ++archive->nbKeep;
  
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_KEEP]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error2;
  }
  
  // temporary record during extraction
  if ((date = currentTime()) == -1) goto error; 
  date += 5*MINUTE;

  if (record->date < date) {
    record->date = date;
    if (!computeArchiveStatus(coll, archive)) goto error2;
  }

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_ALLOC]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
 error:
    if (!rc) {
    logMemory(LOG_ERR, "keepArchive fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : unKeepArchive
 * Description: do not keep archive on cache anymore
 * Synopsis   : int unKeepArchive(Collection* coll, Archive* archive, 
 *                                                time_t prevDate)
 * Input      : Collection* coll = related collection
 *              Archive* archive = archive to un-keep
 *              time_t prevDate = date to set
 * Output     : TRUE on success
 *
 * Note       : only the temporary records used during extraction 
 *              should be unkeep
 =======================================================================*/
int
unKeepArchive(Collection* coll, Archive* archive)
{
  int rc = FALSE;
  CacheTree* cache = 0;
  Record* record = 0;
  int err = 0;

  checkCollection(coll);
  checkArchive(archive);
  record = archive->localSupply;
  checkRecord(record);
  cache = coll->cacheTree;
  logMemory(LOG_DEBUG, "un keep %s:%lli",   
	  archive->hash, archive->size); 
  
  if (record->type & REMOVE) {
    logMemory(LOG_ERR, 
	    "cannot unkeep as local supply is already removedkeep");
    goto error;
  }
 
  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_KEEP]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  --archive->nbKeep;
  if (archive->nbKeep == 0) record->date = archive->backupDate;
  archive->backupDate = 0;

  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_KEEP]))) {
    logMemory(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  if (!computeArchiveStatus(coll, archive)) goto error;

  rc = TRUE;
 error:
    if (!rc) {
    logMemory(LOG_ERR, "unKeepArchive fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : cleanCacheTree
 * Description: Remove record marked as removed from the cache tree
 * Synopsis   : int cleanCacheTree(Collection* coll)
 * Input      : Collection* coll: the collection to use
 * Output     : TRUE on success
 =======================================================================*/
int cleanCacheTree(Collection* coll)
{
  int rc = FALSE;
  RG* records = 0;
  RG* archives = 0;
  Record* record = 0;
  Record* next = 0;
  Archive* archive = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  logMemory(LOG_DEBUG, "cleanCacheTree %s", coll->label);
  if (!lockCacheWrite(coll)) goto error;

  // for all records marked as "removed"
  records = coll->cacheTree->recordTree->records;
  next = rgNext_r(records, &curr);
  while (next) {
    record = next;
    archive = record->archive;
    next = rgNext_r(records, &curr);
    if (!(record->type & REMOVE)) continue; 

    switch (getRecordType(record)) {

    case MALLOC_SUPPLY:
    case LOCAL_SUPPLY:
      if (archive->localSupply == record) {
	archive->localSupply = 0; 
      }
      break;
      
    case FINAL_SUPPLY:
      rgDelItem(archive->finalSupplies, record);
      break;
      
    case REMOTE_SUPPLY:
      rgDelItem(archive->remoteSupplies, record);
      break;
      
    case FINAL_DEMAND:
    case LOCAL_DEMAND:
    case REMOTE_DEMAND:
      rgDelItem(archive->demands, record);
      break;
      
    default:
      logMemory(LOG_ERR, "cannot unindex %s record", strRecordType(record));
      goto error2;
    }

    // del record if there
    if (!rgDelItem(records, record)) {
      logMemory(LOG_WARNING, "record not found into cache");
      goto error2;
    }

    // remove archive from cache if it has no record
    if (archive->state == UNUSED) {
      archives = coll->cacheTree->archives;
      rgDelItem(archives, archive);
    }

    // try to disease archive
    if (!diseaseArchive(coll, record->archive)) goto error2;  

    // remove the record as owned by cache->recordTree->records
    if (!delRecord(coll, record)) goto error;;
  }

  rc = TRUE;
 error2:
  if (!unLockCache(coll)) rc = FALSE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "cleanCacheTree fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : diseaseCacheTree
 * Description: Disease the cache tree
 * Synopsis   : int diseaseCacheTree(Collection* coll)
 * Input      : Collection* coll: the collection to use
 * Output     : TRUE on success
 * Note       : Do not disease the recordTree !?
 =======================================================================*/
int diseaseCacheTree(Collection* coll)
{
  int rc = FALSE;
  RG* ring = 0;
  Record* record = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  logMemory(LOG_DEBUG, "disease %s cache tree", coll->label);

  // for all records
  if (!expandCollection(coll)) goto error;
  ring = coll->cacheTree->recordTree->records;
  while((record = rgNext_r(ring, &curr))) {
    if (!(record->type & REMOVE) &&
	!delCacheEntry(coll, record)) goto error;
  }

  if (!cleanCacheTree(coll)) goto error;

  rc = TRUE;
 error:
  if (!rc) logMemory(LOG_ERR, "diseaseCacheTree fails");
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
