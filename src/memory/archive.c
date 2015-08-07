/*=======================================================================
 * Version: $Id: archive.c,v 1.7 2015/08/07 17:50:29 nroche Exp $
 * Project: MediaTeX
 * Module : archive
 *
 * archive producer interface

 MediaTex is an Electronic Archives Management System
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
 * Function   : createArchive
 * Description: Create, by memory allocation a Archive
 *              configuration projection.
 * Synopsis   : Archive* createArchive(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Archive* 
createArchive(void)
{
  Archive* rc = 0;

  if ((rc = (Archive*)malloc(sizeof(Archive))) == 0)
    goto error;
   
  memset(rc, 0, sizeof(Archive));
  if ((rc->images = createRing()) == 0) goto error;
  if ((rc->fromContainers = createRing()) == 0) goto error;
  if ((rc->documents = createRing()) == 0) goto error;
  if ((rc->assoCaracs = createRing()) == 0) goto error;
  if ((rc->records = createRing()) == 0) goto error;
  if ((rc->demands = createRing()) == 0) goto error;
  if ((rc->remoteSupplies = createRing()) == 0) goto error;
  if ((rc->finaleSupplies = createRing()) == 0) goto error;
  rc->imageScore = -1;   // still not computed
  rc->extractScore = -1; // still not computed
  rc->state = UNUSED;

  return rc;
 error:
  logMemory(LOG_ERR, "malloc: cannot create Archive");
  return destroyArchive(rc);
}

/*=======================================================================
 * Function   : destroyArchive
 * Description: Destroy a configuration by freeing all the allocate memory.
 * Synopsis   : void destroyArchive(Archive* self)
 * Input      : Archive* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a configuration.
 =======================================================================*/
Archive* 
destroyArchive(Archive* self)
{
  Archive* rc = 0;

  if(self == 0) goto error;

#if 0 // developpement mode
  strncpy(self->hash, "................................", 
	  MAX_SIZE_HASH);
  self->size = 0;
  self->imageScore = -1;
  self->extractScore = -1;
  self->state = UNUSED;
#endif

  // delete assoCarac associations
  self->assoCaracs
    = destroyRing(self->assoCaracs,
		  (void*(*)(void*)) destroyAssoCarac);
  
  // we do not free the objects (owned by trees), just the rings
  self->images = destroyOnlyRing(self->images);
  self->fromContainers = destroyOnlyRing(self->fromContainers);
  self->documents = destroyOnlyRing(self->documents);
  self->records = destroyOnlyRing(self->records);
  self->demands = destroyOnlyRing(self->demands);
  self->remoteSupplies = destroyOnlyRing(self->remoteSupplies);
  self->finaleSupplies = destroyOnlyRing(self->finaleSupplies);
  free(self);
  
error:
  return rc;
}

/*=======================================================================
 * Function   : cmpArchive
 * Description: compare 2 archives
 * Synopsis   : int cmpArchive(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2 : the archives
 * Output     : -1, 0 or 1 respectively for lower, equal or greater
 * Note       : the first one is used by sort and second one by avl trees
 =======================================================================*/
int 
cmpArchive(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Archive* 
   */
  
  Archive* a1 = *((Archive**)p1);
  Archive* a2 = *((Archive**)p2);

  rc = strncmp(a1->hash, a2->hash, MAX_SIZE_HASH);
  if (!rc) rc = a1->size - a2->size; // growing sizes
 
  return rc;
}

int 
cmpArchive2(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on items
   * and items are suposed to be Archive* 
   */
  
  Archive* a1 = (Archive*)p1;
  Archive* a2 = (Archive*)p2;

  rc = strncmp(a1->hash, a2->hash, MAX_SIZE_HASH);
  if (!rc) rc = a1->size - a2->size; // growing sizes
 
  return rc;
}

/*=======================================================================
 * Function   : cmpArchiveSize
 * Description: compare 2 archives by size
 * Synopsis   : int cmpArchiveSize(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2 : the archives
 * Output     : -1, 0 or 1 respectively for lower, equal or greater
 =======================================================================*/
int 
cmpArchiveSize(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Archive* 
   */
  
  Archive* a1 = *((Archive**)p1);
  Archive* a2 = *((Archive**)p2);

  rc = a1->size - a2->size; // growing sizes
  if (!rc) rc = strncmp(a1->hash, a2->hash, MAX_SIZE_HASH);
 
  return rc;
}

/*=======================================================================
 * Function   : cmpArchiveScore
 * Description: compare 2 archives by size first
 * Synopsis   : int cmpArchiveSize(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2 : the archives
 * Output     : -1, 0 or 1 respectively for lower, equal or greater
 =======================================================================*/
int 
cmpArchiveScore(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Archive* 
   */
  
  Archive* a1 = *((Archive**)p1);
  Archive* a2 = *((Archive**)p2);

  rc = a1->extractScore - a2->extractScore; // growing scores
  if (!rc) rc = strncmp(a1->hash, a2->hash, MAX_SIZE_HASH);
  if (!rc) rc = a1->size - a2->size; // growing sizes
 
  return rc;
}

/*=======================================================================
 * Function   : strArchiveState
 * Description: return a string for the archive state
 * Synopsis   : char* strArchiveState(ArchiveLocalState state)
 * Input      : ArchiveLocalState state
 * Output     : the state string or 0 on error
 =======================================================================*/
char* 
strAState(AState self)
{
  static char* typeLabels[ASTATE_MAX] =
    {"UNUSED", "USED", "WANTED", "ALLOCATED", "AVAILABLE",  "TOKEEP"};

  if (self < 0 || self >= ASTATE_MAX) {
    logMemory(LOG_WARNING, "unknown archive type %i", self);
    return "???";
  }

  return typeLabels[self];
}


/*=======================================================================
 * Function   : getArchive
 * Description: Find an archive
 * Synopsis   : Archive* getArchive(Collection* coll, 
                                    char* hash, off_t size)
 * Input      : Collection* coll : where to find
 *              char* hash : id1 of the archive
 *              off_t size : id2 of the archive
 * Output     : Archive* : the Archive we have found
 =======================================================================*/
Archive* 
getArchive(Collection* coll, char* hash, off_t size)
{
  Archive* rc = 0;
  Archive archive;
  AVLNode* node = 0;

  checkCollection(coll);
  checkLabel(hash);
  logMemory(LOG_DEBUG, "getArchive %s:%lli", hash, (long long int) size);

  // look for archive
  strncpy(archive.hash, hash, MAX_SIZE_HASH);
  archive.size = size;
  if ((node = avl_search(coll->archives, &archive))) {
    rc = (Archive*)node->item;
  }

 error:
  return rc;
}

/*=======================================================================
 * Function   : addArchive
 * Description: Add an archive if not already there
 * Synopsis   : Archive* addArchive(Collection* coll,
 *                                  char* hash, off_t size)
 * Input      : Collection* coll : where to add
 *              char* hash : id1 of the archive
 *              off_t size : id2 of the archive
 * Output     : Archive* : the Archive we have found/added
 =======================================================================*/
Archive* 
addArchive(Collection* coll, char* hash, off_t size)
{
  Archive* rc = 0;
  Archive* archive = 0;

  checkCollection(coll);
  checkLabel(hash);
  logMemory(LOG_DEBUG, "addArchive %s:%lli", hash, (long long int) size);
  
  // already there
  if ((archive = getArchive(coll, hash, size))) goto end;

  // add new one if not already there
  if (!(archive = createArchive())) goto error;
  strncpy(archive->hash, hash, MAX_SIZE_HASH);
#if 1
  archive->hash[MAX_SIZE_HASH] = (char)0; // developpement code
#endif
  archive->size = size;
  archive->id = coll->maxId++;

  if (!avl_insert(coll->archives, archive)) goto error;

 end:
  rc = archive;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addArchive fails");
    archive = destroyArchive(archive);
  }
  return rc;
}

/*=======================================================================
 * Function   : delArchive
 * Description: Del an archive
 * Synopsis   : int delArchive(Collection* coll, Archive* archive)
 * Input      : Collection* coll : where to del
 *              Archive* archive : the archive to del
 * Output     : TRUE on success
 =======================================================================*/
int
delArchive(Collection* coll, Archive* self)
{
  int rc = FALSE;
  Record* rec = 0;
  Image* img = 0;
  FromAsso* af = 0;
  Document* doc = 0;
  AssoCarac* ac = 0;
  RGIT* curr = 0;
 
  checkCollection(coll);
  checkArchive(self);
  logMemory(LOG_DEBUG, "delArchive %s:%lli", 
	  self->hash, (long long int) self->size);

  // delete archive from record ring
  curr = 0;
  while((rec = rgNext_r(self->records, &curr)) != 0)
    if (!delRecord(coll, rec)) goto error;

  // delete archive from image ring
  curr = 0;
  while((img = rgNext_r(self->images, &curr)) != 0)
    if (!delImage(coll, img)) goto error;

  // delete archive from container ring
  if (!delContainer(coll, self->toContainer)) goto error;

  // delete archive from content association
  curr = 0;
  while((af = rgNext_r(self->fromContainers, &curr)) != 0)
    if (!delFromAsso(coll, af)) goto error;
  
  // delete archive from document ring
  curr = 0;
  while((doc = rgNext_r(self->documents, &curr)) != 0)
    if (!delArchiveFromDocument(coll, self, doc)) goto error;
  
  // delete archive from assoCarac ring
  curr = 0;
  while((ac = rgNext_r(self->assoCaracs, &curr)) != 0) {
    rgRemove(self->assoCaracs);
    destroyAssoCarac(ac);
  }

  // delete archive from collection cache
  if ((curr = rgHaveItem(coll->cacheTree->archives, self))) {
    rgRemove_r(coll->cacheTree->archives, &curr);
  }

  // delete archive from collection ring and free the archive
  avl_delete(coll->archives, self);

  rc = TRUE;
 error:
  return rc;
}

/*=======================================================================
 * Function   : diseaseArchive
 * Description: Disease all archive we can in order to free memory
 * Synopsis   : int diseaseArchive(Collection* coll, Archive* arch)
 * Input      : Collection* coll : where to free
 * Output     : TRUE on success
 =======================================================================*/
int
diseaseArchive(Collection* coll, Archive* self)
{
  int rc = FALSE;

  checkCollection(coll);
  checkArchive(self);
  logMemory(LOG_DEBUG, "diseaseArchive %s:%lli", 
	  self->hash, (long long int) self->size);

  // check images ring
  if (self->images->nbItems >0) goto next;
  
  // check container's rings
  if (self->fromContainers->nbItems >0) goto next;
  if (self->toContainer) goto next;
  
  // check document's rings
  if (self->documents->nbItems >0) goto next;
  if (self->assoCaracs->nbItems >0) goto next;
  
  // check records ring
  if (self->records->nbItems >0) goto next;
  
  // check cache's rings
  if (self->demands->nbItems >0) goto next;
  if (self->remoteSupplies->nbItems >0) goto next;
  if (self->finaleSupplies->nbItems >0) goto next;
  if (self->localSupply != 0) goto next;
  
  // delete archive from collection ring and free it
  avl_delete(coll->archives, self);

 next:
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "diseaseArchive fails");
  }
  return rc;
}
/*=======================================================================
 * Function   : diseaseArchives
 * Description: Disease all archive we can in order to free memory
 * Synopsis   : int diseaseArchives(Collection* coll)
 * Input      : Collection* coll : where to free
 * Output     : TRUE on success
 =======================================================================*/
int
diseaseArchives(Collection* coll)
{
  int rc = FALSE;
  Archive* arch = 0;
  AVLNode *node = 0;
  AVLNode *next = 0;
 
  if(coll == 0) goto error;
  logMemory(LOG_DEBUG, "diseaseArchives %s", coll);

  // for each archive host by the collection,
  if ((node = coll->archives->head)) {
    do {
      next = node->next;
      arch = (Archive*)node->item;
      if (!diseaseArchive(coll, arch)) goto error;
    }
    while ((node = next));
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "diseaseArchives fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
