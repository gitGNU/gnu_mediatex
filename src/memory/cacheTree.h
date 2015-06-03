/*=======================================================================
 * Version: $Id: cacheTree.h,v 1.3 2015/06/03 14:03:38 nroche Exp $
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

#ifndef MDTX_MEMORY_CACHE_TREE_H
#define MDTX_MEMORY_CACHE_TREE_H 1

#include "../mediatex.h"
#include "confTree.h"
#include <pthread.h>

/* API */

typedef enum {
  MUTEX_ALLOC=0, 
  MUTEX_KEEP=1, 
  MUTEX_COMPUTE=2,
  MUTEX_MAX=3
} CacheMutex;

// cache content
struct CacheTree
{
  // cache occupancy (read only)
  off_t totalSize;
  off_t useSize;
  off_t frozenSize;

  // cache parameters (from configuration and servers.txt)
  off_t  cacheSize; // maximum size for cache
  time_t cacheTTL;  // time to live for target files in cache
  time_t queryTTL;  // time to live for queries in memory

  // lock to manage threads access
  pthread_rwlockattr_t* attr;
  pthread_rwlock_t* rwlock;
  pthread_mutex_t mutex[MUTEX_MAX];

  // serializer to load-from/save-to disk
  RecordTree* recordTree;

  // Archive* that define at less 1 Record
  // use by cache.c::freeCache
  RG* archives; // Archive*
};

CacheTree* createCacheTree(void);
CacheTree* destroyCacheTree(CacheTree* self);

int lockCacheRead(Collection* coll);
int lockCacheWrite(Collection* coll);
int unLockCache(Collection* coll);

int addCacheEntry(Collection* coll, Record* record);
int delCacheEntry(Collection* coll, Record* record);
int cleanCacheTree(Collection* coll);

int keepArchive(Collection* coll, Archive* archive, RecordType type);
int unKeepArchive(Collection* coll, Archive* archive);

int getCacheSizes(Collection* coll, 
		 off_t* totalSize, off_t* useSize, off_t* frozenSize);

int diseaseCacheTree(Collection* coll);

#endif /* MDTX_MEMORY_CACHE_TREE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
