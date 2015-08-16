/*=======================================================================
 * Version: $Id: upgrade.c,v 1.10 2015/08/16 20:35:09 nroche Exp $
 * Project: MediaTeX
 * Module : upgrade
 *
 * Manage servers.txt upgrade from mediatex.conf and supports.txt

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
#include <math.h>      // pow
#include <dirent.h>    // scandir

/*=======================================================================
 * Function   : scoreSupport
 * Description: Look for time life of a support
 * Synopsis   : int scoreSupport(Support* supp, ScoreParam *p)
 * Input      : Support* supp = the support to check
 *              ScoreParam *p = parameters
 * Output     : TRUE on success
 =======================================================================*/
int 
scoreSupport(Support* supp, ScoreParam *p)
{
  Configuration* conf = 0;
  time_t now = 0;
  time_t laps = 0;
  int rc = FALSE;

  checkSupport(supp);
  logCommon(LOG_INFO, "compute support score for %s:%lli (%s)", 
	  supp->fullHash, supp->size, supp->name);

  if (!(conf = getConfiguration())) goto error;

  // date
  if ((now = currentTime()) == -1) goto error;
  supp->score = 0; // obsolete: need to be checked

  // check data consistancy
  if (supp->firstSeen > now) {
    logCommon(LOG_ERR, "support firstSeen date is in the futur");
    goto error;
  }

  if (supp->firstSeen > supp->lastCheck) {
    logCommon(LOG_ERR, "support lastCheck > firstSeen");
    goto error;
  }

  if (supp->lastCheck > supp->lastSeen) {
    logCommon(LOG_ERR, "support lastCheck > lastSeen");
    goto error;
  }

  // check support validity
  laps = now - supp->lastCheck;
  if (conf->checkTTL < 0 ||
      (conf->checkTTL >= 0 &&
       laps > conf->checkTTL)) {
    logCommon(LOG_WARNING, "\"%s\" support have expired since %d days",
    	    supp->name, laps/(60*60*24));
    // score = 0: unchecked support for too many time (may be broken)
    goto end;
  }

  // compute a support score base on its age
  laps = now - supp->firstSeen;

  // score > badScore: active support
  if (laps <= p->suppTTL) {
    supp->score = 
      powf ( ((p->suppTTL - (float)laps) / p->suppTTL), 1 / p->powSupp ) 
      * (p->maxScore - p->badScore) + p->badScore;
    logCommon(LOG_INFO, 
	    "laps < suppTTL: pow ( %.6f, 1/%.2f ) * %.2f + %.2f = %.2f", 
	    ((p->suppTTL - (float)laps) / p->suppTTL),
	    p->powSupp,
	    p->maxScore - p->badScore,
	    p->badScore,
	    supp->score);
  }
  // score < badScore: retire support (may be replaced but still there)
  else {
    supp->score = 
      (1 / (1 + p->factSupp * (((float)laps - p->suppTTL) / p->suppTTL)) )
      * p->badScore; 
    logCommon(LOG_INFO, 
	    "laps > suppTTL: (1 / (1 + %.2f * (%.2f)) ) * %.2f = %.2f",
	    p->factSupp, ((float)laps - p->suppTTL) / p->suppTTL, 
	    p->badScore, supp->score);
  }

 end:
  rc = TRUE;
 error:
  if (!rc && supp) {
    logCommon(LOG_ERR, "scoreSupport fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : scoreLocaleImages
 * Description: Compute ISO score on local server
 * Synopsis   : int scoreLocaleImages(Collection* coll)
 * Input      : Collection* coll = collection to upgrade
 *              Server* server = the local server
 * Output     : TRUE on success 
 =======================================================================*/
int 
scoreLocalImages(Collection* coll)
{
  int rc = FALSE;
  ServerTree* serverTree = 0;
  Support* supp = 0;
  Archive* archive = 0;
  RG* images = 0; // temporary objects
  Image* image1 = 0;
  Image* image2 = 0;
  Image* image3 = 0;
  //char* label = 0;
  int nbSupp = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "scores for %s collection", coll->label);

#ifndef utMAIN
  if (!loadConfiguration(SUPP)) goto error;
#endif

  if ((images = createRing()) == 0)  goto error;
  serverTree = coll->serverTree;

  // for each support names:
  rgRewind(coll->supports);
  while((supp = rgNext(coll->supports))) {
 
    // compute score on support (using collection's parameters)
    if (!scoreSupport(supp, &serverTree->scoreParam)) goto error;

    // add supports to temporary ring
    if (!(archive = addArchive(coll, supp->fullHash, supp->size)))
      goto error;
    if ((image1 = createImage()) == 0) goto error;
    image1->archive = archive;
    image1->score = supp->score;
    if (!rgInsert(images, image1)) goto error;
  }

  // sort supports on the ring by scores and grouped by images
  if (!rgSort(images, cmpImageScore)) goto error;

   // for each group of support names:
  rgRewind(images);
  image1 = image2 = rgNext(images);
  while (image1) {
    logCommon(LOG_INFO, "compute image score for %s:%lli", 
	    image1->archive->hash, image1->archive->size);
    nbSupp = 0;

    // while supports match the same image
    while (image2 && 
	   image1->archive == image2->archive) {
      
      // if not the first image, incremment the final score
      if (++nbSupp > 1) image1->score += image2->score;
      logCommon(LOG_INFO, "%c %5.2f", 
	      (nbSupp > 1)?'+':' ', image2->score);

      image2 = rgNext(images);
    }

    // truncate it if more than maxScore
    if (image1->score > serverTree->scoreParam.maxScore) {
      image1->score = serverTree->scoreParam.maxScore;
      logCommon(LOG_INFO, "> %5.2f", coll->serverTree->scoreParam.maxScore);
    }

    logCommon(LOG_INFO, "-------", archive->imageScore);
    logCommon(LOG_INFO, "= %5.2f", image1->score);

    // finaly, add image1 to local server's images ring
    if (!(image3 = addImage(coll, coll->localhost, image1->archive))) 
      goto error;
    image3->score = image1->score;

    // next group
    image1 = image2;
  }

  rc = TRUE;
 error:
  images = destroyRing(images, (void*(*)(void*)) destroyImage);
  if (!rc) {
    logCommon(LOG_ERR, "scoreLocalImages fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : serializeCvsRootClientFiles
 * Description: Serialize the CVS/Root files.
 * Synopsis   : static int serializeCvsRootClientFiles(Collection* coll)
 * Input      : Collection* coll = what to serialize
 *              char* dirpath = directory whe is the Root file
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeCvsRootClientFile(Collection* coll, char* dirpath)
{ 
  int rc = FALSE;
  char* path = 0;
  FILE* fd = stdout; 
  ServerTree* self = 0;
  mode_t mask;

  mask = umask(0117);
  checkCollection(coll);
  if (!(self = coll->serverTree)) goto error;
  checkServer(self->master);
  logCommon(LOG_DEBUG, "serialize CVS/Root files");

  if (!(path = createString(dirpath)) ||
      !(path = catString(path, "/")) ||
      !(path = catString(path, "Root"))) goto error;
      
  // output file
  if (!env.dryRun) {
    if ((fd = fopen(path, "w")) == 0) {
      logCommon(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
      goto error;
    }
  }
      
  fprintf(fd, ":ext:%s@%s:/var/lib/cvsroot\n",
	  self->master->user, self->master->host);
  fprintf(fd, "# This file is managed by MediaTeX software.\n");

  fflush(fd);
  rc = TRUE;  
 error:  
  if (fd != stdout && fclose(fd)) {
    logCommon(LOG_ERR, "fclose fails: %s", strerror(errno));
    rc = FALSE;
  }
  if (!rc) {
    logCommon(LOG_ERR, "serializeCvsRootClientFile fails");
  }
  umask(mask);
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : scanCvsClientDirectory
 * Description: Serialize the CVS/Root files.
 * Synopsis   : static int scanCvsClientDirectory(Collection* coll,
 *                                                char* path)
 * Input      : Collection* coll = context
 *              char* path = directory to scan
 * Output     : TRUE on success
 =======================================================================*/
int 
scanCvsClientDirectory(Collection* coll, char* path) 
{
  int rc = FALSE;
  ServerTree* self = 0;
  struct dirent** entries;
  struct dirent* entry;
  int nbEntries = 0;
  int n = 0;
  char* subdir = 0;

  if (!(self = coll->serverTree)) goto error;
  checkServer(self->master);
  logCommon(LOG_DEBUG, "scaning CVS directory: %s", path);

  entries = 0;
  if ((nbEntries 
       = scandir(path, &entries, 0, alphasort)) == -1) {
    logCommon(LOG_ERR, "scandir fails on %s: %s", path, strerror(errno));
    goto error;
  }

  for (n=0; n<nbEntries; ++n) {
    entry = entries[n];
    if (!strcmp(entry->d_name, ".")) continue;
    if (!strcmp(entry->d_name, "..")) continue;
    
    switch (entry->d_type) {

    case DT_DIR: 
      if (!(subdir = createString(path)) ||
	  !(subdir = catString(subdir, "/")) ||
	  !(subdir = catString(subdir, entry->d_name)))	goto error;

      if (!strcmp(entry->d_name, "CVS")) {
	if (!serializeCvsRootClientFile(coll, subdir)) goto error;
      }
      else {
	if (!scanCvsClientDirectory(coll, subdir)) goto error;
      }

      default: 
	break;
      }   
      
    subdir = destroyString(subdir);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "scanCvsClientDirectory fails");
  }
  if (entries) {
    for (n=0; n<nbEntries; ++n) {
      remind(entries[n]);
      free(entries[n]);
    }
    remind(entries);
    free(entries);
  }
  return rc;
}

/*=======================================================================
 * Function   : upgradeCollection
 * Description: Upgrade servers.txt
 * Synopsis   : int upgradeCollection(Collection* coll)
 * Input      : Collection* coll = collection to upgrade              
 * Output     : TRUE on success 

 * Note       : Should not be called by the server (to save memory).
 *              This function do almost all the consistency job:
 *              - re-compute the local image's score
 *              - update own configuration values into servers.txt
 *              - update ssh configuration files (config, key and auth)
 *              - update master collection host values into:
 *                - configuration file (mdtx.conf)
 *                - CVS/Root files
 =======================================================================*/
int 
upgradeCollection(Collection* coll)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Server* localhost = 0;
  Server* master = 0;
  char* string = 0;
  RG* ring = 0;
  RGIT* curr = 0;
  int isMaster = FALSE;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "upgradeCollection %s", coll->label);
  if (!(conf = getConfiguration())) goto error;
  if (!populateConfiguration()) goto error;

  // get the host and user keys and update the fingerprints
  if (!populateCollection(coll)) goto error;

  // delete and rebuild own server entry (update own values)
  // note: server.localhost := conf
  if (!(localhost = getLocalHost(coll))) goto error; 
  if (!diseaseServer(coll, localhost)) goto error;

  strncpy(localhost->host, conf->host, MAX_SIZE_HOST);
  localhost->mdtxPort = conf->mdtxPort;
  localhost->sshPort = conf->sshPort;
  localhost->wwwPort = conf->wwwPort;
  if (!(localhost->comment = createString(conf->comment))) goto error;
  if (!(localhost->userKey = createString(coll->userKey))) goto error;
  if (!(localhost->hostKey = createString(conf->hostKey))) goto error;
  localhost->cacheSize = coll->cacheSize?coll->cacheSize:0;
  localhost->cacheTTL = coll->cacheTTL?coll->cacheTTL:0;
  localhost->queryTTL = coll->queryTTL?coll->queryTTL:0;
  if (!(localhost->label = createString(env.confLabel))) goto error;
  if (!(localhost->user = createString(env.confLabel))
      || !(localhost->user = catString(localhost->user, "-"))
      || !(localhost->user = catString(localhost->user, coll->label)))
    goto error;

  // get networks and gateway from configuration
  ring = coll->networks;
  if (isEmptyRing(ring)) ring = conf->networks;
  curr = 0;
  while ((string = rgNext_r(ring, &curr))) {
    if (!rgInsert(localhost->networks, string)) goto error;
  }
  ring = coll->gateways;
  if (isEmptyRing(ring)) ring = conf->gateways;
  curr = 0;
  while ((string = rgNext_r(ring, &curr))) {
    if (!rgInsert(localhost->gateways, string)) goto error;
  }

  coll->localhost = localhost;
  localhost = 0;

  // add local images to serverTree
  if (!scoreLocalImages(coll)) goto error;

  // check if we are the collection master host
  isMaster = 
    (!strcmp(coll->masterLabel, env.confLabel) &&
     (!strcmp(coll->masterHost, "localhost") ||
      !strcmp(coll->masterHost, conf->host)));
  
  // if master change his collection key, we need
  // to look first to the master prefix into mdtx.conf file
  if (isMaster) {
    coll->serverTree->master = coll->localhost;
  }

  // else we trust the servers.txt file
  if (!(master = coll->serverTree->master)) {
    logCommon(LOG_ERR, "loose master server for %s collection", 
	    coll->label);
    goto error;
  }

  // upgrade master ids on collection stanza into configuration
  // note: this is also valuable for master server
  // conf.coll := server.master
  if (strncmp(coll->masterHost, master->host, MAX_SIZE_HOST)) {
    strncpy(coll->masterHost, master->host, MAX_SIZE_HOST);
    coll->masterHost[MAX_SIZE_HOST] = 0;
    conf->fileState[iCFG] = MODIFIED;
  }
  if (strcmp(coll->masterLabel, master->label)) {
    destroyString(coll->masterLabel);
    if (!(coll->masterLabel = createString(master->label))) goto error;
    conf->fileState[iCFG] = MODIFIED;
  }
  if (strcmp(coll->masterUser, master->user)) {
    destroyString(coll->masterUser);
    if (!(coll->masterUser = createString(master->user))) goto error;
    conf->fileState[iCFG] = MODIFIED;
  }
  if (coll->masterPort != master->sshPort) {
    coll->masterPort = master->sshPort;
    conf->fileState[iCFG] = MODIFIED;
  } 

  // upgrade SSH settings (using servers)
  if (!env.noRegression && !upgradeSshConfiguration(coll)) goto error;

  // upgrade CVS/Root files (using conf.coll)
  if (!env.noRegression && !scanCvsClientDirectory(coll, coll->cvsDir)) 
    goto error;

  rc = TRUE;
error:
  if (!rc) {
    logCommon(LOG_ERR, "upgradeCollection fails");
    // note: not a good idea to destroy localhost server on error,
    // as it may be used later
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
