/*=======================================================================
 * Project: MediaTeX
 * Module : upgrade
 *
 * Manage servers.txt upgrade from mediatex.conf and supports.txt

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche
 
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
 * Function   : doCheckSupport 
 * Description: Do checksums on an available support
 * Synopsis   : int soCheckSupport(Support *supp, char* path)
 * Input      : Support *supp = the support object
 *              char* path = the device that host the support
 * Output     : TRUE on success
 *
 * Note       : supp->lastCheck: (O will force check)
 *              also call by supp module
 =======================================================================*/
int 
doCheckSupport(Support *supp, char* path)
{
  int rc = FALSE;
  Configuration* conf = 0;
  time_t now = 0;
  time_t laps = 0;
  time_t ttl = 0;
  CheckData data;

  logMain(LOG_DEBUG, "doCheckSupport");
  if (!(conf = getConfiguration())) goto error;
  checkSupport(supp);
  memset(&data, 0, sizeof(CheckData));
  
  if ((data.path = createString(path)) == 0) {
    logMain(LOG_ERR, "cannot dupplicate path string");
    goto error;
  }
  
  // current date
  if ((now = currentTime()) == -1) goto error;
 
  // by default do not exists: full computation (no check)
  data.opp = CHECK_SUPP_ADD;

  // check if support need to be full checked or not
  if (supp->lastCheck > 0) {

    // exists: quick check
    data.opp = CHECK_SUPP_ID;

    // but maybe need to be re-checked fully
    ttl = (isSupportFile(supp))?conf->fileTTL:conf->checkTTL;
    laps = now - supp->lastCheck;
    if (laps > ttl/2) {

      // soon obsolete: full check
      data.opp = CHECK_SUPP_CHECK;
      logMain(LOG_NOTICE, "support no checked since %d days: checking...", 
	      laps/60/60/24);
    }
  }
  
  // copy size and current checksums to compare them
  switch (data.opp) {
  case CHECK_SUPP_CHECK:
    strncpy(data.fullMd5sum, supp->fullMd5sum, MAX_SIZE_MD5);
    strncpy(data.fullShasum, supp->fullShasum, MAX_SIZE_SHA);
  case CHECK_SUPP_ID:
    data.size = supp->size;
    strncpy(data.quickMd5sum, supp->quickMd5sum, MAX_SIZE_MD5);
    strncpy(data.quickShasum, supp->quickShasum, MAX_SIZE_SHA);
  default:
    break;
  }

  // checksum computation
  rc = TRUE;
  if (!doChecksum(&data)) {
      logMain(LOG_DEBUG, 
	      "internal error on md5sum computation for \"%s\" support", 
	      supp->name);
      rc = FALSE;
      goto error;
  }

  if (data.rc == CHECK_FALSE_SIZE) {
    logMain(LOG_WARNING, "wrong size on \"%s\" support", supp->name);
    rc = FALSE;
  }
  if (data.rc == CHECK_FALSE_QUICK) {
    logMain(LOG_WARNING, "wrong quick hash on \"%s\" support", supp->name);
    rc = FALSE;
  }
  if (data.rc == CHECK_FALSE_FULL) {
    logMain(LOG_WARNING, "wrong full hash on \"%s\" support", supp->name);
    rc = FALSE;
  }

  if (!rc) {
    logMain(LOG_WARNING, "please manualy check \"%s\" support", supp->name);
    if (!isSupportFile(supp)) {
      logMain(LOG_WARNING, "either this is not \"%s\" support at %s, or", 
	      supp->name, path);
    }
    logMain(LOG_WARNING, "maybe the \"%s\" support is obsolete", 
	    supp->name);
    goto error;
  }

  // store results
  switch (data.opp) {
  case  CHECK_SUPP_ADD:
    supp->size = data.size;
    strncpy(supp->quickMd5sum, data.quickMd5sum, MAX_SIZE_MD5);
    strncpy(supp->fullMd5sum, data.fullMd5sum, MAX_SIZE_MD5);
    strncpy(supp->quickShasum, data.quickShasum, MAX_SIZE_SHA);
    strncpy(supp->fullShasum, data.fullShasum, MAX_SIZE_SHA);
  case CHECK_SUPP_CHECK:
    if (env.noRegression)
      supp->lastCheck = currentTime() + 1*DAY;
    else
      supp->lastCheck = now;
  case CHECK_SUPP_ID:
    if (env.noRegression)
      supp->lastSeen = currentTime() + 1*DAY;
    else
      supp->lastSeen = now;
  default:
    break;
  }

  conf->fileState[iSUPP] = MODIFIED;
 error:
  free(data.path);
  return rc;
}

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
  time_t ttl = 0;
  int rc = FALSE;

  checkSupport(supp);
  logCommon(LOG_INFO, "compute support score for %s:%lli (%s)", 
	  supp->fullMd5sum, supp->size, supp->name);

  if (!(conf = getConfiguration())) goto error;
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
  ttl = (isSupportFile(supp))?conf->fileTTL:conf->checkTTL;
  laps = now - supp->lastCheck;

  // static score for support file
  if (isSupportFile(supp)) {

    // check support file before they become obsolete
    if (laps > ttl/2) {
      if (!doCheckSupport(supp, supp->name)) {
	logMain(LOG_WARNING, "\"%s\" support file seems lost", supp->name);
	// score = 0: fails to check support file on disk
	goto end;
      }
    }
      
    supp->score = p->fileScore;
    logCommon(LOG_INFO, "file: = %.2f", supp->score);
    goto end;
  }
  
  if (laps > ttl) {
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
 * Function   : scoreLocalImages
 * Description: Compute ISO score on local server
 * Synopsis   : int scoreLocalImages(Collection* coll)
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
  int nbSupp = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "scores for %s collection", coll->label);

  if (!loadConfiguration(SUPP)) goto error;
  if ((images = createRing()) == 0)  goto error;
  serverTree = coll->serverTree;

  // for each support names:
  curr = 0;
  while ((supp = rgNext_r(coll->supports, &curr))) {
 
    // compute score on support (using collection's parameters)
    if (!scoreSupport(supp, &serverTree->scoreParam)) goto error;

    // add supports to temporary ring
    if (!(archive = addArchive(coll, supp->fullMd5sum, supp->size)))
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
 * Function   : upgradeCollection
 * Description: Upgrade servers.txt
 * Synopsis   : int upgradeCollection(Collection* coll)
 * Input      : Collection* coll = collection to upgrade              
 * Output     : TRUE on success 

 * Note       : Must only be call by client (nor server or cgi).
 *              And should only be called by  openClose.c::loadColl().
 *
 *              Do almost all the consistency job:
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
  char url[256];

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
  //localhost->wwwPort = conf->wwwPort;
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

  // update lastCommit
  if (!(localhost->lastCommit = currentTime())) goto error;

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
    coll->serverTree->master = localhost;
  }

  // else we trust the servers.txt file
  if (!(master = coll->serverTree->master)) {
    logCommon(LOG_ERR, "loose master server for %s collection", 
	    coll->label);
    goto error;
  }

  // uprgade www port on servers.txt
  localhost->wwwPort =
    coll->serverTree->doHttps ? conf->httpsPort : conf->httpPort;
  
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

  // upgrade GIT settings
  if (sprintf(url, "ssh://%s@%s:/var/lib/gitbare/%s",
	      coll->serverTree->master->user,
	      coll->serverTree->master->host,
	      coll->serverTree->master->user) <= 0) goto error;
  if (!callUpgrade(coll->user, coll->userFingerPrint, url)) goto error;
  
  rc = TRUE;
error:
  if (!rc) {
    logCommon(LOG_ERR, "upgradeCollection fails");
    // note: not a good idea to destroy localhost server on error,
    // as it may be used later
  }
  return rc;
}

/*=======================================================================
 * Function   : computeUrls
 * Description: Update all server's url
 * Synopsis   : int computeUrls(Collection* coll)
 * Input      : Collection* coll = collection to update              
 * Output     : TRUE on success 
 * Note       : this is done out from upgrade() as cgi and server only
 *              read servers.txt but do not upgrade
 =======================================================================*/
int 
computeUrls(Collection* coll)
{
  int rc = FALSE;
  ServerTree* self = 0;
  Server* server = 0;
  RGIT* curr = 0;
  char url[512];

  checkCollection(coll);
  logCommon(LOG_DEBUG, "computeUrls %s", coll->label);

  self = coll->serverTree;
  
  while ((server = rgNext_r(self->servers, &curr))) {
    
    // add port into the url if not the default one
    if ((self->doHttps && server->wwwPort == 443) ||
	(!self->doHttps && server->wwwPort == 80)) {
      if (sprintf(url, "http%s://%s/~%s",
		  self->doHttps?"s":"",
		  server->host, server->user) <= 0) goto error;
    }
    else {
      if (sprintf(url, "http%s://%s:%i/~%s",
		  self->doHttps?"s":"",
		  server->host, server->wwwPort, server->user) <= 0)
	goto error;
      
    }
    if (!(server->url = createString(url))) goto error;
  }

  // do the same for dns entry
  //  (all hosts must use the same ports and mdtx's label)
  if (*self->dnsHost) {

    // add port into the url if not the default one
    if ((self->doHttps && self->master->wwwPort == 443) ||
	(!self->doHttps && self->master->wwwPort == 80)) {
      if (sprintf(url, "http%s://%s/~%s",
		  self->doHttps?"s":"",
		  self->dnsHost, self->master->user) <= 0) goto error;
    }
    else {
      if (sprintf(url, "http%s://%s:%i/~%s",
		  self->doHttps?"s":"", self->dnsHost,
		  self->master->wwwPort, self->master->user) <= 0)
	goto error;
      
    }
    if (!(self->dnsUrl = createString(url))) goto error;
  }
  
  rc = TRUE;
error:
  if (!rc) {
    logCommon(LOG_ERR, "computeUrls fails");
  }
  return rc;
}


/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
