/*=======================================================================
 * Version: $Id: motd.c,v 1.13 2015/09/22 23:05:56 nroche Exp $
 * Project: MediaTeX
 * Module : motd
 *
 * Manage message of the day

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
  
  RG* motdSupports; // MotdSupport*: support to ask for
  RG* motdRecords;  // MotdRecord*: records to burn
  
} Motd;

typedef struct MotdSearch {
  
  Collection* coll;
  int isAvailable;
  RG* imagesWanted; // *Archives
  
} MotdSearch;

int motdArchive(MotdSearch* data, Archive* archive);

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

  self->motdSupports
    = destroyRing(self->motdSupports,
		  (void*(*)(void*)) destroyMotdSupport);

  self->motdRecords
    = destroyRing(self->motdRecords,
		  (void*(*)(void*)) destroyMotdRecord);

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
  if ((rc->motdSupports = createRing()) == 0) goto error;
  if ((rc->motdRecords = createRing()) == 0) goto error;

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
 * Synopsis   : MotdSupport* addMotdSupport(
 * Input      : 
 * Output     : TRUE on success
 =======================================================================*/
int 
addMotdSupport(Motd* motd, Support* support, Collection* coll)
{
  int rc = FALSE;
  MotdSupport* motdSupp = 0;
  RGIT* curr = 0;
   
  logMain(LOG_DEBUG, "addMotdSupport %s form %s", 
	  support->name, coll->label);
  checkSupport(support);
  if (!motd) goto error;

  // add motSupport if not already there
  while ((motdSupp = rgNext_r(motd->motdSupports, &curr))) {
    if (!cmpSupport(&motdSupp->support, &support)) break;
  } 
  if (!motdSupp) {
    if (!(motdSupp = createMotdSupport())) goto error;
    if (!rgInsert(motd->motdSupports, motdSupp)) goto error;
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
 * Function   : updateMotdFromSupportDB
 * Description: List all obsolete supports
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
  
  logMain(LOG_DEBUG, "updateMotdFromSupportDB");
  if (!(conf = getConfiguration())) goto error;

  // notify bad supports
  rgRewind(conf->supports);
  while ((support = rgNext(conf->supports))) {
    if (!scoreSupport(support, &conf->scoreParam)) goto error;
    if (support->score <= conf->scoreParam.badScore) {
      if (!addMotdSupport(motd, support, 0)) goto error;
    }
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "updateMotdFromSupportDB fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : motdContainer
 * Description: Find image related to a container
 * Synopsis   : int motdContainer
 * Input      : Collection* coll = context
 *              Container* container = what to motd
 * Output     : int *isAvailable = TRUE when image is already available
 *              FALSE on error
 =======================================================================*/
int motdContainer(MotdSearch* data, Container* container)
{
  int rc = FALSE;
  Archive* archive = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "motd a container %s/%s:%lli", 
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size);

  data->isAvailable = FALSE;
  checkCollection(data->coll);

  if (isEmptyRing(container->parents)) goto end;

#warning TODO: try to remove this line
  //data->isAvailable = TRUE; // is this needed ?

  // we motd all parents (only one for TGZ, several for CAT...)
  while ((archive = rgNext_r(container->parents, &curr))) {
    if (!motdArchive(data, archive)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "motdContainer fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : motdArchive
 * Description: Find image related to an archive
 * Synopsis   : int motdArchive
 * Input      : Collection* coll = context
 *              Archive* archive = what to motd
 * Output     : int *isAvailable = TRUE image is already available
 *              FALSE on error
 =======================================================================*/
int motdArchive(MotdSearch* data, Archive* archive)
{
  int rc = FALSE;
  FromAsso* asso = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "motd an archive: %s:%lli", 
	  archive->hash, archive->size);

  data->isAvailable = FALSE;
  checkCollection(data->coll);

  if (archive->state == AVAILABLE) {
    data->isAvailable = TRUE;
    goto end;
  }

  // look for a matching image 
  if (archive->images->nbItems > 0) {
    logMain(LOG_DEBUG, "archive match %i images", archive->images->nbItems);
    if (!rgInsert(data->imagesWanted, archive)) goto error;
  }

  // continue searching ; we stop at the first container already there
  while (!data->isAvailable && 
	(asso = rgNext_r(archive->fromContainers, &curr))) {
    if (!motdContainer(data, asso->container)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "motdArchive fails");
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
 =======================================================================*/
int
updateMotdFromMd5sumsDB(Motd* motd, Collection* coll, 
			MotdRecord* motdRecord)
{
  int rc = FALSE;
  Archive* archive = 0;
  Support* support = 0;
  RGIT* curr = 0;
  MotdSearch data;
  Image* image = 0;

  logMain(LOG_DEBUG, "updateMotdFromMd5sumsDB for %s collection",
	  coll->label);
  memset(&data, 0, sizeof(MotdSearch));

  if (!loadCollection(coll, SERV|EXTR|CACH)) goto error;
  if (!(data.imagesWanted = createRing())) goto error2;
  data.coll = coll;
  
  // 1) for each cache entry
  while ((archive = rgNext_r(coll->cacheTree->archives, &curr))) {

    // scan wanted archives not available
    if (archive->state != WANTED) continue;
    if (!motdArchive(&data, archive)) goto error2;
  }
  
  // for images, match supports
  rgRewind(data.imagesWanted);
  while ((archive = rgNext(data.imagesWanted))) {
    
    // match support shared with that collection
    rgRewind(coll->supports);
    while ((support = rgNext(coll->supports))) {
      if (!strncmp(support->fullMd5sum, archive->hash, MAX_SIZE_MD5) &&
	  support->size == archive->size) {
	
	// add a motd notification on support (to check)
	if (!addMotdSupport(motd, support, coll)) goto error2;
      }
    }
  }   
  
  // 2) for each cache entry
  motdRecord->coll = coll;
  if (!computeExtractScore(coll)) goto error2;
  curr = 0;
  while ((archive = rgNext_r(coll->cacheTree->archives, &curr))) {

    // looking for top containers as content cannot have worse score
    if (!isEmptyRing(archive->fromContainers)) continue;

    // looking for archive having a bad score
    if (archive->extractScore > coll->serverTree->scoreParam.maxScore /2)
      continue;      
    
    // looking for archive available into the cache
    if (archive->state < AVAILABLE) continue;
 
    // looking for archive that have a no local image...
    if ((image = getImage(coll, coll->localhost, archive)) &&
	// ...or a local bad score
	image->score > coll->serverTree->scoreParam.maxScore / 2)
      continue;

    // add a motd notification on record (to burn)
    if (!rgInsert(motdRecord->records, archive->localSupply)) goto error2;
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
    if (!mdtxSyncSignal(MDTX_SAVEMD5)) {
      logMain(LOG_WARNING, "fail to update md5sumsDB file");
    }
  }

  if (conf->collections == 0) goto error;
  rgRewind(conf->collections);
  while ((coll = rgNext(conf->collections))) {

    logRecordTree(LOG_MAIN, LOG_DEBUG, coll->cacheTree->recordTree, 0);
    
    // assert we have the localhost server object
    if (!getLocalHost(coll)) goto error;
    
    // match wanted records to local images and find record to burn
    if (!(motdRecord = createMotdRecord())) goto error;
    if (!updateMotdFromMd5sumsDB(motd, coll, motdRecord)) goto error;
    if (!rgInsert(motd->motdRecords, motdRecord)) goto error;
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
 * Function   : updateMotd
 * Description: Print the motd
 * Synopsis   : int updateMotd()
 * Input      : N/A
 * Output     : TRUE on success
 * TODO       : show sharable supports ?
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
  //char* text = 0;
  //off_t badSize = 0;
  //char buf[30];

  logMain(LOG_DEBUG, "updateMotd");

  if (!allowedUser(env.confLabel)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!loadConfiguration(CFG|SUPP)) goto error;
  if (!expandConfiguration()) goto error2;
  if (!(motd = createMotd())) goto error2;
  
  if (!updateMotdFromSupportDB(motd))  {
    logMain(LOG_ERR, "cannot update motd from supportDB");
    goto error2;
  }
  
  if (!updateMotdFromAllMd5sumsDB(motd)) {
    logMain(LOG_ERR, "cannot update motd from md5sumsDB");
    goto error2;
  }

  printf("\n%s\n", "*****************************");
  printf("%s\n", "Mediatex's message of the day");
  printf("%s\n", "*****************************");

  // supports to ask for
  if (!rgSort(motd->motdSupports, cmpMotdSupport)) goto error2;
  printf("Please provide theses local supports:\n");
  while ((motdSupp = rgNext(motd->motdSupports))) {
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

  // records to burn
  printf("Please burn theses files:\n"); 
  while ((motdRec = rgNext(motd->motdRecords))) {
    if (isEmptyRing(motdRec->records)) continue;
    printf("- Collection %s:\n", motdRec->coll->label); 
    rgRewind(motdRec->records);
    while ((record = rgNext(motdRec->records))) {
      printf(" - %s/%s (%5.2f)\n", motdRec->coll->cacheDir,
	     record->extra, record->archive->extractScore); 
    }
  }

  /* // display collections global status  */
  /* printf("Looking for content to burn:\n");  */
  /* if (conf->collections == 0) goto error2; */
  /* rgRewind(conf->collections); */
  /* while ((coll = rgNext(conf->collections))) { */
  /*   if (!computeExtractScore(coll)) goto error2; */
  /*   if (!(text = getExtractStatus(coll, &badSize, 0)))  */
  /*     goto error2; */

  /*   if (badSize == 0) goto next; */
  /*   sprintSize(buf, badSize); */
  /*   printf("- %s (%5.2f): %s", coll->label, coll->extractTree->score, buf); */

  /* next: */
  /*   text = destroyString(text); */
  /* } */
  /* printf("\n"); */

  rc = TRUE;
 error2:
  if (!loadConfiguration(CFG|SUPP)) rc = FALSE;
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
