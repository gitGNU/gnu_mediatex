/*=======================================================================
 * Project: MediaTeX
 * Module : openClose
 
 * Manage meta-data files

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

static char* CollFiles[] = {
  "    ", "   C", "  X ", "  XC", " S  ", " S C", " SX ", " SXC",
  "5   ", "5  C", "5 X ", "5 XC", "5S  ", "5S C", "5SX ", "5SXC"
};

/*=======================================================================
 * Function   : callUpgrade
 * Description: call upgrade.sh
 * Synopsis   : int callUpgrade(char* user, 
 *                              char* signature1, char* signature2)
 * Input      : char* label: the collection module to commit
 *              char* fingerprint: the commit's author fingerprint
 *              char* url: remote origin url (if provided)
 * Output     : TRUE on success
 =======================================================================*/
int 
callUpgrade(char* user, char* fingerprint, char* url)
{ 
  int rc = FALSE;  
  Configuration* conf = 0;
  char *argv[] = {0, 0, 0, 0, 0, 0};

  checkLabel(user);
  logCommon(LOG_INFO, "callUpgrade %s: fp=%s url=%s",
	    user, fingerprint?fingerprint:"?", url?url:"");

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/upgrade.sh"))) goto error;
  argv[1] = user;
  argv[2] = conf->host;
  argv[3] = *fingerprint?fingerprint:"?";
  argv[4] = url;
  
  if (!env.noRegression && !env.dryRun && !env.noGit) {
    if (!execScript(argv, user, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logCommon(LOG_WARNING, "callUpgrade fails");
  }
  argv[0] = destroyString(argv[0]);
  return(rc);
}

/*=======================================================================
 * Function   : callCommit
 * Description: call commit.sh
 * Synopsis   : int callCommit(char* user, char* comment)
 * Input      : char* label: the collection module to commit
 *              char* comment: to overhide command line as comment
 * Output     : TRUE on success
 =======================================================================*/
int 
callCommit(char* user, char* comment)
{ 
  int rc = FALSE;  
  Configuration* conf = 0;
  char *argv[] = {0, 0, 0, 0};

  checkLabel(user);
  env.commandLine[strlen(env.commandLine)-1] = 0; // \n -> \0
  logCommon(LOG_INFO, "callCommit %s: %s", user,
	    comment?comment:env.commandLine);

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/commit.sh"))) goto error;
  argv[1] = user;
  argv[2] = comment?comment:env.commandLine;
  
  if (!env.noRegression && !env.dryRun && !env.noGit) {
    if (!execScript(argv, user, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logCommon(LOG_WARNING, "callCommit fails");
  }
  argv[0] = destroyString(argv[0]);
  env.commandLine[strlen(env.commandLine)] = '\n';
  return(rc);
}

/*=======================================================================
 * Function   : callPull
 * Description: call pull.sh
 * Synopsis   : int callPull(char* user)
 * Input      : char* user: the collection module to pull
 * Output     : TRUE on success
 =======================================================================*/
int 
callPull(char* user)
{ 
  int rc = FALSE;
  Configuration* conf = 0;
  char *argv[] = {0, 0, 0};

  checkLabel(user);
  logCommon(LOG_INFO, "callPull %s", user);

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/pull.sh"))) goto error;
  argv[1] = user;

  if (!env.noRegression && !env.dryRun && !env.noGitPullPush) {
    if (!execScript(argv, user, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logCommon(LOG_WARNING, "callPull fails");
  }
  argv[0] = destroyString(argv[0]);
  return(rc);
}

/*=======================================================================
 * Function   : callPush
 * Description: call push.sh
 * Synopsis   : int callPush(char* user)
 * Input      : char* label: the collection module to commit
 * Output     : TRUE on success
 =======================================================================*/
int 
callPush(char* user)
{ 
  int rc = FALSE;  
  Configuration* conf = 0;
  char *argv[] = {0, 0, 0};

  checkLabel(user);
  logCommon(LOG_INFO, "callPush %s", user);

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/push.sh"))) goto error;
  argv[1] = user;
  
  if (!env.noRegression && !env.dryRun && !env.noGitPullPush) {
    if (!execScript(argv, user, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logCommon(LOG_WARNING, "callPush fails");
  }
  argv[0] = destroyString(argv[0]);
  return(rc);
}

/*=======================================================================
 * Function   : loadConfiguration
 * Description: Call the parser on private files
 * Synopsis   : int loadConfiguration(int confFiles)
 * Input      : int confFiles: OR from ConfFile (CFG,SUPP)
 * Output     : TRUE on success
 =======================================================================*/
int loadConfiguration(int confFiles)
{
  int rc = FALSE;
  Configuration* conf = 0;

  logCommon(LOG_DEBUG, "load configuration");
  if (!confFiles) {
    logCommon(LOG_ERR, "please provide files to load");
    goto error;
  }
  if (!(conf = getConfiguration())) goto error;

  if (!env.noGit) {
    if (!callCommit(env.confLabel, "manual user edition")) goto error;
    if (!env.noGitPullPush) callPull(env.confLabel);
  }
  
  if ((confFiles & CFG) && conf->fileState[iCFG] == DISEASED) {
    if (!parseConfiguration(conf->confFile)) goto error;
    conf->fileState[iCFG] = LOADED;
  }

  if ((confFiles & SUPP) && conf->fileState[iSUPP] == DISEASED) {
    if (!parseSupports(conf->supportDB)) goto error;
    conf->fileState[iSUPP] = LOADED;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to load configuration");   
  }
  return rc;
}


/*=======================================================================
 * Function   : loadRecords
 * Description: Call the parser on records file
 * Synopsis   : int loadRecords(Collection* coll)
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int loadRecords(Collection* coll)
{
  int rc = FALSE;
  int fd = -1;
  RecordTree* tree = 0;
  Record* record = 0;
  AVLNode* node = 0;
  
  checkCollection(coll);
  logCommon(LOG_DEBUG, "load %s records", coll->label);
  logCommon(LOG_INFO, "parse records file: %s", coll->md5sumsDB);

  // compute scores (only once), as score will be used by addCacheEntry
  if (!computeExtractScore(coll)) goto error;

  if (access(coll->md5sumsDB, R_OK) == -1) {
    logCommon(LOG_NOTICE, "no md5sums file: %s", coll->md5sumsDB);
    goto end;
  }

  // open md5sumsDB file
  if ((fd = open(coll->md5sumsDB, O_RDONLY)) == -1) {
    logCommon(LOG_ERR, "open: %s", strerror(errno));
    logCommon(LOG_ERR, "cannot open records file: %s", coll->md5sumsDB);
    goto error;
  }
  if (!lock(fd, F_RDLCK)) goto error;

  // parse md5sumsDB file into the main recordTree
  if ((tree = parseRecords(fd)) == 0) goto error;

  if (!unLock(fd)) goto error;
  if (close(fd) == -1) {
    logCommon(LOG_ERR, "close: %s", strerror(errno));
    logCommon(LOG_ERR, "cannot close records file: %s", coll->md5sumsDB);
    goto error;
  }

  // index the record tree
  if (!diseaseCacheTree(coll)) goto error;
  destroyRecordTree(coll->cacheTree->recordTree);
  coll->cacheTree->recordTree = tree;
  coll->cacheTree->recordTree->messageType = DISK;
  for (node = tree->records->head; node; node = node->next) {
    record = node->item;
    if (!addCacheEntry(coll, record)) goto error;
  }
   
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to load records");   
  }
  return rc;
}

/*=======================================================================
 * Function   : strCF
 * Description: return a string for collFiles
 * Synopsis   : char* strCF(int collFiles)
 * Input      : int collFiles
 * Output     : char*    
 =======================================================================*/
char* 
strCF(int collFiles)
{
  return CollFiles[collFiles];
}

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : Collection* coll: collection to load
 *              int collFiles: OR from CollFiles (CTLG,EXTR,SERV)
 * Output     : TRUE on success
 =======================================================================*/
static int 
collectionLoop(Collection* coll, int collFiles,
	       int (*callback)(Collection* coll, int fileIdx))
{
  int rc = FALSE;
  int i = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "file loop on %s collection (%s)", 
	  coll->label, strCF(collFiles));

  // reverse order:
  // - cache need computeExtractScore so cache must be disease first.
  // - catalog get archives so they need to be added first by extract.
  for (i=iCACH; i>=iCTLG; --i) {
    if (collFiles & (1<<i)) {
      if (!callback(coll, i)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "file loop fails");   
  }
  return rc;
}

/*=======================================================================
 * Function   : loadCvsFiles
 * Description: Call the parser on shared files
 * Synopsis   : int loadCvsFiles(Collection* coll, char* base)
 * Input      : Collection* coll: collection to load
 *              int fileIdx
 * Output     : TRUE on success
 =======================================================================*/
static int 
loadCvsFiles(Collection* coll, int fileIdx)
{
  int rc = FALSE;
  int (*parser)(Collection*, const char* path);
  char* path = 0;
  int l = 0;
  int i = 0;

  logCommon(LOG_DEBUG, "load %s files", strCF(1<<fileIdx));

  switch (fileIdx) {
  case iCTLG:
    if (!(path = createString(coll->catalogDB))) goto error;
    parser = parseCatalogFile;
    break;
  case iEXTR:
    if (!(path = createString(coll->extractDB))) goto error;
    parser = parseExtractFile;
    break;
  default:
    logCommon(LOG_ERR, "metadata file not handle"); 
    goto error;
  }

  l = strlen(path);
  if (!(path = catString(path, "000.txt"))) goto error;

  // load from part files
  do {
    if (!sprintf(path+l, "%03i.txt", i)) goto error;
    if (access(path, R_OK)) break;
    if (!parser(coll, path)) goto error;
  }
  while (++i < 1000);
  coll->fileState[fileIdx] = LOADED;

  // load last addon
  if (!sprintf(path+l, "%s", "NNN.txt")) goto error;
  if (access(path, R_OK) == 0) {
    if (!parser(coll, path)) goto error;
    i=1;

    // cgi and server only read the meta-data
    if (!env.noGit) {
      // force re-serialize and commit to eat NNN.txt
      coll->fileState[fileIdx] = MODIFIED;
    }
  }  
  
  if (!i) {
    logCommon(LOG_INFO, "no metadata file founded");
  }
  rc = (i >= 0);
 error:
  if (!rc) {
    logCommon(LOG_ERR, "loadCvsFiles fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : loadColl
 * Description: Call the parser on shared files
 * Synopsis   : int loadColl(Collection* coll, int fileIdx)
 * Input      : Collection* coll: collection to load
 *              int fileIdx: CTLG,EXTR or SERV
 * Output     : TRUE on success
 =======================================================================*/
static int 
loadColl(Collection* coll, int fileIdx)
{
  int rc = FALSE;
  int err = 0;

  checkCollection(coll);
  if (fileIdx < iCTLG || fileIdx > iCACH) goto error;
  logCommon(LOG_DEBUG, "do load %s collection (%s)", 
	  coll->label, strCF(1<<fileIdx));

  if ((err = pthread_mutex_lock(&coll->mutex[fileIdx]))) {
    logCommon(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  
  // load if needed
  if (coll->fileState[fileIdx] == DISEASED) {
    switch (fileIdx) {
    case iCTLG:
      if (!loadCvsFiles(coll, iCTLG)) goto error2;
      break;
    case iEXTR:
      if (!loadCvsFiles(coll, iEXTR)) goto error2;
      break;
    case iSERV:
      if (!parseServerFile(coll, coll->serversDB)) goto error2;
      coll->fileState[iSERV] = LOADED;

      // cgi and server only read the meta-data
      if (!env.noGit) {
	if (!upgradeCollection(coll)) goto error2;
	coll->fileState[iSERV] = MODIFIED;
      }
      if (!computeUrls(coll)) goto error2;

      break;
    case iCACH:
      if (!loadRecords(coll)) goto error2;	
      coll->fileState[iCACH] = LOADED;
      break;
    default:
      goto error2;
    }
  }
  
  // in use +1
  ++coll->cptInUse[fileIdx];

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&coll->mutex[fileIdx]))) {
    logCommon(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logCommon(LOG_ERR, "loadColl fails");   
  }
  return rc;
}

/*=======================================================================
 * Function   : loadCollectionNbSteps
 * Description: Estimate the maximum number of steps for progBar
 * Synopsis   : 
 * Input      : Collection* coll: collection to load
 *              int collFiles: OR from CollFiles (CTLG,EXTR,SERV)
 * Output     : TRUE on success
 =======================================================================*/
static int
loadCollectionNbSteps(Collection* coll, int collFiles)
{
  int rc = FALSE;
  char* path = 0;
  FILE* fd = 0;
  int fileIdx = 0, i = 0, l = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "loadCollectionNbSteps");

  env.progBar.cur = env.progBar.max = 0;
  for (fileIdx=iCTLG; fileIdx<=iCACH; ++fileIdx) {
    if (collFiles & (CTLG | EXTR) & (1<<fileIdx)) {
      if (coll->fileState[i] != DISEASED) continue;
      switch (fileIdx) {
      case iCTLG:
	if (!(path = createString(coll->catalogDB))) goto error;
	break;
      case iEXTR:
	if (!(path = createString(coll->extractDB))) goto error;
	break;
      default:
	goto error;
      }
      
      i = 0;
      l = strlen(path);
      if (!(path = catString(path, "000.txt"))) goto error;
      
      // part files
      do {
	if (!sprintf(path+l, "%03i.txt", i)) goto error;
	if (access(path, R_OK)) break;
	
	// roughtly estimate the number of steps (number of lines)
	fd = fopen(path, "r");
	while (EOF != (fscanf(fd, "%*[^\n]"), fscanf(fd, "%*c")))
	  ++env.progBar.max;
	fclose(fd);
      }
      while (++i < 1000);
      
      // load last addon
      if (!sprintf(path+l, "%s", "NNN.txt")) goto error;
      if (access(path, R_OK) == 0) {
	fd = fopen(path, "r");
	while (EOF != (fscanf(fd, "%*[^\n]"), fscanf(fd, "%*c")))
	  ++env.progBar.max;
	fclose(fd);
      }
      path = destroyString(path);
    }    
  }

  // add 3%
  env.progBar.max *= 1.03;

  if (env.progBar.max > 0) {
    logCommon(LOG_INFO, "estimate %i steps for load", env.progBar.max);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "loadCollectionNbSteps fails");   
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : loadCollection
 * Description: Call the parser on shared files
 * Synopsis   : int loadCollection(Collection* coll, int collFiles)
 * Input      : Collection* coll: collection to load
 *              int collFiles: OR from CollFiles (CTLG,EXTR,SERV)
 * Output     : TRUE on success
 =======================================================================*/
int 
loadCollection(Collection* coll, int collFiles)
{
  int rc = FALSE;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "load %s collection (%s)", 
	    coll->label, strCF(collFiles));

  // force catalog to use archives from extract (assert no ophanes)
  if (collFiles & CTLG) collFiles |= EXTR;

  if (!expandCollection(coll)) goto error;

  if (!env.noGit) {
    if (!callCommit(coll->user, "manual user edition")) goto error;
    if (!env.noGitPullPush) {
      if (coll->toUpdate && collFiles & (CTLG | EXTR | SERV)) {
	logMain(LOG_INFO, "Git pull %s collection", coll->label);
	if (callPull(coll->user)) coll->toUpdate = FALSE;
      }
    }
  }

  if (!loadCollectionNbSteps(coll, collFiles)) goto error;
  if (env.progBar.max > 0) {
    logMain(LOG_INFO, "parse %s collection (%s)", 
	    coll->label, strCF(collFiles));
  }

  startProgBar("load");
  if (!collectionLoop(coll, collFiles, loadColl)) goto error;
  stopProgBar();

  if (env.progBar.max) {
    logCommon(LOG_INFO, "steps: %lli / %lli", 
	      env.progBar.cur, env.progBar.max);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to load collection");   
  }
  return rc;
}

/*=======================================================================
 * Function   : wasModifiedColl
 * Description: Call serializer on private files
 * Synopsis   : int wasModifiedColl()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
static int 
wasModifiedColl(Collection* coll, int i)
{
  int rc = FALSE;
  int err = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "do set %s collection as modified (%s)", 
	  coll->label, strCF(1<<i));

  if ((err = pthread_mutex_lock(&coll->mutex[i]))) {
    logCommon(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  
  coll->fileState[i] = MODIFIED;
  
  if ((err = pthread_mutex_unlock(&coll->mutex[i]))) {
    logCommon(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to set collection as modified");
  }
  return rc;
}

/*=======================================================================
 * Function   : wasModifiedCollection
 * Description: Call serializer on private files
 * Synopsis   : int wasModifiedCollection()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
wasModifiedCollection(Collection* coll, int collFiles)
{
  int rc = FALSE;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "set %s collection as modified (%s)", 
	  coll->label, strCF(collFiles));

  if (!loadCollection(coll, collFiles)) goto error;
  if (!collectionLoop(coll, collFiles, wasModifiedColl)) goto error2;

  rc = TRUE;
 error2:
  releaseCollection(coll, collFiles);
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to set collection as modified");
  }
  return rc;
}

/*=======================================================================
 * Function   : releaseCollection
 * Description: Call serializer on private files
 * Synopsis   : int releaseCollection()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
static int 
releaseColl(Collection* coll, int i)
{
  int rc = FALSE;
  int err = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "do release %s collection (%s)", 
	  coll->label, strCF(1<<i));

  if ((err = pthread_mutex_lock(&coll->mutex[i]))) {
    logCommon(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  
  // in use -1
  --coll->cptInUse[i];

  // assert cpt >= 0
  if (coll->cptInUse[i] < 0) {
    logCommon(LOG_WARNING, "cptInUse for %s = %i !", 
	    strCF(1<<i), coll->cptInUse[i]);
  }

  if ((err = pthread_mutex_unlock(&coll->mutex[i]))) {
    logCommon(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to do release collection");
  }
  return rc;
}

/*=======================================================================
 * Function   : releaseCollection
 * Description: Call serializer on private files
 * Synopsis   : int releaseCollection()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
releaseCollection(Collection* coll, int collFiles)
{
  int rc = FALSE;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "release %s collection (%s)", 
	  coll->label, strCF(collFiles));

  // force catalog to use archives from extract (assert no ophanes)
  if (collFiles & CTLG) collFiles |= EXTR;

  if (!collectionLoop(coll, collFiles, releaseColl)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to release collection");
  }
  return rc;
}

/*=======================================================================
 * Function   : saveConfiguration
 * Description: Call serializer on private files
 * Synopsis   : int saveConfiguration()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
saveConfiguration()
{
  int rc = FALSE;
  Configuration* conf = 0;
  int change = FALSE;

  logCommon(LOG_DEBUG, "save configuration");
  if (!(conf = getConfiguration())) goto error;

  if (conf->fileState[iCFG] == MODIFIED) {
    if (!expandConfiguration()) goto error;
    if (!populateConfiguration()) goto error;
    if (!serializeConfiguration(conf)) goto error;
    conf->fileState[iCFG] = LOADED;
    change = TRUE;
    conf->toHup = TRUE;
  }

  if (conf->fileState[iSUPP] == MODIFIED) {
    if (!serializeSupports()) goto error;
    conf->fileState[iSUPP] = LOADED;
    change = TRUE;
  }

  // commit changes
  if (change && !env.noGit) {
    if (!callUpgrade(env.confLabel, conf->hostFingerPrint, 0)) goto error;
    if (!callCommit(env.confLabel, 0)) goto error;
    if (!env.noGitPullPush) {
	callPush(env.confLabel);
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to save configuration");   
  }
  return rc;
}

/*=======================================================================
 * Function   : saveColl
 * Description: Call serializer on private files
 * Synopsis   : int saveColl()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
static int 
saveColl(Collection* coll, int i)
{
  int rc = FALSE;
  int err = 0;
  CvsFile fd = {0, 0, 0, FALSE, 0, cvsCutOpen, cvsCutPrint};

  checkCollection(coll);
  logCommon(LOG_DEBUG, "do save %s collection (%s)", 
	  coll->label, strCF(1<<i));

  if ((err = pthread_mutex_lock(&coll->mutex[i]))) {
    logCommon(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  // save if modifyed and no more used
  if (coll->fileState[i] == MODIFIED && coll->cptInUse[i] == 0) {
    switch (i) {
    case iCTLG:
      if (!serializeCatalogTree(coll, &fd)) goto error2;
      coll->toCommit = TRUE;
      break;
    case iEXTR:
      if (!serializeExtractTree(coll, &fd)) goto error2;
      coll->toCommit = TRUE;
      break;
    case iSERV:
      if (!serializeServerTree(coll)) goto error2;
      coll->toCommit = TRUE;
      break;
    case iCACH:
      // this must only be done by the server !
      if (!lockCacheRead(coll)) goto error2;
      coll->cacheTree->recordTree->collection = coll;
      coll->cacheTree->recordTree->messageType = DISK;
      if (!serializeRecordTree(coll->cacheTree->recordTree,
			       coll->md5sumsDB, 0)) goto error2;
      if (!unLockCache(coll)) goto error2;
      break;
    default:
      goto error2;
    }
    coll->fileState[i] = LOADED;
  }
  else {
    logCommon(LOG_INFO, "do not save %s collection (%s)",
	      coll->label, strCF(1<<i));
    if (coll->fileState[i] != MODIFIED) {
      logCommon(LOG_DEBUG, "... as not modified");
    }
    if (coll->cptInUse[i]) {
      logCommon(LOG_DEBUG, "... as still used by %i functions", 
	      coll->cptInUse[i]);
    }
  }

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&coll->mutex[i]))) {
    logCommon(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to save collection");
  }
  return rc;
}

/*=======================================================================
 * Function   : saveCollectionNbSteps
 * Description: Estimate the maximum number of steps for progBar
 * Synopsis   : 
 * Input      : Collection* coll: collection to save
 *              int collFiles: OR from CollFiles (CTLG,EXTR,SERV)
 * Output     : TRUE on success
 =======================================================================*/
static int
saveCollectionNbSteps(Collection* coll, int collFiles)
{
  int rc = FALSE;
  int fileIdx = 0, i = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "saveCollectionNbSteps");

  env.progBar.cur = env.progBar.max = 0;
  for (fileIdx=iCTLG; fileIdx<=iCACH; ++fileIdx) {
    if (collFiles & (CTLG | EXTR) & (1<<fileIdx)) {
      if (coll->fileState[i] != MODIFIED) continue;
      switch (fileIdx) {
      case iCTLG:
	env.progBar.max += avl_count(coll->catalogTree->documents);
	env.progBar.max += avl_count(coll->catalogTree->humans);
      case iEXTR:
	env.progBar.max += avl_count(coll->extractTree->containers);
	break;
      default:
	goto error;
      }
    }
  }

  if (env.progBar.max > 0) { // else nothing to do
    logCommon(LOG_INFO, "estimate %i steps for save", env.progBar.max);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "saveCollectionNbSteps fails");   
  }
  return rc;
}

/*=======================================================================
 * Function   : saveCollection
 * Description: Call serializer on private files
 * Synopsis   : int saveCollection(Collection* coll, int collFiles)
 * Input      : Collection* coll
 *              int collFiles
 * Output     : TRUE on success
 =======================================================================*/
int 
saveCollection(Collection* coll, int collFiles)
{
  int rc = FALSE;
  Configuration* conf = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "save %s collection (%s)", 
	    coll->label, strCF(collFiles));

  if (!(conf = getConfiguration())) goto error;
  if (!saveCollectionNbSteps(coll, collFiles)) goto error;
  if (env.progBar.max > 0) { // else nothing to do
    logMain(LOG_INFO, "serialize  %s collection (%s)", 
	      coll->label, strCF(collFiles));
  }

  startProgBar("save");
  if (!collectionLoop(coll, collFiles, saveColl)) goto error;
  stopProgBar();
  logCommon(LOG_INFO, "steps: %lli / %lli", 
	    env.progBar.cur, env.progBar.max);

  // commit changes
  if (coll->toCommit && !env.noGit) {
    if (!callCommit(coll->user, 0)) goto error;
    if (!env.noGitPullPush) {
      logMain(LOG_INFO, "Git push %s collection", coll->label);
      if (callPush(coll->user)) {
	conf->toHup = TRUE;
	coll->toCommit = FALSE;
	coll->toUpdate = TRUE;
      }
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to save collection");
  }
  return rc;
}

/*=======================================================================
 * Function   : clientSaveAll
 * Description: Call serializer on all modified files
 * Synopsis   : int clientSaveAll(char* cmdLine)
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
clientSaveAll()
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;
  
  logCommon(LOG_DEBUG, "clientSaveAll");
  if (!(conf = env.confTree)) goto end; // nothing was loaded

  while ((coll = rgNext_r(conf->collections, &curr))) {
    if (!saveCollection(coll, CTLG|EXTR|SERV)) goto error;
  }

  if (!saveConfiguration()) goto error;

  // tell the server we have upgrade files
  if (!env.noRegression && !env.dryRun && conf->toHup) {
    mdtxAsyncSignal(0); // send HUP signal to daemon
    conf->toHup = FALSE;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "clientSaveAll fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : serverSaveAll
 * Description: Call serializer on all modified files
 * Synopsis   : int serverSaveAll()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int serverSaveAll()
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;
  
  logCommon(LOG_DEBUG, "server save all");
  if (!(conf = env.confTree)) goto end; // nothing was loaded

  while ((coll = rgNext_r(conf->collections, &curr))) {
    if (!saveCollection(coll, CACH)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "server fails to save all");
  }
  return rc;
}

/*=======================================================================
 * Function   : diseaseCollection
 * Description: Release memory used to parse metadata files
 * Synopsis   : int diseaseCollection()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
static int 
diseaseColl(Collection* coll, int i)
{
  int rc = FALSE;
  int err = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "disease %s collection (%s)", 
	  coll->label, strCF(1<<i));

  if ((err = pthread_mutex_lock(&coll->mutex[i]))) {
    logCommon(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  } 
  
  // disease if loaded and no more used
  if (coll->fileState[i] == LOADED && coll->cptInUse[i] == 0) {
    switch (i) {
    case iCTLG:
      if (!diseaseCatalogTree(coll)) goto error2;
      break;
    case iEXTR:
      if (!diseaseExtractTree(coll)) goto error2;
      break;
    case iSERV:
      if (!diseaseServerTree(coll)) goto error2;
      break;
    case iCACH:
      if (!diseaseCacheTree(coll)) goto error2;
      break;
    default:
      goto error2;
    }
    coll->fileState[i] = DISEASED;
  }
  else {
    logCommon(LOG_INFO, "do not disease %s collection (%s)",
	      coll->label, strCF(1<<i));
    if (coll->fileState[i] == MODIFIED) {
      logCommon(LOG_DEBUG, "... as modified");
    }
    if (coll->fileState[i] != LOADED) {
      logCommon(LOG_DEBUG, "... as not loaded");
    }
    if (coll->cptInUse[i]) {
      logCommon(LOG_DEBUG, "... as still used by %i functions", 
	      coll->cptInUse[i]);
    }
  }

  rc = TRUE; 
 error2:
  if ((err = pthread_mutex_unlock(&coll->mutex[i]))) {
    logCommon(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to disease collection");
  }
  return rc;
}

/*=======================================================================
 * Function   : diseaseCollection
 * Description: Call serializer on private files
 * Synopsis   : int diseaseCollection()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
diseaseCollection(Collection* coll, int collFiles)
{
  int rc = FALSE;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "disease %s collection (%s)", 
	  coll->label, strCF(collFiles));

  if (!env.noRegression) memoryStatus(LOG_INFO, __FILE__, __LINE__);
  if (!collectionLoop(coll, collFiles, diseaseColl)) goto error;
  if (!env.noRegression) memoryStatus(LOG_INFO, __FILE__, __LINE__);

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to disease collection");
  }
  return rc;
}


/*=======================================================================
 * Function   : clientDiseaseAll
 * Description: Free memory from unchanged collections
 * Synopsis   : int clientDiseaseAll()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
clientDiseaseAll()
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;
  
  logCommon(LOG_DEBUG, "clientDiseaseAll");
  if (!(conf = env.confTree)) goto error; // do not malloc

  while ((coll = rgNext_r(conf->collections, &curr))) {
    if (!(coll->memoryState & EXPANDED)) continue;
    if (!saveCollection(coll, CTLG|EXTR|SERV)) goto error;
    if (!diseaseCollection(coll, CTLG|EXTR|SERV|CACH)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "clientDiseaseAll fails");
  }
  return rc;
}
/*=======================================================================
 * Function   : serverDiseaseAll
 * Description: Free memory from collection's caches no more used
 * Synopsis   : int serverDiseaseAll()
 * Input      : N/A
 * Output     : TRUE on success
 * Note       : call both by client and server
 =======================================================================*/
int serverDiseaseAll()
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;
  
  logCommon(LOG_DEBUG, "serverDiseaseAll");
  if (!(conf = env.confTree)) goto error; // do not malloc

  while ((coll = rgNext_r(conf->collections, &curr))) {
    if (!(coll->memoryState & EXPANDED)) continue;
    if (!saveCollection(coll, CACH)) goto error;
    if (!diseaseCollection(coll, CTLG|EXTR|SERV|CACH)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "serverDiseaseAll fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxGetCollection
 * Description: load a collection
 * Synopsis   : Collection* mdtxGetCollection(char* label)
 * Input      : char* label: collection label to search for
 * Output     : Collection*: the matching collection ; 0 if not found
 =======================================================================*/
Collection* mdtxGetCollection(char* label)
{
  Collection* rc = 0;
  Collection* coll = 0;

  checkLabel(label);  
  logCommon(LOG_DEBUG, "get %s collection", label);

  if (!loadConfiguration(CFG)) goto error;
  if (!(coll = getCollection(label))) goto error;
  if (!expandCollection(coll)) goto error;
    
  rc = coll;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to get collection");
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxGetSupport
 * Description: load a support
 * Synopsis   : Support* mdtxGetSupport(char* label)
 * Input      : char* label: support label to search for
 * Output     : Support*: the matching support ; 0 if not found
 =======================================================================*/
Support* mdtxGetSupport(char* label)
{
  Support* rc = 0;
  Support* supp = 0;

  checkLabel(label);  
  logCommon(LOG_DEBUG, "get %s support", label);

  if (!allowedUser(env.confLabel)) goto error;
  if (!loadConfiguration(SUPP)) goto error;
  if (!(supp = getSupport(label))) goto error;

  rc = supp;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "support '%s' not found (into '%s')", 
	      label, getConfiguration()->supportDB);
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxLoop
 * Description: Execute a callback function on all collections
 * Synopsis   : int clientLoop(int (*callback)(char*))
 * Input      : void (*callback)(char*): callback function
 * Output     : TRUE on success
 =======================================================================*/
int 
clientLoop(int (*callback)(char*))
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;

  logCommon(LOG_DEBUG, "loop on all collections");

  if (!allowedUser(env.confLabel)) goto error;

  // for all collection
  if (!loadConfiguration(CFG)) goto error;
  conf = getConfiguration();
  if (conf->collections) {
    while ((coll = rgNext_r(conf->collections, &curr))) {
      if (!callback(coll->label)) goto error;
      
      // free memory as soon as possible
      if (!saveCollection(coll, CTLG|EXTR|SERV)) goto error;
      if (!diseaseCollection(coll, CTLG|EXTR|SERV|CACH)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "clientLoop fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : serverLoop
 * Description: Execute a callback function on all collections
 * Synopsis   : int serverLoop(int (*callback)(char*))
 * Input      : void (*callback)(char*): callback function
 * Output     : TRUE on success
 =======================================================================*/
int 
serverLoop(int (*callback)(Collection*))
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;

  logCommon(LOG_DEBUG, "loop on all collections (2)");

  if (!allowedUser(env.confLabel)) goto error;

  // for all collection
  if (!loadConfiguration(CFG)) goto error;
  conf = getConfiguration();
  if (conf->collections) {
    while ((coll = rgNext_r(conf->collections, &curr))) {
      if (!callback(coll)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "serverLoop fails");
  } 
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
