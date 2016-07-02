/*=======================================================================
 * Project: mediaTeX
 * Module : configuration
 *
 * configuration producer interface

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

/*=======================================================================
 * Function   : cmpCollection
 * Description: Compare 2 collections
 * Synopsis   : int cmpCollection(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2
 * Output     : like strncmp
 =======================================================================*/
int cmpCollection(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Archive* 
   */
  
  Collection* a1 = *((Collection**)p1);
  Collection* a2 = *((Collection**)p2);

  rc = strncmp(a1->label, a2->label, MAX_SIZE_COLL);
  return rc;
}


/*=======================================================================
 * Function   : createCollection
 * Description: Create, by memory allocation a Collection projection.
 * Synopsis   : Collection* createCollection(void)
 * Input      : N/A
 * Output     : The address of the create empty collection.
 =======================================================================*/
Collection* 
createCollection(void)
{
  Collection* rc = 0;
  int i = 0;
  int err = 0;

  if((rc = (Collection*)malloc(sizeof(Collection))) == 0) {
    goto error;
  }
    
  memset(rc, 0, sizeof(Collection));

  rc->masterPort = SSH_PORT;
  rc->toUpdate = TRUE;

  /* computed paths will be filled by expandCollection */

  if (!(rc->archives = 
	avl_alloc_tree(cmpArchiveAvl, (avl_freeitem_t)destroyArchive)))
    goto error;

  if ((rc->networks = createRing()) == 0) goto error;
  if ((rc->gateways = createRing()) == 0) goto error;
  if ((rc->supports = createRing()) == 0) goto error;

  // init the locks
  for (i=iCTLG; i<iCACH; ++i) {
    if ((err = pthread_mutex_init(&rc->mutex[i], (pthread_mutexattr_t*)0))
	!= 0) {
      logMemory(LOG_INFO, "pthread_mutex_init: %s", strerror(err));
      goto error;
    }
  }

  return rc;
 error:
  logMemory(LOG_ERR, "malloc: cannot create Collection");
  return destroyCollection(rc);
}

/*=======================================================================
 * Function   : destroyCollection
 * Description: Destroy a Collection by freeing all the allocate memory.
 * Synopsis   : void destroyCollection(Collection* self)
 * Input      : Collection* self = the address of the Collection to
 *              destroy.
 * Output     : Nil address of a Collection.
 =======================================================================*/
Collection* 
destroyCollection(Collection* self)
{
  Collection* rc = 0;
  int i = 0;
  int err = 0;

  if(self == 0) goto error;

  self->masterLabel = destroyString(self->masterLabel);
  self->masterUser = destroyString(self->masterUser);
  self->user = destroyString(self->user);

  // directories
  self->homeDir = destroyString(self->homeDir);
  self->gitDir = destroyString(self->gitDir);
  self->sshDir = destroyString(self->sshDir);
  self->cacheDir = destroyString(self->cacheDir);
  self->extractDir = destroyString(self->extractDir);
  self->htmlDir = destroyString(self->htmlDir);
  self->htmlIndexDir = destroyString(self->htmlIndexDir);
  self->htmlCacheDir = destroyString(self->htmlCacheDir);
  self->htmlScoreDir = destroyString(self->htmlScoreDir);
  self->htmlCgiDir = destroyString(self->htmlCgiDir);

  // files
  self->catalogDB = destroyString(self->catalogDB);
  self->serversDB = destroyString(self->serversDB);
  self->extractDB = destroyString(self->extractDB);
  self->md5sumsDB = destroyString(self->md5sumsDB);
  
  self->sshAuthKeys = destroyString(self->sshAuthKeys);
  self->sshConfig = destroyString(self->sshConfig);
  self->sshKnownHosts = destroyString(self->sshKnownHosts);
  self->sshRsaPublicKey = destroyString(self->sshRsaPublicKey);
  self->sshDsaPublicKey = destroyString(self->sshDsaPublicKey);
  
  self->cgiUrl = destroyString(self->cgiUrl);
  self->cacheUrl = destroyString(self->cacheUrl);
  self->userKey = destroyString(self->userKey);
  
  avl_free_tree(self->archives);

  // do not free the supports objects (owned by conf), just the rings
  self->networks = destroyOnlyRing(self->networks);
  self->gateways = destroyOnlyRing(self->gateways);
  self->supports = destroyOnlyRing(self->supports);
  
  self->extractTree = destroyExtractTree(self->extractTree);
  self->catalogTree = destroyCatalogTree(self->catalogTree);
  self->serverTree = destroyServerTree(self->serverTree);
  self->cacheTree = destroyCacheTree(self->cacheTree);
  
  // free the locks
  for (i=iCTLG; i<iCACH; ++i) {
    if ((err = pthread_mutex_destroy(&self->mutex[i]))) {
      logMemory(LOG_INFO, "pthread_mutex_destroy[%i]: %s", i, 
		strerror(err));
      goto error;
    }
  }

  free(self);
 error:
  return(rc);
}

/*=======================================================================
 * Function   : expandCollection
 * Description: Check if all fields are initialized and complete them
 * Synopsis   : int expandCollection(Collection* self, 
 *                                     Configuration* self)
 * Input      : Collection* self (label + masterHost)
 * Output     : True on success
 =======================================================================*/
int 
expandCollection(Collection* self)
{
  int rc = FALSE;
  Configuration* conf = 0;
  int i,j;
  char* dataV[] = {CONF_CATHFILE, CONF_SERVFILE, CONF_EXTRFILE, "/cgi"};
  char** dataP[] = {
    &self->catalogDB, &self->serversDB, &self->extractDB};
  char* htmlV[] = {"/index", "/cache", "/score", "/cgi"};
  char** htmlP[] = {
    &self->htmlIndexDir, &self->htmlCacheDir, 
    &self->htmlScoreDir, &self->htmlCgiDir};
  char* label = 0;

  // defensive code
  checkCollection(self);
  if (isEmptyString(self->masterLabel) ||
      isEmptyString(self->label) ||
      isEmptyString(self->masterHost)) {
    logMemory(LOG_ERR, 
	    "masterLabel, label and masterHost must be initialized");
    goto error;
  }

  if (self->memoryState & EXPANDED) {
    logMemory(LOG_DEBUG, "%s collection already expanded", self->label);
    goto end;
  }

  logMemory(LOG_DEBUG, "expand the %s collection", self->label);
  if (!(conf = getConfiguration())) goto error;

  // replace localhost with hostname on master server
  if (!strncmp(self->masterHost, "localhost", MAX_SIZE_HOST)) {
    strncpy(self->masterHost, conf->host, MAX_SIZE_HOST);
  }

  // master user
  if (!(self->masterUser = createString(self->masterLabel))
      || !(self->masterUser = catString(self->masterUser, "-"))
      || !(self->masterUser = catString(self->masterUser, self->label)))
    goto error;
  
  // collection user
  if (!(self->user = createString(env.confLabel)) 
      || !(self->user = catString(self->user, "-"))
      || !(self->user = catString(self->user, self->label))) 
    goto error;

  // directories
  if (!(label = createString("/"))
      || !(label = catString(label, self->user))
      || !(self->cacheDir = createString(conf->cacheDir)) 
      || !(self->cacheDir =  catString(self->cacheDir, label))
      || !(self->extractDir = createString(conf->extractDir)) 
      || !(self->extractDir = catString(self->extractDir, label))
      || !(self->gitDir = createString(conf->gitDir)) 
      || !(self->gitDir = catString(self->gitDir, label))
      || !(self->homeDir = createString(conf->homeDir))
      || !(self->homeDir =  catString(self->homeDir, CONF_HOME))
      || !(self->homeDir =  catString(self->homeDir, label))
      || !(self->sshDir = createString(self->homeDir)) 
      || !(self->sshDir = catString(self->sshDir, CONF_SSHDIR))
      || !(self->htmlDir = createString(self->homeDir)) 
      || !(self->htmlDir = catString(self->htmlDir, CONF_HTMLDIR))
      )
    goto error;
  
  // html directories
  for (i=0; i<4; ++i) {
    if (!(*htmlP[i] = createString(self->htmlDir)) 
	|| !(*htmlP[i] = catString(*htmlP[i], htmlV[i])))
      goto error;
  }
  
  // md5sum file
  if (!(self->md5sumsDB = createString(conf->md5sumDir)) 
      || !(self->md5sumsDB =  catString(self->md5sumsDB, "/"))
      || !(self->md5sumsDB =  catString(self->md5sumsDB, self->user))
      || !(self->md5sumsDB =  catString(self->md5sumsDB, ".md5")))
    goto error;

  // metadata files
  for (j=0; j<3; ++j) {
    if (!(*dataP[j] = createString(self->gitDir)) 
	|| !(*dataP[j] = catString(*dataP[j], dataV[j]))) 
      goto error;
  }
  if (!(*dataP[1] = catString(*dataP[1], ".txt"))) goto error;

  // key files
  if (!(self->sshAuthKeys = createString(self->sshDir)) 
      || !(self->sshAuthKeys =  
	   catString(self->sshAuthKeys, CONF_SSHAUTH)))
    goto error;
  
  if (!(self->sshConfig = createString(self->sshDir)) 
      || !(self->sshConfig =  
	   catString(self->sshConfig, CONF_SSHCONF)))
    goto error;

  if (!(self->sshKnownHosts = createString(self->sshDir)) 
      || !(self->sshKnownHosts = 
	   catString(self->sshKnownHosts, CONF_SSHKNOWN)))
    goto error;

  if (!(self->sshRsaPublicKey = createString(self->sshDir)) 
      || !(self->sshRsaPublicKey = 
	   catString(self->sshRsaPublicKey, CONF_RSAUSERKEY)))
    goto error;

  if (!(self->sshDsaPublicKey = createString(self->sshDir)) 
      || !(self->sshDsaPublicKey = 
	   catString(self->sshDsaPublicKey, CONF_DSAUSERKEY)))
    goto error;

  // default values if not set
  if (!self->cacheSize) self->cacheSize = conf->cacheSize;
  if (!self->cacheTTL) self->cacheTTL = conf->cacheTTL;
  if (!self->queryTTL) self->queryTTL = conf->queryTTL;

  // object trees 
  if (!(self->serverTree = createServerTree())) goto error;
  if (!(self->extractTree = createExtractTree())) goto error;
  if (!(self->catalogTree = createCatalogTree())) goto error;
  if (!(self->cacheTree = createCacheTree())) goto error;

  // init objects default values
  if ((self->cacheTree->totalSize = self->cacheSize) == 0) {
    self->cacheTree->totalSize = conf->cacheSize;
  }
  self->cacheTree->recordTree->collection = self;

  // check directories
  if (!checkDirectoryPerm(self->user, self->homeDir, 
			  VAR_CACHE_M_MDTX_HOME_COLL)
      || !checkDirectoryPerm(self->user, self->cacheDir,
			     VAR_CACHE_M_MDTX_CACHE_COLL)
      || !checkDirectoryPerm(self->user, self->extractDir,
			     VAR_CACHE_M_MDTX_TMP_COLL)
      || !checkDirectoryPerm(self->user, self->gitDir, 
			     VAR_CACHE_M_MDTX_GIT_COLL)
      || !checkDirectoryPerm(self->user, self->sshDir, 
			     VAR_CACHE_M_MDTX_HOME_COLL_SSH)
      || !checkDirectoryPerm(self->user, self->htmlDir, 
			     VAR_CACHE_M_MDTX_HOME_COLL_HTML)
      )
    goto error;

  self->memoryState |= EXPANDED;
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "fails to expand collection");
  }
  label = destroyString(label);
  return rc;
}

/*=======================================================================
 * Function   : populateCollection 
 * Description: Populate a collection
 * Synopsis   : int populateCollection(Collection* self)
 * Input      : Collection* self = collection to populate
 * Output     : TRUE on success
 =======================================================================*/
int 
populateCollection(Collection* self)
{ 
  int rc = FALSE;
  Configuration* conf = 0;
  int uid = getuid();

  // defensive code
  if (self == 0) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!(self->memoryState & EXPANDED)) {
    logMemory(LOG_ERR, "collection must be expanded first");
    goto error;
  }
  if (self->memoryState & POPULATED) {
    logMemory(LOG_INFO, "%s collection already populated", self->label);
    goto end;
  }
  logMemory(LOG_DEBUG, "populate the %s collection", self->label);

  if (!becomeUser(self->user, FALSE)) goto error;

  rc = TRUE;

  if (!rc) goto error;
  rc = FALSE;

  // load the user public key
  if (!access(self->sshRsaPublicKey, F_OK) &&
      (self->userKey = readPublicKey(self->sshRsaPublicKey))) goto next;
  if (!access(self->sshDsaPublicKey, F_OK) &&
      (self->userKey = readPublicKey(self->sshDsaPublicKey))) goto next;
  logMemory(LOG_ERR, "fails to load the user public key");
  goto error;

 next:
  if (!getFingerPrint(self->userKey, self->userFingerPrint)) goto error;
  self->memoryState |= POPULATED;
  end:
  rc = TRUE;
 error:
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logMemory(LOG_ERR, "fails to populate %s collection", self->label);
  }
  return rc;
}

/*=======================================================================
 * Function   : serializeCollection 
 * Description: Serialize collection
 * Synopsis   : int serialize(Collection* self, FILE* fd)
 * Input      : Collection* self = what to serialize
 *              fd               = where to serialize
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeCollection(Collection* self, FILE* fd)
{ 
  int rc = FALSE;
  Configuration* conf = 0;
  Support* supp = 0;
  char* string = 0;
  RGIT* curr = 0;

  if(self == 0) goto error;
  logMemory(LOG_DEBUG, "serialize the %s collection", self->label);
  if (!(conf = getConfiguration())) goto error;

  fprintf(fd, "\nCollection %s-%s@%s:%i\n", 
	  self->masterLabel, self->label, 
	  self->masterHost, self->masterPort);

  /* mediatex network */
  if (!isEmptyRing(self->networks)) {
    fprintf(fd, "\t%s  ", "networks");
    if (!rgSort(self->networks, cmpString)) goto error;
    curr = 0;
    if (!(string = rgNext_r(self->networks, &curr))) goto error;
    fprintf(fd, "%s", string);
    while ((string = rgNext_r(self->networks, &curr))) {
      fprintf(fd, ", %s", string);
    }
    fprintf(fd, "\n");
  }
  
  if (!isEmptyRing(self->gateways)) {
    fprintf(fd, "\t%s  ", "gateways");
    if (!rgSort(self->gateways, cmpString)) goto error;
    curr = 0;
    if (!(string = rgNext_r(self->gateways, &curr))) goto error;
    fprintf(fd, "%s", string);
    while ((string = rgNext_r(self->gateways, &curr))) {
      fprintf(fd, ", %s", string);
    }
    fprintf(fd, "\n");
  }

  /* cache parameters (maccro need to be protected by {}) */
  if (self->cacheSize > 0 && self->cacheSize != conf->cacheSize) {
    printCacheSize(fd, "\t%s", "cacheSize", self->cacheSize);
  }
  if (self->cacheTTL > 0 && self->cacheTTL != conf->cacheTTL) {
    printLapsTime(fd, "\t%s ", "cacheTTL", self->cacheTTL);
  }
  if (self->queryTTL > 0 && self->queryTTL != conf->queryTTL) {
    printLapsTime(fd, "\t%s ", "queryTTL", self->queryTTL);
  }

  /* mediatex private parameters */
  fprintf(fd, "\t# --\n");
  fprintf(fd, "\tlocalhost %s\n", self->userFingerPrint); 

  // list of supports
  if (!isEmptyRing(self->supports)) {
    if (!rgSort(self->supports, cmpSupport)) goto error;
    curr = 0;
    if (!(supp = rgNext_r(self->supports, &curr))) goto error;
    fprintf(fd, "\tshare\t  %s", supp->name);
    while ((supp = rgNext_r(self->supports, &curr))) {
      fprintf(fd, ",\n\t\t  %s", supp->name);
    }
    fprintf(fd, "\n");
  }

  fprintf(fd, "end\n");
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "error while serializing Collection");
  }
  return rc;
}


/*=======================================================================
 * Function   : createConfiguration
 * Description: Create, by memory allocation a Configuration projection.
 * Synopsis   : Configuration* createConfiguration(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Configuration* 
createConfiguration(void)
{
  Configuration* rc = 0;
  Configuration* conf = 0;
  ScoreParam defaultScoreParam = DEFAULT_SCORE_PARAM;
  char* tmpDir = 0;
  char* pidDir = 0;
  char* label = 0;
  int i = 0;

  if((conf = (Configuration*)malloc(sizeof(Configuration))) 
     == 0) {
    logMemory(LOG_ERR, "malloc: cannot create Configuration");
    goto error;
  }
    
  memset(conf, 0, sizeof(Configuration));
  conf->sem = -1;
  if ((conf->allNetworks = createRing()) == 0) goto error;
  if ((conf->networks = createRing()) == 0) goto error;
  if ((conf->gateways = createRing()) == 0) goto error;
  if ((conf->collections = createRing()) == 0) goto error;
  if ((conf->supports = createRing()) == 0) goto error;

  label = destroyString(label);
  if (!(label = createString("/"))
      || !(label = catString(label, env.confLabel)))
    goto error;

  // prefix by "$PWD/tmp" on unit tests
  if (env.noRegression) {
    if (!(tmpDir = get_current_dir_name())) {
      logMemory(LOG_ERR, "get_current_dir_name fails: %s", 
		strerror(errno));
      goto error;
    }

    remind(tmpDir);

    // remove prefix up to "_build" on "make distcheck"
    if (strstr(CONF_PREFIX, "_inst")) i = strlen(CONF_PREFIX);
    //logMemory(LOG_NOTICE, "CONF_PREFIX=%s, i=%i", CONF_PREFIX, i);

    if (!(tmpDir = catString(tmpDir, "/tmp"))
	|| !(pidDir = createString(tmpDir))
	|| !(conf->gitBareDir = createString(tmpDir))
	|| !(conf->scriptsDir = createString(tmpDir))
	|| !(conf->homeDir = createString(tmpDir))
	|| !(conf->hostSshDir = createString(tmpDir)))
      goto error;
  }

  // directories
  if (!(pidDir = catString(pidDir, CONF_PIDDIR+i))
      || !(conf->gitBareDir = catString(conf->gitBareDir, CONF_STATEDIR+i))
      || !(conf->gitBareDir = catString(conf->gitBareDir, label))
      || !(conf->scriptsDir = catString(conf->scriptsDir, CONF_SCRIPTS+i))
      || !(conf->homeDir = catString(conf->homeDir, CONF_CACHEDIR+i))
      || !(conf->homeDir = catString(conf->homeDir, label))
      || !(conf->jailDir = createString(conf->homeDir))
      || !(conf->jailDir = catString(conf->jailDir, CONF_JAIL))
      || !(conf->md5sumDir = createString(conf->homeDir))
      || !(conf->md5sumDir = catString(conf->md5sumDir, CONF_MD5SUMS))
      || !(conf->cacheDir = createString(conf->homeDir))
      || !(conf->cacheDir = catString(conf->cacheDir, CONF_CACHES))
      || !(conf->extractDir = createString(conf->homeDir))
      || !(conf->extractDir = catString(conf->extractDir, CONF_EXTRACT))
      || !(conf->gitDir = createString(conf->homeDir))
      || !(conf->gitDir = catString(conf->gitDir, CONF_GITCLT))
      || !(conf->mdtxGitDir = createString(conf->gitDir))
      || !(conf->mdtxGitDir = catString(conf->mdtxGitDir, label))
      || !(conf->hostSshDir = catString(conf->hostSshDir, CONF_HOSTSSH+i)))
    goto error;

  // files
  if (!(conf->supportDB = createString(conf->mdtxGitDir))
      || !(conf->supportDB = catString(conf->supportDB, CONF_SUPPFILE))
      || !(conf->confFile = createString(conf->mdtxGitDir))
      || !(conf->confFile = catString(conf->confFile, label))
      || !(conf->confFile = catString(conf->confFile, CONF_CONFFILE))
      || !(conf->pidFile = createString(pidDir))
      || !(conf->pidFile = catString(conf->pidFile, label))
      || !(conf->pidFile = catString(conf->pidFile, CONF_PIDFILE))
      || !(conf->sshRsaPublicKey = createString(conf->hostSshDir))
      || !(conf->sshRsaPublicKey = 
	   catString(conf->sshRsaPublicKey, CONF_RSAHOSTKEY))
      || !(conf->sshDsaPublicKey = createString(conf->hostSshDir))
      || !(conf->sshDsaPublicKey = 
	   catString(conf->sshDsaPublicKey, CONF_DSAHOSTKEY)))
    goto error;

  // values
  conf->uploadRate = CONF_UPLOAD_RATE;
  conf->cacheSize = DEFAULT_CACHE_SIZE;
  conf->cacheTTL = DEFAULT_TTL_CACHE;
  conf->queryTTL = DEFAULT_TTL_QUERY;
  conf->checkTTL = DEFAULT_TTL_CHECK;
  conf->fileTTL = DEFAULT_TTL_FILE;
  conf->scoreParam = defaultScoreParam;
  strncpy(conf->host, DEFAULT_HOST, MAX_SIZE_HOST);
  conf->sshPort = SSH_PORT;
  conf->httpPort = HTTP_PORT;
  conf->httpsPort = HTTPS_PORT;
  conf->mdtxPort = env.noRegression?TESTING_PORT:CONF_PORT;

  rc = conf;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "createConfiguration fails");
    conf = destroyConfiguration(conf);
  }
  tmpDir = destroyString(tmpDir);
  pidDir = destroyString(pidDir);
  label = destroyString(label);
  return rc;
}


/*=======================================================================
 * Function   : destroyConfiguration
 * Description: Destroy a Configuration by freeing all the allocate memory.
 * Synopsis   : void destroyConfiguration(Configuration* self)
 * Input      : Configuration* self = the address of the Configuration to
 *              destroy.
 * Output     : Nil address of a Configuration.
 =======================================================================*/
Configuration* 
destroyConfiguration(Configuration* self)
{
  Configuration* rc = 0;

  if(self) {

    // directories
    self->gitBareDir = destroyString(self->gitBareDir);
    self->scriptsDir = destroyString(self->scriptsDir);
    self->homeDir = destroyString(self->homeDir);
    self->md5sumDir = destroyString(self->md5sumDir);
    self->jailDir = destroyString(self->jailDir);
    self->cacheDir = destroyString(self->cacheDir);
    self->extractDir = destroyString(self->extractDir);
    self->gitDir = destroyString(self->gitDir);
    self->mdtxGitDir = destroyString(self->mdtxGitDir);
    self->hostSshDir = destroyString(self->hostSshDir);

    // files
    self->confFile = destroyString(self->confFile);
    self->pidFile = destroyString(self->pidFile);
    self->supportDB = destroyString(self->supportDB);
    self->sshRsaPublicKey = destroyString(self->sshRsaPublicKey);
    self->sshDsaPublicKey = destroyString(self->sshDsaPublicKey);

    self->comment = destroyString(self->comment);
    self->host[0] = (char)0;
    self->mdtxPort = 0;
    self->sshPort = 0;
    self->httpPort = 0;
    self->httpsPort = 0;
    self->cacheTTL = 0;
    self->queryTTL = 0;
    self->checkTTL = 0;
    self->fileTTL = 0;
    self->hostKey = destroyString(self->hostKey);

    self->allNetworks = 
      destroyRing(self->allNetworks, (void*(*)(void*)) destroyString);
    self->supports = 
      destroyRing(self->supports, (void*(*)(void*)) destroySupport);
    self->collections = 
      destroyRing(self->collections, (void*(*)(void*)) destroyCollection);

    self->networks = destroyOnlyRing(self->networks);
    self->gateways = destroyOnlyRing(self->gateways);

    free(self);
  }
  
  return(rc);
}

/*=======================================================================
 * Function   : expandConfiguration
 * Description: Initialize all fields on collections
 * Synopsis   : int completeConfiguration(Configuration* self)
 * Input      : N/A
 * Output     : True on success
 =======================================================================*/
int 
expandConfiguration()
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;

  // defensive code
  if (!(conf = getConfiguration())) goto error;
  logMemory(LOG_DEBUG, "expand configuration");
  if (conf->memoryState & EXPANDED) {
    logMemory(LOG_INFO, "configuration already expanded");
    goto end;
  }

  // check directories
  if (FALSE
      || !checkDirectoryPerm(0, conf->homeDir, VAR_CACHE_M_MDTX_HOME)
      || !checkDirectoryPerm(0, conf->jailDir, VAR_CACHE_M_MDTX_JAIL)
      || !checkDirectoryPerm(0, conf->md5sumDir, VAR_CACHE_M_MDTX_MD5SUMS)
      || !checkDirectoryPerm(0, conf->cacheDir, VAR_CACHE_M_MDTX_CACHE)
      || !checkDirectoryPerm(0, conf->extractDir, VAR_CACHE_M_MDTX_TMP)
      || !checkDirectoryPerm(0, conf->gitDir, VAR_CACHE_M_MDTX_GIT)
      || !checkDirectoryPerm(0, conf->mdtxGitDir, VAR_CACHE_M_MDTX_GIT_MDTX)
      )
    goto error;

  // expand collections
  if (conf->collections) {
    while ((coll = rgNext_r(conf->collections, &curr))) {
      if (!expandCollection(coll)) goto error;
    }
  }

  conf->memoryState |= EXPANDED;
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "fails to expand configuration");
  }
  return rc;
}

/*=======================================================================
 * Function   : populateConfiguration 
 * Description: Populate a Configuration
 * Synopsis   : int populateConfiguration()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
populateConfiguration()
{ 
  int rc = FALSE;
  Configuration* conf = 0;

  // defensive code
  if (!(conf = getConfiguration())) goto error;
  /* if (!(conf->memoryState & EXPANDED)) { */
  /*   logMemory(LOG_ERR, "configuration must be expanded first"); */
  /*   goto error; */
  /* } */
  if (conf->memoryState & POPULATED) {
    logMemory(LOG_INFO, "configuration already populated");
    goto end;
  }
  logMemory(LOG_DEBUG, "populate configuration");

  // load the host public key
  if (!access(conf->sshRsaPublicKey, F_OK) && 
      (conf->hostKey = readPublicKey(conf->sshRsaPublicKey))) goto next;
  if (!access(conf->sshDsaPublicKey, F_OK) &&
      (conf->hostKey = readPublicKey(conf->sshDsaPublicKey))) goto next;
  logMemory(LOG_ERR, "fails to load the host public key");
  goto error;

 next:
  if (!getFingerPrint(conf->hostKey, conf->hostFingerPrint)) goto error;
  conf->memoryState |= POPULATED;
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "fails to populate Configuration");
  }
  return(rc);
}

/*=======================================================================
 * Function   : serializeConfiguration 
 * Description: Serialize a Configuration
 * Synopsis   : int serializeConfiguration(Configuration* self, char* path)
 *
 * Input      : Configuration* self = what to serialize
 *              char* path: file where to write
 *
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeConfiguration(Configuration* self)
{ 
  int rc = FALSE;
  char* path = 0;
  FILE* fd = stdout; 
  Collection* coll = 0;
  RGIT* curr = 0;
  char* string = 0;
  int uid = getuid();

  if(self == 0) goto error;
  logMemory(LOG_DEBUG, "serialize the configuration");

  // we need the host key to be loaded so as to be able
  // to print the host fingerprint !
  if (!(self->memoryState & POPULATED)) {
    logMemory(LOG_ERR, "configuration must be populated first");
    goto error;
  }

  if (!becomeUser(env.confLabel, TRUE)) goto error;

  // output file
  if (!env.dryRun) path = self->confFile;
  logMemory(LOG_INFO, "Serializing configuration into: %s", 
	  path?path:"stdout");
  if (path && *path != (char)0) {
    if ((fd = fopen(path, "w")) == 0) {
      logMemory(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
      fd = stdout;
      goto error;
    }
    if (!lock(fileno(fd), F_WRLCK)) goto error;
  }

  fprintf(fd, "# This is the MediaTeX software configuration file.\n");

  fprintf(fd, "\n# comment: greeter for this server\n");
  fprintf(fd, "%-10s \"%s\"\n", "comment", self->comment);

  fprintf(fd, "\n# host: hostname used in urls\n");
  fprintf(fd, "%-10s %s\n", "host", self->host);

  fprintf(fd, "\n# port: listening port for incoming requests\n");
  fprintf(fd, "%-10s %i\n", "mdtxPort", self->mdtxPort);
  
  fprintf(fd, "\n# port: listening port for SSHd\n");
  fprintf(fd, "%-10s %i\n", "sshPort", self->sshPort);

  fprintf(fd, "\n# port: listening ports for Apache\n");
  fprintf(fd, "%-10s %i\n", "httpPort", self->httpPort);
  fprintf(fd, "%-10s %i\n", "httpsPort", self->httpsPort);

  fprintf(fd, "\n# networks: networks the host belongs to\n");
  fprintf(fd, "%-10s ", "networks");
  if (isEmptyRing(self->networks)) {
    fprintf(fd, "%s\n", "www");
  }
  else {
    if (!rgSort(self->networks, cmpString)) goto error;
    curr = 0;
    if (!(string = rgNext_r(self->networks, &curr))) goto error;
    fprintf(fd, "%s", string);
    while ((string = rgNext_r(self->networks, &curr))) {
      fprintf(fd, ", %s", string);
    }
    fprintf(fd, "\n");
  }
  
  fprintf(fd, "\n# gateways: networks the host is a gateway for\n");
  if (!isEmptyRing(self->gateways)) {
    fprintf(fd, "%-10s ", "gateways");
    if (!rgSort(self->gateways, cmpString)) goto error;
    curr = 0;
    if (!(string = rgNext_r(self->gateways, &curr))) goto error;
    fprintf(fd, "%s", string);
    while ((string = rgNext_r(self->gateways, &curr))) {
      fprintf(fd, ", %s", string);
    }
    fprintf(fd, "\n");
  }

  fprintf(fd, "\n# default cache parameters\n");
  printCacheSize(fd, "%-10s", "cacheSize", self->cacheSize);

  printLapsTime(fd,  "%-10s", "cacheTTL",  self->cacheTTL);
  printLapsTime(fd,  "%-10s", "queryTTL",  self->queryTTL);

  fprintf(fd, "\n# local support parameters\n");
  printLapsTime(fd,  "%-10s", "checkTTL", self->checkTTL);
  printLapsTime(fd,  "%-10s", "fileTTL", self->fileTTL);
  printLapsTime(fd, "%-10s", "suppTTL",  self->scoreParam.suppTTL);
  fprintf(fd, "%-10s %.2f\n", "maxScore", self->scoreParam.maxScore);
  fprintf(fd, "%-10s %.2f\n", "badScore", self->scoreParam.badScore);
  fprintf(fd, "%-10s %.2f\n", "powSupp",  self->scoreParam.powSupp);
  fprintf(fd, "%-10s %.2f\n", "factSupp", self->scoreParam.factSupp);

  fprintf(fd, 
	  "\n# The below section is also managed by MediaTeX software.\n"
	  "# You should not edit by hand parameters bellow the -- line.\n"
	  "# (host fingerprint: %s)\n", self->hostFingerPrint);

  if (!isEmptyRing(self->collections)) {
    if (!rgSort(self->collections, cmpCollection)) goto error;
    while ((coll = rgNext_r(self->collections, &curr))) {
      if (!serializeCollection(coll, fd)) goto error;  
    }
  }
 
  fflush(fd);
  rc = TRUE;
 error:
  if (fd != stdout) {
    if (!unLock(fileno(fd))) rc = FALSE;
    if (fclose(fd)) {
      logMemory(LOG_ERR, "fclose fails: %s", strerror(errno));
      rc = FALSE;
    }
  }
  if (!logoutUser(uid)) rc = FALSE;
  return rc;
}

/*=======================================================================
 * Function   : getConfiguration
 * Description: Get the address of the current Configuration.
 * Synopsis   : Configuration* getConfiguration(void)
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
Configuration*
getConfiguration(void)
{
  Configuration* rc = 0;

  if(env.confTree == 0) {
    if ((env.confTree = createConfiguration()) == 0) {
      logMemory(LOG_ERR, "cannot malloc default collection");
      goto error;
    }
  }

  rc = env.confTree;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "fails to get configuration");
  }
  return rc;
}

/*=======================================================================
 * Function   : freeConfiguration
 * Description: free static variables
 * Synopsis   : void freeConfiguration(void)
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
void
freeConfiguration(void)
{
  logMemory(LOG_INFO, "free configuration");
  env.confTree = destroyConfiguration(env.confTree);
}


/*=======================================================================
 * Function   : getCollection
 * Description: Find a collection
 * Synopsis   : Collection* getCollection(char* label)
 * Input      : char* label = The catalog's label
 * Output     : The catalog's collection we have found
 =======================================================================*/
Collection* 
getCollection(char *label)
{
  Configuration* conf = 0;
  Collection *rc = 0;
  RGIT* curr = 0;

  if (!(conf = getConfiguration())) goto error;
  logMemory(LOG_DEBUG, "getCollection %s", label);

  // look for collection
  while ((rc = rgNext_r(conf->collections, &curr)))
    if (!strncmp(rc->label, label, MAX_SIZE_COLL)) break;

 error:
  return rc;
}

/*=======================================================================
 * Function   : addCollection
 * Description: Add a collection if not already there.
 * Synopsis   : Collection* addCollection(char* label)
 * Input      : char* label         = The catalog's label
 * Output     : The catalog's collection we have found/added
 =======================================================================*/
Collection* 
addCollection(char *label)
{
  Configuration* conf = 0;
  Collection *rc = 0;
  Collection *coll = 0;

  checkLabel(label);
  if (!(conf = getConfiguration())) goto end;
  logMemory(LOG_DEBUG, "addCollection %s", label);

  // already there
  if ((coll = getCollection(label))) goto end;

  // add new one if not already there
  if (!(coll = createCollection())) goto error;
  strncpy(coll->label, label, MAX_SIZE_COLL);
  coll->label[MAX_SIZE_COLL] = (char)0;
  if (!rgInsert(conf->collections, coll)) goto error;

 end:
  rc = coll;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "fails to add a collection");
    if (coll) destroyCollection(coll);
  }
  return rc;
}

/*=======================================================================
 * Function   : delCollection
 * Description: Del a collection if not already done.
 * Synopsis   : int delCollection(Collection* self)
 * Input      : Collection* coll: the collection to delete
 * Output     : TRUE on success
 =======================================================================*/
int
delCollection(Collection* self)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support* supp = 0;
  RGIT* curr = 0;

  checkCollection(self);
  if (!(conf = getConfiguration())) goto error;
  logMemory(LOG_DEBUG, "delCollection %s", self->label);

  // delete support associations
  curr = 0;
  while ((supp = rgHead(self->supports))) {
    if (!delSupportFromCollection(supp, self)) goto error;
  }

  // delete collection from configuration ring
  if ((curr = rgHaveItem(conf->collections, self))) {
    rgRemove_r(conf->collections, &curr);
  }

  // free the collection
  destroyCollection(self);
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delCollection fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : getNetwork
 * Description: Find a network string
 * Synopsis   : char* getNetwork(char* label)
 * Input      : char* label = The network's label
 * Output     : the string we have found (so as to get a uniq pointer)
 =======================================================================*/
char* 
getNetwork(char *label)
{
  Configuration* conf = 0;
  char* rc = 0;
  RGIT* curr = 0;

  if (!(conf = getConfiguration())) goto error;
  logMemory(LOG_DEBUG, "getNetwork %s", label);

  // look for network
  while ((rc = rgNext_r(conf->allNetworks, &curr)))
    if (!strcmp(rc, label)) break;

 error:
  return rc;
}

/*=======================================================================
 * Function   : addNetwork
 * Description: Add a network if not already there.
 * Synopsis   : char* addNetwork(char* label)
 * Input      : char* label = The network's label
 * Output     : The network we have found/added
 =======================================================================*/
char* 
addNetwork(char *label)
{
  Configuration* conf = 0;
  char* rc = 0;
  char *net = 0;

  checkLabel(label);
  if (!(conf = getConfiguration())) goto end;
  logMemory(LOG_DEBUG, "addNetwork %s", label);

  // already there
  if ((net = getNetwork(label))) goto end;

  // add new one if not already there
  if (!(net = createString(label))) goto error;
  if (!rgInsert(conf->allNetworks, net)) goto error;

 end:
  rc = net;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "fails to add a network");
    if (net) destroyString(net);
  }
  return rc;
}

/*=======================================================================
 * Function   : addNetworkToRing
 * Description: Add a network into a ring (if not already there)
 * Synopsis   : int addNetworkToRing(RG* ring, char* label)
 * Input      : char* label = The network's label
 * Output     : TRUE on success
 * Note       : do not use directly the input string so as to index it
 =======================================================================*/
int 
addNetworkToRing(RG* ring, char *label)
{
  int rc = FALSE;
  char* string = 0;

  logMemory(LOG_DEBUG, "addNetworkToRing");

  if (!(string = addNetwork(label))) goto error;
  if (!rgHaveItem(ring, string)) {
    if (!rgInsert(ring, string)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "fails to add a network to a ring");
  }
  return rc;
}

/*=======================================================================
 * Function   : addSupportToCollection
 * Description: Share a support with a collection
 * Synopsis   : int addSupportToCollection(Support* support, 
 *                                                     Collection* coll)
 * Input      : Support* support: the support to share
 *              Collection* coll: the collection to share with
 * Output     : TRUE on success
 =======================================================================*/
int addSupportToCollection(Support* support, Collection* coll)
{
  int rc = FALSE;
  int alreadyThere = FALSE;

  checkCollection(coll);
  checkSupport(support);
  logMemory(LOG_DEBUG, "addSupportToCollection %s to %s", 
	  support->name, coll->label);

  // add collection to the support ring
  if (rgHaveItem(support->collections, coll)) {
    alreadyThere = TRUE;
  }
  else {
    if (!rgInsert(support->collections, coll)) goto error;
  }

  // add support to collection ring
  if (rgHaveItem(coll->supports, support)) {
    alreadyThere = TRUE;
  }
  else {
    if (!rgInsert(coll->supports, support)) goto error;
  }

  if (alreadyThere) {
    logMemory(LOG_NOTICE, "support %s already added to collection %s",
	      support->name, coll->label);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addSupportToCollection fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : delSupportFromCollection
 * Description: Withdraw a support with a collection
 * Synopsis   : int delSupportFromCollection(Support* support, 
 *                                                     Collection* coll)
 * Input      : Support* support: the support to un-share
 *              Collection* coll: the collection to un-share with
 * Output     : TRUE on success
 =======================================================================*/
int delSupportFromCollection(Support* support, Collection* coll)
{
  int rc = FALSE;
  RGIT* curr = 0;

  checkCollection(coll);
  checkSupport(support);
  logMemory(LOG_DEBUG, "delSupportFromCollection %s from %s",
	  support->name, coll->label);

  // del collection to the support ring
  if ((curr = rgHaveItem(support->collections, coll))) {
    rgRemove_r(support->collections, &curr);
  }

  // del support to collection ring
  if ((curr = rgHaveItem(coll->supports, support))) {
    rgRemove_r(coll->supports, &curr);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delSupportFromCollection fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : getLocalHost
 * Description: Get the localhost server
 * Synopsis   : Server* getLocalHost(Collection* coll)
 * Input      : Collection* coll: 
 * Output     : The localhost server, 0 on error
 * Requirement: populateCollection(coll)
 =======================================================================*/
Server* 
getLocalHost(Collection* coll)
{
  Configuration* conf = 0;
  Server* rc = 0;

  checkCollection(coll);
  if (!(conf = getConfiguration())) goto error;
  logMemory(LOG_DEBUG, "getLocalHost");

  // assert we have the localhost server object
  if (!coll->localhost) {
    if (!(coll->localhost = addServer(coll, coll->userFingerPrint))) 
      goto error;
  }

  // minimum requirements to survive without a true upgrade (for server)
  strncpy(coll->localhost->host, conf->host, MAX_SIZE_HOST);
  coll->localhost->mdtxPort = conf->mdtxPort;
  coll->localhost->sshPort = conf->sshPort;

  rc = coll->localhost;
 error:
  if (rc == 0) {
    logMemory(LOG_ERR, "fails to get localhost server object");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

