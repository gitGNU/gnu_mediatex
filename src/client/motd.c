/*=======================================================================
 * Project: MediaTeX
 * Module : motd
 *
 * Manage message of the day

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
#include "client/mediatex-client.h"

typedef struct MotdSupport {

  Support* support;
  int ask4periodicCheck;
  RG* collections;

} MotdSupport;

typedef struct MotdRecord {

  Collection* coll;
  RG* records;

} MotdRecord;

typedef struct Motd {
  
  RG* motdAskSupports; // MotdSupport*: support to ask for
  RG* motdAddRecords;  // MotdRecord*: records to burn
  RG* motdDelSupports; // MotdSupport*: support to destroy
  
} Motd;

typedef struct MotdSearch {
  
  Collection* coll;
  int isAvailable;
  RG* imagesWanted; // *Archives
  
} MotdSearch;

int motdArchive(MotdSearch* data, Archive* archive, int depth);

/*=======================================================================
 * Function   : destroyMotdSupport
 * Description: Destroy an object by freeing all the allocate memory.
 * Synopsis   : void destroyMotdSupport(MotdSearch* self)
 * Input      : MotdSupport* self = the address of the object to destroy.
 * Output     : NULL address
 =======================================================================*/
MotdSupport* 
destroyMotdSupport(MotdSupport* self)
{
  MotdSupport* rc = 0;

  if (!self) goto error;

  // delete collection association
  self->collections = destroyOnlyRing(self->collections);
  
  free(self);
  
 error:
  return rc;
}

/*=======================================================================
 * Function   : createMotdSupport
 * Description: Create, by memory allocation a motd support object
 * Synopsis   : Motd* createMotdSupport(void)
 * Input      : N/A
 * Output     : The address of the created empty object
 =======================================================================*/
MotdSupport* 
createMotdSupport(void)
{
  MotdSupport* rc = 0;

  if (!(rc = (MotdSupport*)malloc(sizeof(MotdSupport)))) goto error;
  memset(rc, 0, sizeof(MotdSupport));
  if ((rc->collections = createRing()) == 0) goto error;

  return rc;
 error:
  logMain(LOG_ERR, "malloc: cannot create MotdSuport");
  return destroyMotdSupport(rc);
}

/*=======================================================================
 * Function   : destroyMotdRecord
 * Description: Destroy an object by freeing all the allocate memory.
 * Synopsis   : void destroyMotdRecord(MotdRecord* self)
 * Input      : MotdRecord* self = the address of the object to destroy.
 * Output     : NULL address
 =======================================================================*/
MotdRecord* 
destroyMotdRecord(MotdRecord* self)
{
  MotdRecord* rc = 0;

  if (!self) goto error;

  // delete records association
  self->records = destroyOnlyRing(self->records);
  
  free(self);
  
 error:
  return rc;
}

/*=======================================================================
 * Function   : createMotdRecord
 * Description: Create, by memory allocation a motd support object
 * Synopsis   : Motd* createMotdRecord(void)
 * Input      : N/A
 * Output     : The address of the created empty object
 =======================================================================*/
MotdRecord* 
createMotdRecord(void)
{
  MotdRecord* rc = 0;

  if (!(rc = (MotdRecord*)malloc(sizeof(MotdRecord)))) goto error;
  memset(rc, 0, sizeof(MotdRecord));
  if ((rc->records = createRing()) == 0) goto error;

  return rc;
 error:
  logMain(LOG_ERR, "malloc: cannot create MotdRecord");
  return destroyMotdRecord(rc);
}

/*=======================================================================
 * Function   : destroyMotd
 * Description: Destroy a motd object by freeing all the allocate memory.
 * Synopsis   : void destroyMotd(Motd* self)
 * Input      : Motd* self = the address of the motd object to destroy.
 * Output     : NULL address
 =======================================================================*/
Motd* 
destroyMotd(Motd* self)
{
  Motd* rc = 0;

  if (!self) goto error;

  self->motdAskSupports
    = destroyRing(self->motdAskSupports,
		  (void*(*)(void*)) destroyMotdSupport);

  self->motdAddRecords
    = destroyRing(self->motdAddRecords,
		  (void*(*)(void*)) destroyMotdRecord);

  self->motdDelSupports
    = destroyRing(self->motdDelSupports,
		  (void*(*)(void*)) destroyMotdSupport);
    
  free(self);  
 error:
  return rc;
}

/*=======================================================================
 * Function   : createMotd
 * Description: Create, by memory allocation a motd object
 * Synopsis   : Motd* createMotd(void)
 * Input      : N/A
 * Output     : The address of the created empty object
 =======================================================================*/
Motd* 
createMotd(void)
{
  Motd* rc = 0;

  if ((rc = (Motd*)malloc(sizeof(Motd))) == 0)
    goto error;
   
  memset(rc, 0, sizeof(Motd));
  if ((rc->motdAskSupports = createRing()) == 0) goto error;
  if ((rc->motdAddRecords = createRing()) == 0) goto error;
  if ((rc->motdDelSupports = createRing()) == 0) goto error;
 
  return rc;
 error:
  logMain(LOG_ERR, "malloc: cannot create Motd");
  return destroyMotd(rc);
}

/*=======================================================================
 * Function   : cmpMotdSupport
 * Description: Compare 2 image files
 * Synopsis   : int cmpMotdSupport(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2: the image to compare
 * Output     : like strcmp
 =======================================================================*/
int 
cmpMotdSupport(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be MotdSupport* 
   */
  
  MotdSupport* s1 = *((MotdSupport**)p1);
  MotdSupport* s2 = *((MotdSupport**)p2);

  rc = cmpSupport(&s1->support, &s2->support);
  return rc;
}

/*=======================================================================
 * Function   : addMotdSupport
 * Description: Add an support if not already there
 * Synopsis   : MotdSupport* addMotdSupport(RG* ring, Support* support,
 *                                          Collection* coll)
 * Input      : RG* ring: motd->motdAskSupports or motd->motdDelSupports
 *              Support* support: support to add
 *              Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int 
addMotdSupport(RG* ring, Support* support, Collection* coll)
{
  int rc = FALSE;
  MotdSupport* motdSupp = 0;
  RGIT* curr = 0;
   
  logMain(LOG_DEBUG, "addMotdSupport %s form %s", 
	  support->name, coll->label);
  checkSupport(support);
  if (!ring) goto error;

  // add motSupport if not already there
  while ((motdSupp = rgNext_r(ring, &curr))) {
    if (!cmpSupport(&motdSupp->support, &support)) break;
  } 
  if (!motdSupp) {
    if (!(motdSupp = createMotdSupport())) goto error;
    if (!rgInsert(ring, motdSupp)) goto error;
  }

  // register who add this notification on support
  motdSupp->support = support;
  if (!coll) {
    motdSupp->ask4periodicCheck = TRUE;
  }
  else {
    // add collection if not already there
    if (!(rgHaveItem(motdSupp->collections, coll))) {
      if (!rgInsert(motdSupp->collections, coll)) goto error;
    }
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "addMotdSupport fails");
    destroyMotdSupport(motdSupp);
  }
  return rc;
}

/*=======================================================================
 * Function   : motdContainer
 * Description: Find image related to a container
 * Synopsis   : int motdContainer(MotdSearch* data, 
 *                                Container* container, int depth)
 * Input      : Collection* coll = context
 *              Container* container = what to motd
 *              int depth: use to indent logs
 * Output     : int *isAvailable = TRUE when image is already available
 *              FALSE on error
 =======================================================================*/
int motdContainer(MotdSearch* data, Container* container, int depth)
{
  int rc = FALSE;
  Archive* archive = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "%*slook for container %s/%s:%lli", 
	  depth, "", strEType(container->type),
	  container->parent->hash,
	  (long long int)container->parent->size);

  data->isAvailable = FALSE;
  checkCollection(data->coll);

  if (isEmptyRing(container->parents)) goto end;

  // we motd all parents (only one for TGZ, several for CAT...)
  while ((archive = rgNext_r(container->parents, &curr))) {
    if (!motdArchive(data, archive, depth+1)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%*smotdContainer fails", depth, "");
  }
  return rc;
} 

/*=======================================================================
 * Function   : motdArchive
 * Description: Find image related to an archive
 * Synopsis   : int motdArchive(MotdSearch* data, Archive* archive, 
 *                              int depth)
 * Input      : Collection* coll = context
 *              Archive* archive = what to motd
 *              int depth: use to indent logs
 * Output     : int *isAvailable = TRUE image is already available
 *              FALSE on error
 =======================================================================*/
int motdArchive(MotdSearch* data, Archive* archive, int depth)
{
  int rc = FALSE;
  Support *supp = 0;
  FromAsso* asso = 0;
  RGIT* curr = 0;
  int nbSupports = 0;
  int nbSupportFiles = 0;

  logMain(LOG_DEBUG, "%*slook for archive: %s:%lli", 
	  depth, "", archive->hash, archive->size);
  checkCollection(data->coll);
  data->isAvailable = FALSE;

  if (archive->state >= AVAILABLE) {
    logMain(LOG_DEBUG, "%*savailable from cache", depth, "");
    data->isAvailable = TRUE;
    goto end;
  }

  // look for a matching image 
  if (archive->images->nbItems > 0) {
    logMain(LOG_INFO, "%*s%i images matched", depth, "",
	    archive->images->nbItems);

    // look for matching physical supports (but not support files)
    rgRewind(data->coll->supports);
    while ((supp = rgNext(data->coll->supports))) {
      if (supp->size != archive->size) continue;
      if (strcmp(supp->fullMd5sum, archive->hash)) continue;
      ++nbSupports;
      if (*supp->name != '/') continue;
      ++nbSupportFiles;
    }
    if (nbSupportFiles) {
      // note: support files are loaded as final supplies by server
      logMain(LOG_INFO, "%*s%i support files matched", depth, "",
	      nbSupportFiles);
    }
    else {
      if (nbSupports) {
	// we need to re-loop as they may be several matching supports
	if (!rgInsert(data->imagesWanted, archive)) goto error;
      }
    }
  }

  // continue searching ; we stop at the first container already there
  while (!data->isAvailable && 
	(asso = rgNext_r(archive->fromContainers, &curr))) {
    if (!motdContainer(data, asso->container, depth+1)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%*smotdArchive fails", depth, "");
  }
  return rc;
} 

/*=======================================================================
 * Function   : updateMotdFromMd5sumsDB
 * Description: work on record tree the server manage
 * Synopsis   : int updateMotdFromMd5sumsDB(Motd* motd, Collection* coll, 
 *                                          MotdRecord* motdRecord)
 * Input      : Motd* motd: where we write results 
 *              Collection* coll: collection we are working on
 *              MotdRecord* motdRecord: where we write records to burn
 * Output     : TRUE on success
 * TODO       : finish test covering within utmotd
 =======================================================================*/
int
updateMotdFromMd5sumsDB(Motd* motd, Collection* coll, 
			MotdRecord* motdRecord)
{
  int rc = FALSE;
  Archive* archive = 0;
  Support* support = 0;
  AVLNode* node = 0;
  MotdSearch data;
  Image* image = 0;
  MotdPolicy motdPolicy = mUNDEF;
  MotdSupport* motdSupp = 0;
  FromAsso* asso = 0;
  RGIT* curr = 0;
  int doIt = FALSE;

  logMain(LOG_DEBUG, "updateMotdFromMd5sumsDB for %s collection",
	  coll->label);
  memset(&data, 0, sizeof(MotdSearch));

  if ((motdPolicy = getMotdPolicy(coll)) == mUNDEF) goto error;
  if (!loadCollection(coll, SERV|EXTR|CACH)) goto error;
  if (!(data.imagesWanted = createRing())) goto error2;
  data.coll = coll;

  logRecordTree(LOG_MAIN, LOG_DEBUG, coll->cacheTree->recordTree, 0);  
  
  // 1) for each remote demand, check image we can provide
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;

    // scan wanted archives not available
    if (archive->state != WANTED) continue;
    if (!motdArchive(&data, archive, 0)) goto error2;
  }

  // match supports related to images previously selected
  rgRewind(data.imagesWanted);
  while ((archive = rgNext(data.imagesWanted))) {
    
    // match support shared with that collection
    rgRewind(coll->supports);
    while ((support = rgNext(coll->supports))) {
      if (!strncmp(support->fullMd5sum, archive->hash, MAX_SIZE_MD5) &&
	  support->size == archive->size) {
	
	// add a motd notification on support (to provide)
	if (!addMotdSupport(motd->motdAskSupports, support, coll))
	  goto error2;
      }
    }
  }   
  
  // 2) for each cache entry, check what we can burn (as local supports)
  motdRecord->coll = coll;
  if (!computeExtractScore(coll)) goto error2;
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    
    // looking for archive available into the cache
    if (archive->state < AVAILABLE) continue;
    doIt = FALSE;
    
    // if motd policy is ALL...
    if (motdPolicy == ALL &&
	// ...looking for top container...
	!archive->fromContainers->nbItems &&
	// ...that have no local image...
	(!(image = getImage(coll, coll->localhost, archive)))) {
      doIt = TRUE;
    }

    // looking for bad top container...
    if (!doIt && isBadTopContainer(coll, archive) &&
	// ...that have no local image...
	(!(image = getImage(coll, coll->localhost, archive)) ||
	 // ...or a local bad score
	 image->score <= coll->serverTree->scoreParam.maxScore / 2)) {
      doIt = TRUE;
    }
    
    if (doIt) {
      // add a motd notification on record (to burn)
      if (!rgInsert(motdRecord->records, archive->localSupply))
	goto error2;
    }
  }

  // 3) for each support, check support we can withdraw
  rgRewind(coll->supports);
  while ((support = rgNext(coll->supports))) {
    if (!(archive =
	  getArchive(coll, support->fullMd5sum, support->size)))
      goto error;

    // assert it is included into another safe support
    curr = 0;
    while ((asso = rgNext_r(archive->fromContainers, &curr))) {
      if (asso->container->score >
	  coll->serverTree->scoreParam.maxScore / 2) break;
    }
    if (!asso) continue;

    /* TODO: not tested by utmotd yet */
    
    // assert we do not already ask for it
    curr = 0;
    while ((motdSupp = rgNext_r(motd->motdAskSupports, &curr))) {
      if (!cmpSupport(&motdSupp->support, &support)) break;
    } 
    if (motdSupp) continue;
    
    if (!addMotdSupport(motd->motdDelSupports, support, coll))
      goto error;
  }
  
  rc = TRUE; 
 error2:
  if (!releaseCollection(coll, SERV|EXTR|CACH)) rc = 0;
 error:
  if (!rc) {
    logMain(LOG_ERR, "updateMotdFromMd5sumsDB on %s collection",
	    coll->label);
  }
  destroyOnlyRing(data.imagesWanted);
  return rc;
}

/*=======================================================================
 * Function   : updateMotdFromAllMd5sumsDB
 * Description: List all requested supports
 * Synopsis   : SupportTree* updateMotdAllFromMd5sumsDB(
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int
updateMotdFromAllMd5sumsDB(Motd* motd)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  MotdRecord* motdRecord = 0;

  logMain(LOG_DEBUG, "updateMotdFromAllMd5sumsDB");
  if (!(conf = getConfiguration())) goto error;

  if (!env.noRegression) {
    // ask daemon to update md5sumsDB file
    if (!mdtxSyncSignal(REG_SAVEMD5)) {
      logMain(LOG_WARNING, "fail to update md5sumsDB file");
    }
  }

  if (conf->collections == 0) goto error;
  rgRewind(conf->collections);
  while ((coll = rgNext(conf->collections))) {

    // assert we have the localhost server object
    if (!getLocalHost(coll)) goto error;
    
    // match wanted records to local images and find record to burn
    if (!(motdRecord = createMotdRecord())) goto error;
    if (!updateMotdFromMd5sumsDB(motd, coll, motdRecord)) goto error;
    if (!rgInsert(motd->motdAddRecords, motdRecord)) goto error;
    motdRecord = 0;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "updateMotdFromAllMd5sumsDB fails");
  }
  destroyMotdRecord(motdRecord);
  return rc;
}

/*=======================================================================
 * Function   : updateMotdFromSupportDB
 * Description: List not seens for a while supports
 * Synopsis   : SupportTree* updateMotdFromSupportDB(Motd* motd)
 * Input      : Motd* motd: where we write results 
 * Output     : TRUE on success
 =======================================================================*/
int
updateMotdFromSupportDB(Motd* motd)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support *support = 0;
  MotdSupport* motdSupp = 0;
  RGIT* curr = 0;
  
  logMain(LOG_DEBUG, "updateMotdFromSupportDB");
  if (!(conf = getConfiguration())) goto error;

  // notify supports we no more use
  rgRewind(conf->supports);
  while ((support = rgNext(conf->supports))) {
    if (!isEmptyRing(support->collections)) continue;

    // assert we do not already aked for it
    while ((motdSupp = rgNext_r(motd->motdAskSupports, &curr))) {
      if (!cmpSupport(&motdSupp->support, &support)) break;
    } 
    if (motdSupp) continue;
    
    // tels it is not used by any collection
    if (!addMotdSupport(motd->motdDelSupports, support, 0)) goto error;
  }
  
  // notify supports we need to check
  rgRewind(conf->supports);
  while ((support = rgNext(conf->supports))) {
    
    // assert support is not already tells to be unused
    while ((motdSupp = rgNext_r(motd->motdDelSupports, &curr))) {
      if (!cmpSupport(&motdSupp->support, &support)) break;
    } 
    if (motdSupp) continue;
    
    // obsolete: need to be checked
    if (!scoreSupport(support, &conf->scoreParam)) goto error; 
    if (support->score) continue;

    // tels we do not see this support for too long time, and that
    // we need the Publisher to provides it.
    if (!addMotdSupport(motd->motdAskSupports, support, 0)) goto error;
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "updateMotdFromSupportDB fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : updateMotd
 * Description: Print the motd
 * Synopsis   : int updateMotd()
 * Input      : N/A
 * Output     : TRUE on success
 * MAYBE      : show sharable supports?
 * TODO       : reply above question
 =======================================================================*/
int 
updateMotd()
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  Motd* motd = 0;
  MotdSupport* motdSupp = 0;
  MotdRecord* motdRec = 0;
  Record* record = 0;
  char car = ':';
  int isFileToBurn = 0;
  int isAllowed = 0;

  logMain(LOG_DEBUG, "updateMotd");

  if (!allowedUser(env.confLabel, &isAllowed, 0)) goto error;
  if (!isAllowed) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!loadConfiguration(CFG|SUPP)) goto error;
  if (!expandConfiguration()) goto error;
  if (!(motd = createMotd())) goto error;
  
  if (!updateMotdFromAllMd5sumsDB(motd)) {
    logMain(LOG_ERR, "cannot update motd from md5sumsDB");
    goto error;
  }

  if (!updateMotdFromSupportDB(motd))  {
    logMain(LOG_ERR, "cannot update motd from supportDB");
    goto error;
  }
  
  printf("\n%s\n", "*****************************");
  printf("%s\n", "Mediatex's message of the day");
  printf("%s\n", "*****************************");

  // supports to ask for
  if (!isEmptyRing(motd->motdAskSupports)) {
    printf("Please provide theses local supports:\n");
    if (!rgSort(motd->motdAskSupports, cmpMotdSupport)) goto error;
    while ((motdSupp = rgNext(motd->motdAskSupports))) {
      printf("- %s", motdSupp->support->name);
      car = ':';
      if (motdSupp->ask4periodicCheck) {
	printf("%c %s", car, "periodic check");
	car = ',';
      }
      if (motdSupp->collections) {
	rgRewind(motdSupp->collections);
	while ((coll = rgNext(motdSupp->collections))) {
	  printf("%c %s", car, coll->label);
	  car = ',';
	}
      }
      printf("%s", "\n");
    }
  }
  
  // records to burn
  while ((motdRec = rgNext(motd->motdAddRecords))) {
      if (isEmptyRing(motdRec->records)) continue;
      isFileToBurn++;
  }
  rgRewind(motd->motdAddRecords);
  if (isFileToBurn) {
    printf("Please burn theses files:\n"); 
    while ((motdRec = rgNext(motd->motdAddRecords))) {
      if (isEmptyRing(motdRec->records)) continue;
      printf("- Collection %s:\n", motdRec->coll->label); 
      rgSort(motdRec->records, cmpRecordPath);
      rgRewind(motdRec->records);
      while ((record = rgNext(motdRec->records))) {
	printf(" - %s/%s (%5.2f)\n", motdRec->coll->cacheDir,
	       record->extra, record->archive->extractScore); 
      }
    }
  }

  /* TODO: check why
     Please remove theses local supports:
     - /HERE/misc/logoP1.iso: not used
  */

  // supports to destroy
  if (!isEmptyRing(motd->motdDelSupports)) {
    printf("Please remove theses local supports:\n");
    if (!rgSort(motd->motdDelSupports, cmpMotdSupport)) goto error;
    while ((motdSupp = rgNext(motd->motdDelSupports))) {
      printf("- %s", motdSupp->support->name);
      car = ':';
      if (motdSupp->ask4periodicCheck) {
	printf("%c %s", car, "not used");
	car = ',';
      }
      if (motdSupp->collections) {
	rgRewind(motdSupp->collections);
	while ((coll = rgNext(motdSupp->collections))) {
	  printf("%c %s", car, coll->label);
	  car = ',';
	}
      }
      printf("%s", "\n");
    }
  }
  
  // nothing to do
  if (isEmptyRing(motd->motdAskSupports) &&
      !isFileToBurn &&
      isEmptyRing(motd->motdDelSupports)) {
    printf("nothing to do\n");
  }

  printf("\n");
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to update motd");
  }
  destroyMotd(motd);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
