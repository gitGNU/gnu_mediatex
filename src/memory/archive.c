/*=======================================================================
 * Version: $Id: archive.c,v 1.2 2014/11/13 16:36:27 nroche Exp $
 * Project: MediaTeX
 * Module : archive
 *
 * archive producer interface

 MediaTex is an Electronic Archives Management System
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
#include "archive.h"

#include <string.h> // memset
#include <avl.h>

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
  Archive* rc = NULL;
  static int id = 0;

  if ((rc = (Archive*)malloc(sizeof(Archive))) == NULL)
    goto error;
   
  memset(rc, 0, sizeof(Archive));
  if ((rc->images = createRing()) == NULL) goto error;
  if ((rc->fromContainers = createRing()) == NULL) goto error;
  if ((rc->documents = createRing()) == NULL) goto error;
  if ((rc->assoCaracs = createRing()) == NULL) goto error;
  if ((rc->records = createRing()) == NULL) goto error;
  if ((rc->demands = createRing()) == NULL) goto error;
  if ((rc->remoteSupplies = createRing()) == NULL) goto error;
  if ((rc->finaleSupplies = createRing()) == NULL) goto error;
  rc->extractScore = -1; // still not computed
  rc->imageScore = -1;   // still not computed
  rc->state = UNUSED;
  rc->id = id++;

  return rc;
 error:
  logEmit(LOG_ERR, "%s", "malloc: cannot create Archive");
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
  Archive* rc = NULL;

  if(self == NULL) goto error;

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
 * Output     : the state string or NULL on error
 =======================================================================*/
char* 
strAState(AState self)
{
  static char* typeLabels[ASTATE_MAX] =
    {"UNUSED", "USED", "WANTED", "ALLOCATED", "AVAILABLE",  "TOKEEP"};

  if (self < 0 || self >= ASTATE_MAX) {
    logEmit(LOG_WARNING, "unknown archive type %i", self);
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
  Archive* rc = NULL;
  Archive archive;
  AVLNode* node = NULL;

  checkCollection(coll);
  checkLabel(hash);
  logEmit(LOG_DEBUG, "getArchive %s:%lli", hash, (long long int) size);

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
  Archive* rc = NULL;
  Archive* archive = NULL;

  checkCollection(coll);
  checkLabel(hash);
  logEmit(LOG_DEBUG, "addArchive %s:%lli", hash, (long long int) size);
  
  // already there
  if ((archive = getArchive(coll, hash, size))) goto end;

  // add new one if not already there
  if (!(archive = createArchive())) goto error;
  strncpy(archive->hash, hash, MAX_SIZE_HASH);
#if 1
  archive->hash[MAX_SIZE_HASH] = (char)0; // developpement code
#endif
  archive->size = size;
  if (!avl_insert(coll->archives, archive)) goto error;

 end:
  rc = archive;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "addArchive fails");
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
  Record* rec = NULL;
  Image* img = NULL;
  FromAsso* af = NULL;
  Document* doc = NULL;
  AssoCarac* ac = NULL;
  RGIT* curr = NULL;
 
  checkCollection(coll);
  checkArchive(self);
  logEmit(LOG_DEBUG, "delArchive %s:%lli", 
	  self->hash, (long long int) self->size);

  // delete archive from record ring
  curr = NULL;
  while((rec = rgNext_r(self->records, &curr)) != NULL)
    if (!delRecord(coll, rec)) goto error;

  // delete archive from image ring
  curr = NULL;
  while((img = rgNext_r(self->images, &curr)) != NULL)
    if (!delImage(coll, img)) goto error;

  // delete archive from container ring
  if (!delContainer(coll, self->toContainer)) goto error;

  // delete archive from content association
  // (should be already done by above call)
  curr = NULL;
  while((af = rgNext_r(self->fromContainers, &curr)) != NULL)
    if (!delFromAsso(coll, af, TRUE)) goto error;
  
  // delete archive from document ring
  curr = NULL;
  while((doc = rgNext_r(self->documents, &curr)) != NULL)
    if (!delArchiveFromDocument(coll, self, doc)) goto error;
  
  // delete archive from assoCarac ring
  curr = NULL;
  while((ac = rgNext_r(self->assoCaracs, &curr)) != NULL) {
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
  logEmit(LOG_DEBUG, "diseaseArchive %s:%lli", 
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
  if (self->localSupply != NULL) goto next;
  
  // delete archive from collection ring and free it
  avl_delete(coll->archives, self);

 next:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "diseaseArchive fails");
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
  Archive* arch = NULL;
  AVLNode *node = NULL;
  AVLNode *next = NULL;
 
  if(coll == NULL) goto error;
  logEmit(LOG_DEBUG, "diseaseArchives %s", coll);

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
    logEmit(LOG_ERR, "%s", "diseaseArchives fails");
  }
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

  memoryOptions();
  //fprintf(stderr, "\t\t---\n");
  return;
}

/*=======================================================================
 * Function   : main
 * Author     : Nicolas Roche
 * modif      : 2012/02/11
 * Description: Unit test for md5sumTree module.
 * Synopsis   : utmd5sumTree
 * Input      : N/A
 * Output     : /tmp/mdtx/var/local/cache/mdtx/md5sums.txt
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = NULL;
  Archive* arch1 = NULL;
  Archive* arch2 = NULL;
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
  // test types on this architecture:
  off_t offset = 0xFFFFFFFFFFFFFFFFULL; // 2^64
  time_t timer = 0x7FFFFFFF; // 2^32
  logEmit(LOG_DEBUG, "type off_t is store into %u bytes", 
	  (unsigned int)sizeof(off_t));
  logEmit(LOG_DEBUG, "type time_t is store into %u bytes", 
	  (unsigned int) sizeof(time_t));
  logEmit(LOG_DEBUG,  "off_t max value is: %llu", 
	  (unsigned long long int)offset);
  logEmit(LOG_DEBUG, "time_t max value is: %lu", 
	  (unsigned long int)timer);

  if (!createExempleConfiguration()) goto error;
  if (!(coll = getCollection("coll1"))) goto error;

  // creating an archive 2 times return the same object
  if (!(arch1 =
  	addArchive(coll, "780968014038afcbae5c6feca2e630de", 81)))
    goto error;
  if (!(arch2 =
  	addArchive(coll, "780968014038afcbae5c6feca2e630de", 81)))
    goto error;
  if (arch1 != arch2) goto error;
  
  // search an archive
  if (getArchive(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 22222))
    goto error;
  if (!(arch1 =
  	addArchive(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 22222)))
    goto error;
  if (getArchive(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 22222) != arch1)
    goto error;

  if (avl_count(coll->archives) != 2) goto error;
  if (!(diseaseArchives(coll))) goto error;
  if (avl_count(coll->archives) != 0) goto error;
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
