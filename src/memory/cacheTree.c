/*=======================================================================
 * Version: $Id: cacheTree.c,v 1.1 2014/10/13 19:39:07 nroche Exp $
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
#include "strdsm.h"
#include "confTree.h"


/*=======================================================================
 * Function   : lockCacheRead
 * Description: lock cache for read
 * Synopsis   : int lockCacheRead(CacheTree* self)
 * Input      : CacheTree* self
 * Output     : TRUE on success
 =======================================================================*/
int lockCacheRead(Collection* coll)
{
  int rc = FALSE;
  int err = 0;
 
  checkCollection(coll);
  logEmit(LOG_DEBUG, "%s", "lock cache for read");
 
  // lock R
  if ((err = pthread_rwlock_rdlock(coll->cacheTree->rwlock))) {
    logEmit(LOG_ERR, "pthread_rdlock_rdlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "lockCacheRead fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : lockCacheWrite
 * Description: lock cache for write
 * Synopsis   : int lockCacheWrite(CacheTree* self)
 * Input      : CacheTree* self
 * Output     : TRUE on success
 =======================================================================*/
int lockCacheWrite(Collection* coll)
{
  int rc = FALSE;
  int err = 0;
 
  checkCollection(coll);
  logEmit(LOG_DEBUG, "%s", "lock cache for write");
 
  // lock W
  if ((err = pthread_rwlock_wrlock(coll->cacheTree->rwlock))) {
    logEmit(LOG_ERR, "pthread_rwlock_wrlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "lockCacheWrite fails");
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
  logEmit(LOG_DEBUG, "%s", "unlock Cache");
 
  // lock W
  if ((err = pthread_rwlock_unlock(coll->cacheTree->rwlock))) {
    logEmit(LOG_ERR, "pthread_rwlock_unlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "unLockCache fails");
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
  CacheTree* rc = NULL;
  int i = 0;
  int err = 0;

  if ((rc = (CacheTree*) malloc (sizeof (CacheTree))) == (CacheTree*) 0)
    goto error;
   
  memset(rc, 0, sizeof(CacheTree));
  
  if ((rc->archives = createRing()) == NULL) goto error;
  if ((rc->recordTree = createRecordTree()) == NULL)
    goto error;

  // init the locks

  if ((rc->attr = malloc(sizeof(pthread_rwlockattr_t))) == NULL) {
    logEmit(LOG_ERR, "%s", "cannot allocate pthread_rwlockattr_t");
    goto error;
  }

  if ((rc->rwlock = malloc(sizeof(pthread_rwlock_t))) == NULL) {
    logEmit(LOG_ERR, "%s", "cannot allocate pthread_rwlock_t");
    goto error;
  }
  
  if (pthread_rwlockattr_setpshared(rc->attr, PTHREAD_PROCESS_SHARED)) {
    logEmit(LOG_ERR, "pthread_rwlockattr_setpshared fails: %s", 
	    strerror(errno));
    goto error;
  }
  
  if (pthread_rwlock_init(rc->rwlock, rc->attr)) {
    logEmit(LOG_ERR, "pthread_rwlock_init fails: %s", strerror(errno));
    goto error;
  }

  for (i=0; i<MUTEX_MAX; ++i) {
    if ((err = pthread_mutex_init(&rc->mutex[i], (pthread_mutexattr_t*)0))
	!= 0) {
      logEmit(LOG_INFO, "pthread_mutex_init: %s", strerror(err));
      goto error;
    }
  }

  return rc;
 error:
  logEmit(LOG_ERR, "%s", "malloc: cannot create Record");
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

  if(self == NULL) goto error;

  // do not free archives
  self->archives = destroyOnlyRing(self->archives); 
  self->recordTree = destroyRecordTree(self->recordTree);
  pthread_rwlock_destroy(self->rwlock);
  pthread_rwlockattr_destroy(self->attr);
  
  // free the locks
  free(self->rwlock);
  free(self->attr);
  
  for (i=0; i<MUTEX_MAX; ++i) {
    if ((err = pthread_mutex_destroy(&self->mutex[i]) != 0)) {
      logEmit(LOG_INFO, "pthread_mutex_init: %s", strerror(err));
      goto error;
    }
  }
  
  free(self);
 error:
  return (CacheTree*)0;
}

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : 
 =======================================================================*/
int 
haveRecords(RG* ring) 
{
  int rc = FALSE;
  Record* record = NULL;
  RGIT* curr = NULL;

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
 =======================================================================*/
static int 
computeArchiveStatus(Collection* coll, Archive* archive)
{
  int rc = FALSE;
  CacheTree* cache = NULL;
  AState previousState = UNUSED;
  time_t date = -1;
  int err = 0;

  checkCollection(coll);
  checkArchive(archive);
  cache = coll->cacheTree;
  previousState = archive->state;
  logEmit(LOG_DEBUG, "computeArchiveStatus %s:%lli",   
	  archive->hash, archive->size);

  if ((date = currentTime()) == -1) goto error; 

  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_COMPUTE]))) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  // default state: unused
  archive->state = UNUSED;

  // state1: used
  if (haveRecords(archive->finaleSupplies) ||
      haveRecords(archive->remoteSupplies))
    archive->state = USED;

  // state 2: wanted
  if (haveRecords(archive->demands)) archive->state = WANTED;
  if (archive->localSupply == NULL ||
      (archive->localSupply->type & REMOVE)) goto end;

  // state 3: allocated (archive->localSupply != NULL)
  switch (getRecordType(archive->localSupply)) {
  case MALLOC_SUPPLY:
    archive->state = ALLOCATED;
    goto end;
    break;

  case LOCALE_SUPPLY:
    // state 4: available
    archive->state = AVAILABLE;
    break;

  default:
    logEmit(LOG_ERR, "bad state for local supply: %s", 
	    strRecordType(archive->localSupply));
    goto error2;
  }

  // state 5: to keep
  if (archive->localSupply->date > date) {
    archive->state = TOKEEP;
  }
    
 end:
  logEmit(LOG_INFO, "state: %s -> %s", 
	  strAState(previousState), strAState(archive->state));

  // become allocated
  if (previousState < ALLOCATED && archive->state >= ALLOCATED)
    coll->cacheTree->useSize += archive->size;

  // become unavailable
  if (previousState >= ALLOCATED && archive->state < ALLOCATED)
    coll->cacheTree->useSize -= archive->size;

  // become to be keept
  if (previousState < TOKEEP && archive->state == TOKEEP)
     coll->cacheTree->frozenSize += archive->size;

  // become not to keept anymore
  if (previousState == TOKEEP && archive->state <= TOKEEP)
    coll->cacheTree->frozenSize -= archive->size;

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_COMPUTE]))) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
 error:
  if (!rc) {
     logEmit(LOG_ERR, "%s", "computeArchiveStatus fails");
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
  Archive* archive = NULL;
  RG* ring = NULL;
  //int err = 0;

  checkCollection(coll);
  checkRecord(record);
  archive = record->archive;
  checkArchive(archive);
  logEmit(LOG_DEBUG, "addCacheEntry %s, %s %s:%lli",
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
    logEmit(LOG_ERR, "%s", "cannot add a record marked as removed");
    goto error;
  }

  // index record into the archive
  switch (getRecordType(record)) {

  case MALLOC_SUPPLY:
  case LOCALE_SUPPLY:
    if (archive->localSupply == NULL ||
	(archive->localSupply->type & REMOVE)) {
      archive->localSupply = record;
    }
    else {
      logEmit(LOG_NOTICE, "%s", 
	      "we only record one local supply by archive");
    }
    break;
      
  case FINALE_SUPPLY:
     if (!rgInsert(archive->finaleSupplies, record)) goto error;
     break;

  case REMOTE_SUPPLY:
     if (!rgInsert(archive->remoteSupplies, record)) goto error;
     break;

  case FINALE_DEMAND:
  case LOCALE_DEMAND:
  case REMOTE_DEMAND:
     if (!rgInsert(archive->demands, record)) goto error;
     break;

  default:
    logEmit(LOG_ERR, "cannot index %s record", strRecordType(record));
    goto error;
  }

  // update archive status
  if (!computeArchiveStatus(coll, archive)) goto error;

  /*
  // openClose mutex
  if ((err = pthread_mutex_lock(&coll->mutex[iCACH])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  */

  coll->fileState[iCACH] = MODIFIED;

  /*
  if ((err = pthread_mutex_unlock(&coll->mutex[iCACH])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    goto error;
  }
  */

  rc = TRUE;
 error:
  if (!rc) {
     logEmit(LOG_DEBUG, "%s", "fails to add a cache entry");
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
  Archive* archive = NULL;
  //int err = 0;

  checkCollection(coll);
  checkRecord(record);
  archive = record->archive;
  checkArchive(archive);
  logEmit(LOG_DEBUG, "delCacheEntry %s, %s %s:%lli",
	  strRecordType(record), record->server->fingerPrint, 
	  record->archive->hash, (long long int)record->archive->size);

  record->type |= REMOVE;

  // update archive state
  if (!computeArchiveStatus(coll, record->archive)) goto error;

  /*
  if ((err = pthread_mutex_lock(&coll->mutex[iCACH])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  */

  coll->fileState[iCACH] = MODIFIED;

  /*
  if ((err = pthread_mutex_unlock(&coll->mutex[iCACH])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    goto error;
  }
  */

  rc = TRUE;
 error:
  if (!rc) {
     logEmit(LOG_ERR, "%s", "fails to del a cache entry");
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
 =======================================================================*/
int
keepArchive(Collection* coll, Archive* archive, RecordType type)
{
  int rc = FALSE;
  Configuration* conf = NULL;
  CacheTree* cache = NULL;
  Record* record = NULL;
  time_t date = -1;
  int err = 0;

  checkCollection(coll);
  checkArchive(archive);
  record = archive->localSupply;
  checkRecord(record);
  cache = coll->cacheTree;
  logEmit(LOG_DEBUG, "keep %s %s:%lli",
	  type?strRecordType2(type):"TMP", archive->hash, archive->size); 

  if (record->type & REMOVE) {
      logEmit(LOG_ERR, "%s", 
	      "cannot keep as local supply is already removedkeep");
      goto error;
  }
  
  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_ALLOC]))) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  if ((date = currentTime()) == -1) goto error2; 
  if (!(conf = getConfiguration())) goto error2;
  
  // note: only the temporary record use during extraction 
  // should be unkeep
  switch (type) {
  case FINALE_DEMAND:
    // mail + download http
    date += coll->cacheTTL;
    break;
  case LOCALE_DEMAND:
    // cgi
    date += archive->size / (conf->uploadRate >> 5);
    break;
  case REMOTE_DEMAND:
    // scp (time between 2 extraction on local server)
    date += 1*DAY;
    break;
  default:
    if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_KEEP]))) {
      logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
      goto error2;
    }

    if (archive->nbKeep == 0) archive->backupDate = record->date;
    ++archive->nbKeep;

    if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_KEEP]))) {
      logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
      goto error2;
    }

    // temporary record during extraction
    date += 5*MINUTE;
  }

  if (record->date < date) record->date = date;
  if (!computeArchiveStatus(coll, archive)) goto error2;

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_ALLOC]))) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
 error:
    if (!rc) {
    logEmit(LOG_ERR, "%s", "keepArchive fails");
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
 * Note       : only the temporary record use during extraction 
 *              should be unkeep
 =======================================================================*/
int
unKeepArchive(Collection* coll, Archive* archive)
{
  int rc = FALSE;
  CacheTree* cache = NULL;
  Record* record = NULL;
  int err = 0;

  checkCollection(coll);
  checkArchive(archive);
  record = archive->localSupply;
  checkRecord(record);
  cache = coll->cacheTree;
  logEmit(LOG_DEBUG, "un keep %s:%lli",   
	  archive->hash, archive->size); 
  
  if (record->type & REMOVE) {
    logEmit(LOG_ERR, "%s", 
	    "cannot unkeep as local supply is already removedkeep");
    goto error;
  }
 
  if ((err = pthread_mutex_lock(&cache->mutex[MUTEX_KEEP]))) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  --archive->nbKeep;
  if (archive->nbKeep == 0) record->date = archive->backupDate;
  archive->backupDate = 0;

  if ((err = pthread_mutex_unlock(&cache->mutex[MUTEX_KEEP]))) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  if (!computeArchiveStatus(coll, archive)) goto error;

  rc = TRUE;
 error:
    if (!rc) {
    logEmit(LOG_ERR, "%s", "unKeepArchive fails");
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
  RG* records = NULL;
  RG* archives = NULL;
  Record* record = NULL;
  Record* next = NULL;
  Archive* archive = NULL;
  RGIT* curr = NULL;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "cleanCacheTree %s", coll->label);
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
    case LOCALE_SUPPLY:
      if (archive->localSupply == record) {
	archive->localSupply = NULL; 
      }
      break;
      
    case FINALE_SUPPLY:
      rgDelItem(archive->finaleSupplies, record);
      break;
      
    case REMOTE_SUPPLY:
      rgDelItem(archive->remoteSupplies, record);
      break;
      
    case FINALE_DEMAND:
    case LOCALE_DEMAND:
    case REMOTE_DEMAND:
      rgDelItem(archive->demands, record);
      break;
      
    default:
      logEmit(LOG_ERR, "cannot unindex %s record", strRecordType(record));
      goto error2;
    }

    // del record if there
    if (!rgDelItem(records, record)) {
      logEmit(LOG_WARNING, "%s", "record not found into cache");
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
    logEmit(LOG_ERR, "%s", "cleanCacheTree fails");
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
  RG* ring = NULL;
  Record* record = NULL;
  RGIT* curr = NULL;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "disease %s cache tree", coll->label);

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
  if (!rc) logEmit(LOG_ERR, "%s", "diseaseCacheTree fails");
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "utFunc.h"
GLOBAL_STRUCT_DEF;


/*=======================================================================
 * Function   : testThread
 * Description: Testing the concurent access
 * Synopsis   : void* testThread(void* arg)
 * Input      : void* arg: CacheTree* mergeMd5
 * Output     : N/A
 =======================================================================*/
void* 
testThread(void* arg)
{
  //env.recordTree = copyCurrentRecordTree(mergeMd5);
  return NULL;
}

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

  memoryOptions();
  //fprintf(stderr, "\t\t---\n");
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
  Collection* coll = NULL;
  Record* record = NULL;
  //pthread_t thread;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MEMORY_SHORT_OPTIONS;
  struct option longOptions[] = {
    MEMORY_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
 
      GET_MEMORY_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!createExempleConfiguration()) goto error;
  if (!(coll = getCollection("coll1"))) goto error;

  if (!lockCacheRead(coll)) goto error;
  if (!unLockCache(coll)) goto error;
  if (!lockCacheWrite(coll)) goto error;
  if (!unLockCache(coll)) goto error;
  if (!lockCacheRead(coll)) goto error;
  if (!lockCacheRead(coll)) goto error;
  if (!unLockCache(coll)) goto error;
  if (!unLockCache(coll)) goto error;

  if (!createExempleRecordTree(coll)) goto error;

  // index the record tree
  logEmit(LOG_DEBUG, "%s", "___ test indexation ___");
  coll->cacheTree->recordTree->aes.fd = STDERR_FILENO;

  while ((record = rgHead(coll->cacheTree->recordTree->records))) {
    if (!serializeRecord(coll->cacheTree->recordTree, record)) goto error;
    aesFlush(&coll->cacheTree->recordTree->aes);
    fprintf(stderr, "\n");

    if (!addCacheEntry(coll, record)) goto error;
    if (record->archive->state >= AVAILABLE) {
      if (!keepArchive(coll, record->archive, 0)) goto error;
      if (!unKeepArchive(coll, record->archive)) goto error;
    }
    if (!delCacheEntry(coll, record)) goto error;
    if (!cleanCacheTree(coll)) goto error;
  }
  if (!diseaseCacheTree(coll)) goto error;
  /************************************************************************/

  freeConfiguration();
  rc = TRUE;
 error:
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
