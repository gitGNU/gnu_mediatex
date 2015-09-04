/*=======================================================================
 * Version: $Id: motd.c,v 1.10 2015/09/04 15:30:25 nroche Exp $
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

/* int motdArchive(MotdData* data, Archive* archive); */

/* /\*======================================================================= */
/*  * Function   : updateMotdFromSupportDB */
/*  * Description: List all obsolete supports */
/*  * Synopsis   : SupportTree* updateMotdFromSupportDB() */
/*  * Input      : N/A */
/*  * Output     : SupportTree* = osolete supports list */
/*  =======================================================================*\/ */
/* RG*  */
/* updateMotdFromSupportDB() */
/* { */
/*   RG* rc = 0; */
/*   Configuration* conf = 0; */
/*   Support *support = 0; */

/*   logMain(LOG_DEBUG, "update motd from supports"); */

/*   if (!(conf = getConfiguration())) goto error; */
/*   if (!(rc = createRing())) goto error; */

/*   rgRewind(conf->supports); */
/*   while ((support = rgNext(conf->supports))) { */
/*     if (!scoreSupport(support, &conf->scoreParam)) goto error; */
/*     if (support->score <= conf->scoreParam.badScore) { */
/*       if (!rgInsert(rc, support)) goto error; */
/*     }  */
/*   } */
  
/*   return rc; */
/*  error: */
/*   logMain(LOG_ERR, "fails to update motd from supports"); */
/*   destroyOnlyRing(rc); // do not free support objetcs */
/*   return (RG*)0; */
/* } */

/* /\*======================================================================= */
/*  * Function   : motdContainer */
/*  * Description: Find image related to a container */
/*  * Synopsis   : int motdContainer */
/*  * Input      : Collection* coll = context */
/*  *              Container* container = what to motd */
/*  * Output     : int *found = TRUE when image is already available */
/*  *              FALSE on error */
/*  =======================================================================*\/ */
/* int motdContainer(MotdData* data, Container* container) */
/* { */
/*   int rc = FALSE; */
/*   Archive* archive = 0; */
/*   RGIT* curr = 0; */

/*   logMain(LOG_DEBUG, "motd a container %s/%s:%lli",  */
/* 	  strEType(container->type), container->parent->hash, */
/* 	  (long long int)container->parent->size); */

/*   data->found = FALSE; */
/*   checkCollection(data->coll); */

/*   if (isEmptyRing(container->parents)) goto end; */
/*   data->found = TRUE; */

/*   // we motd all parents (only one for TGZ, several for CAT...) */
/*   while ((archive = rgNext_r(container->parents, &curr))) { */
/*     if (!motdArchive(data, archive)) goto error; */
/*   } */

/*  end: */
/*   rc = TRUE; */
/*  error: */
/*   if (!rc) { */
/*     logMain(LOG_ERR, "motdContainer fails"); */
/*   } */
/*   return rc; */
/* }  */

/* /\*======================================================================= */
/*  * Function   : motdArchive */
/*  * Description: Find image related to an archive */
/*  * Synopsis   : int motdArchive */
/*  * Input      : Collection* coll = context */
/*  *              Archive* archive = what to motd */
/*  * Output     : int *found = TRUE image is already available */
/*  *              FALSE on error */
/*  =======================================================================*\/ */
/* int motdArchive(MotdData* data, Archive* archive) */
/* { */
/*   int rc = FALSE; */
/*   FromAsso* asso = 0; */
/*   RGIT* curr = 0; */

/*   logMain(LOG_DEBUG, "motd an archive: %s:%lli",  */
/* 	  archive->hash, archive->size); */

/*   data->found = FALSE; */
/*   checkCollection(data->coll); */

/*   if (archive->state == AVAILABLE) { */
/*     data->found = TRUE; */
/*     goto end; */
/*   } */

/*   // look for a matching image  */
/*   if (archive->images->nbItems > 0) { */
/*     logMain(LOG_DEBUG, "archive match %i images", archive->images->nbItems); */
/*     if (!rgInsert(data->outArchives, archive)) goto error; */
/*   } */

/*   // continue searching as we stop at the first container already there */
/*   while (!data->found &&  */
/* 	(asso = rgNext_r(archive->fromContainers, &curr))) { */
/*     if (!motdContainer(data, asso->container)) goto error; */
/*   } */

/*  end: */
/*   rc = TRUE; */
/*  error: */
/*   if (!rc) { */
/*     logMain(LOG_ERR, "motdArchive fails"); */
/*   } */
/*   return rc; */
/* }  */

/* /\*======================================================================= */
/*  * Function   : updateMotdFromMd5sumsDB */
/*  * Description: List all requested supports */
/*  * Synopsis   : SupportTree* updateMotdFromMd5sumsDB() */
/*  * Input      : RG* rc = input ring to merge */
/*  * Output     : SupportTree* = list of requested supports */
/*  * Note       : O(records x supports) */
/*  =======================================================================*\/ */
/* RG*  */
/* updateMotdFromMd5sumsDB(RG* ring, Collection* coll) */
/* { */
/*   RG* rc = 0; */
/*   Archive* archive = 0; */
/*   Support* support = 0; */
/*   RGIT* curr = 0; */
/*   MotdData data; */

/*   logMain(LOG_DEBUG, "update motd from server for %s collection", */
/* 	  coll->label); */
/*   memset(&data, 0, sizeof(MotdData)); */

/*   if (!loadCollection(coll, SERV|EXTR|CACH)) goto error; */
/*   if (!(data.outArchives = createRing())) goto error2; */
/*   data.coll = coll; */

/*   // for each cache entry */
/*   while ((archive = rgNext_r(coll->cacheTree->archives, &curr))  */
/* 	!= 0) { */

/*     // scan wanted archives */
/*     if (archive->state != WANTED) continue; */
/*     if (!motdArchive(&data, archive)) goto error2; */
/*   } */

/*   // for each record (that all are images), match supports */
/*   rgRewind(data.outArchives); */
/*   while ((archive = rgNext(data.outArchives))) { */

/*     // match support shared with that collection */
/*     rgRewind(coll->supports); */
/*     while ((support = rgNext(coll->supports))) { */
/*       if (!strncmp(support->fullHash, archive->hash, MAX_SIZE_HASH) && */
/* 	  support->size == archive->size) { */
	
/* 	// add support */
/* 	if (support) { */
/* 	  if (!rgInsert(ring, support)) goto error2; */
/* 	} */
/*       } */
/*     } */

/*   }    */
/*   rc = ring;  */
/*  error2: */
/*   if (!releaseCollection(coll, SERV|EXTR|CACH)) rc = 0; */
/*  error: */
/*   if (!rc) { */
/*     logMain(LOG_ERR, "fails to update motd from server for %s collection", */
/* 	    coll->label); */
/*     destroyOnlyRing(ring); // do not free support objetcs */
/*   } */
/*   destroyOnlyRing(data.outArchives); */
/*   return rc; */
/* } */

/* /\*======================================================================= */
/*  * Function   : updateMotdFromAllMd5sumsDB */
/*  * Description: List all requested supports */
/*  * Synopsis   : SupportTree* updateMotdAllFromMd5sumsDB() */
/*  * Input      : N/A */
/*  * Output     : SupportTree* = list of requested supports */
/*  =======================================================================*\/ */
/* RG*  */
/* updateMotdFromAllMd5sumsDB() */
/* { */
/*   RG* rc = 0; */
/*   RG* ring = 0; */
/*   Configuration* conf = 0; */
/*   Collection* coll = 0; */

/*   logMain(LOG_DEBUG, "update motd from server"); */
/*   if (!(conf = getConfiguration())) goto error; */
/*   if (!(ring = createRing())) goto error; */

/*   if (!env.noRegression) { */
/*     // ask daemon to update md5sumsDB file */
/*     if (!mdtxSyncSignal(MDTX_SAVEMD5)) { */
/*       logMain(LOG_WARNING, "fail to update md5sumsDB file"); */
/*     } */
/*   } */

/*   if (conf->collections == 0) goto error; */
/*   rgRewind(conf->collections); */
/*   while ((coll = rgNext(conf->collections))) { */

/*     // assert we have the localhost server object */
/*     if (!getLocalHost(coll)) goto error; */
    
/*     // match records to local images  */
/*     if (!(ring = updateMotdFromMd5sumsDB(ring, coll))) goto error; */
/*   } */

/*   rc = ring; */
/*  error: */
/*   if (!rc) { */
/*     logMain(LOG_ERR, "fails to update motd from server"); */
/*     rgDelete(ring); // do not free support objects */
/*   } */
/*   return rc; */
/* } */


/* /\*======================================================================= */
/*  * Function   : updateMotd */
/*  * Description: Print the motd */
/*  * Synopsis   : int updateMotd() */
/*  * Input      : N/A */
/*  * Output     : TRUE on success */
/*  * TODO       : show sharable supports ? */
/*  =======================================================================*\/ */
/* int  */
/* updateMotd() */
/* { */
/*   int rc = FALSE; */
/*   Configuration* conf = 0; */
/*   RG* tree1 = 0; */
/*   RG* tree2 = 0; */
/*   Support *support = 0; */
/*   Support *prev = 0; */
/*   Collection* coll = 0; */
/*   char* text = 0; */
/*   off_t badSize = 0; */
/*   char buf[30]; */

/*   logMain(LOG_DEBUG, "updateMotd"); */

/*   if (!allowedUser(env.confLabel)) goto error; */
/*   if (!(conf = getConfiguration())) goto error; */
/*   if (!loadConfiguration(CFG|SUPP)) goto error; */
/*   if (!expandConfiguration()) goto error2; */

/*   if (!(tree1 = updateMotdFromSupportDB()))  { */
/*     logMain(LOG_ERR, "cannot update motd from supportDB"); */
/*     goto error2; */
/*   } */
  
/*   if (!(tree2 = updateMotdFromAllMd5sumsDB())) { */
/*     logMain(LOG_ERR, "cannot update motd from md5sumsDB"); */
/*     goto error2; */
/*   } */

/*   printf("\n%s\n", "*****************************"); */
/*   printf("%s\n", "Mediatex's message of the day"); */
/*   printf("%s\n", "*****************************"); */

/*   if (tree1) { */
/*     if (!rgSort(tree1, cmpSupport)) goto error2; */
/*     printf("Need to check your local supports:\n"); */
/*     while ((support = rgNext(tree1))) { */
/*       printf("- %s\n", support->name); */
/*     } */
/*   } */
 
/*   if (tree2) {    */
/*     if (!rgSort(tree2, cmpSupport)) goto error2;   */
/*     printf("Looking for content from your shared supports:\n");  */
/*     while ((support = rgNext(tree2))) { */
/*       // do not display support twice */
/*       if (!prev || cmpSupport(&support, &prev))  */
/* 	printf("- %s\n", support->name); */
/*       prev = support; */
/*     } */
/*   } */

/*   // display collections global status  */
/*   printf("Looking for content to burn:\n");  */
/*   if (conf->collections == 0) goto error2; */
/*   rgRewind(conf->collections); */
/*   while ((coll = rgNext(conf->collections))) { */
/*     if (!computeExtractScore(coll)) goto error2; */
/*     if (!(text = getExtractStatus(coll, &badSize, 0)))  */
/*       goto error2; */

/*     if (badSize == 0) goto next; */
/*     sprintSize(buf, badSize); */
/*     printf("- %s (%5.2f): %s", coll->label, coll->extractTree->score, buf); */

/*   next: */
/*     text = destroyString(text); */
/*   } */

/*   printf("\n"); */
/*   rc = TRUE; */
/*  error2: */
/*   if (!loadConfiguration(CFG|SUPP)) rc = FALSE; */
/*  error: */
/*   if (!rc) { */
/*     logMain(LOG_ERR, "fails to update motd"); */
/*   } */
/*   tree1 = destroyOnlyRing(tree1); */
/*   tree2 = destroyOnlyRing(tree2); */
/*   return rc; */
/* } */

//////////////////////////////////////////////////////


typedef struct MotdSupport {

  Support* support;
  int ask4periodicCheck;
  RG* collections;

} MotdSupport;

typedef struct Motd {
  
  RG* motdSupports; // MotdSupport*
  
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
 * Synopsis   : void destroyMotdSupport(Motd* self)
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
  logMain(LOG_ERR, "malloc: cannot create MotdSuuport");
  return destroyMotdSupport(rc);
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

  // delete motdSupports association
  self->motdSupports
    = destroyRing(self->motdSupports,
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
  if ((rc->motdSupports = createRing()) == 0) goto error;

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
   
  logMain(LOG_DEBUG, "addMotdSupport %s form %s", support->name, coll->label);
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
 * Input      : N/A
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

#warning TODO: check support files here

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
  data->isAvailable = TRUE; // is this needed ?

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
 * Description: List all requested supports
 * Synopsis   : SupportTree* updateMotdFromMd5sumsDB(
 * Input      : 
 * Output     : TRUE on success
 =======================================================================*/
int
updateMotdFromMd5sumsDB(Motd* motd, Collection* coll)
{
  int rc = FALSE;
  Archive* archive = 0;
  Support* support = 0;
  RGIT* curr = 0;
  MotdSearch data;

  logMain(LOG_DEBUG, "updateMotdFromMd5sumsDB for %s collection",
	  coll->label);
  memset(&data, 0, sizeof(MotdSearch));

  if (!loadCollection(coll, SERV|EXTR|CACH)) goto error;
  if (!(data.imagesWanted = createRing())) goto error2;
  data.coll = coll;

  // for each cache entry
  while ((archive = rgNext_r(coll->cacheTree->archives, &curr)) 
	!= 0) {

    // scan wanted archives not available
    if (archive->state != WANTED) continue;
    if (!motdArchive(&data, archive)) goto error2;
  }

  // for each archive (that all are images), match supports
  rgRewind(data.imagesWanted);
  while ((archive = rgNext(data.imagesWanted))) {

    // match support shared with that collection
    rgRewind(coll->supports);
    while ((support = rgNext(coll->supports))) {
      if (!strncmp(support->fullHash, archive->hash, MAX_SIZE_HASH) &&
	  support->size == archive->size) {
	
	// add a motd notification on support
	if (!addMotdSupport(motd, support, coll)) goto error2;
      }
    }

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
    
    // match records to local images 
    if (!updateMotdFromMd5sumsDB(motd, coll)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "updateMotdFromAllMd5sumsDB fails");
  }
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
  Motd* motd = 0;
  MotdSupport* motdSupp = 0;
  Collection* coll = 0;
  char* text = 0;
  off_t badSize = 0;
  char buf[30];
  char car = ':';

  logMain(LOG_DEBUG, "updateMotd");

  if (!allowedUser(env.confLabel)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!loadConfiguration(CFG|SUPP)) goto error;
  if (!expandConfiguration()) goto error2;

  if (!(motd = createMotd())) goto error;
  
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

  if (!rgSort(motd->motdSupports, cmpMotdSupport)) goto error2;
  printf("Need to check your local supports:\n");
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
 
  // display collections global status 
  printf("Looking for content to burn:\n"); 
  if (conf->collections == 0) goto error2;
  rgRewind(conf->collections);
  while ((coll = rgNext(conf->collections))) {
    if (!computeExtractScore(coll)) goto error2;
    if (!(text = getExtractStatus(coll, &badSize, 0))) 
      goto error2;

    if (badSize == 0) goto next;
    sprintSize(buf, badSize);
    printf("- %s (%5.2f): %s", coll->label, coll->extractTree->score, buf);

  next:
    text = destroyString(text);
  }

  printf("\n");
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
