/*=======================================================================
 * Version: $Id: openClose.c,v 1.1 2014/10/13 19:38:58 nroche Exp $
 * Project: MediaTeX
 * Module : bus/openClose
 
 * Manage data files

 MediaTex is an Electronic Records Management System
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

#include "../misc/log.h"
#include "../misc/setuid.h"
#include "../misc/command.h"
#include "../misc/md5sum.h"
#include "../memory/confTree.h"
#include "../parser/confFile.tab.h"
#include "../parser/supportFile.tab.h"
#include "../parser/catalogFile.tab.h"
#include "../parser/extractFile.tab.h"
#include "../parser/serverFile.tab.h"
#include "../parser/recordList.tab.h"
#include "register.h"
#include "ssh.h"
#include "upgrade.h"
#include "openClose.h"

#include <avl.h>

static char* CollFiles[] = {
  "    ", "   C", "  X ", "  XC", " S  ", " S C", " SX ", " SXC",
  "5   ", "5  C", "5 X ", "5 XC", "5S  ", "5S C", "5SX ", "5SXC"
};

/*=======================================================================
 * Function   : mdtxUpdate
 * Description: call update.sh
 * Synopsis   : int mdtxUpdate(char* label)
 * Input      : char* label: the collection to update
 * Output     : TRUE on success
 =======================================================================*/
int 
callUpdate(char* user)
{ 
  int rc = FALSE;
  Configuration* conf = NULL;
  char *argv[] = {NULL, NULL, NULL};

  checkLabel(user);
  logEmit(LOG_DEBUG, "callUpdate %s", user);

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/update.sh"))) goto error;
  argv[1] = user;

  if (!env.noRegression && !env.dryRun && !env.noCollCvs) {
    if (!execScript(argv, user, NULL, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_WARNING, "%s", "callUpdate fails");
  }
  argv[0] = destroyString(argv[0]);
  return(rc);
}

/*=======================================================================
 * Function   : mdtxCommit
 * Description: call commit.sh
 * Synopsis   : int mdtxCommit(char* label)
 * Input      : char* label: the collection to commit
 * Output     : TRUE on success
 =======================================================================*/
int 
callCommit(char* user, char* signature)
{ 
  int rc = FALSE;  
  Configuration* conf = NULL;
  char *argv[] = {NULL, NULL, NULL, NULL, NULL};

  checkLabel(user);
  logEmit(LOG_DEBUG, "callCommit %s: %s", user, env.commandLine);

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/commit.sh"))) goto error;
  argv[1] = user;
  argv[2] = env.commandLine;
  argv[3] = signature;
  
  if (!env.noRegression && !env.dryRun && !env.noCollCvs) {
    if (!execScript(argv, user, NULL, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_WARNING, "%s", "callCommit fails");
  }
  argv[0] = destroyString(argv[0]);
  return(rc);
}

/*=======================================================================
 * Function   : loadConfiguration
 * Description: Call the parser on private files
 * Synopsis   : int loadConfiguration(int confFiles)
 * Input      : int confFiles: OR from ConfFile (CONF,SUPP)
 * Output     : TRUE on success
 =======================================================================*/
int loadConfiguration(int confFiles)
{
  int rc = FALSE;
  Configuration* conf = NULL;

  logEmit(LOG_DEBUG, "%s", "load configuration");
  if (!confFiles) {
    logEmit(LOG_ERR, "%s", "please provide files to load");
    goto error;
  }
  if (!(conf = getConfiguration())) goto error;

  if ((confFiles & CONF) && conf->fileState[iCONF] == DISEASED) {
    if (!parseConfiguration(conf->confFile)) goto error;
    conf->fileState[iCONF] = LOADED;
  }

  if ((confFiles & SUPP) && conf->fileState[iSUPP] == DISEASED) {
    if (!parseSupports(conf->supportDB)) goto error;
    conf->fileState[iSUPP] = LOADED;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to load configuration");   
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
  RecordTree* tree = NULL;
  RGIT* curr = NULL;
  Record* record = NULL;
  
  checkCollection(coll);
  logEmit(LOG_DEBUG, "load %s records", coll->label);
  logEmit(LOG_INFO, "records file is: %s", coll->md5sumsDB);

  if (access(coll->md5sumsDB, R_OK) == -1) {
    logEmit(LOG_NOTICE, "no md5sums file: %s", coll->md5sumsDB);
    rc = TRUE;
    goto error;
  }

  // open md5sumsDB file
  if ((fd = open(coll->md5sumsDB, O_RDONLY)) == -1) {
    logEmit(LOG_ERR, "open: %s", strerror(errno));
    logEmit(LOG_ERR, "cannot open records file: %s", coll->md5sumsDB);
    goto error;
  }
  if (!lock(fd, F_RDLCK)) goto error;

  // parse md5sumsDB file into the main recordTree
  if ((tree = parseRecordList(fd)) == NULL) goto error;

  if (!unLock(fd)) goto error;
  if (close(fd) == -1) {
    logEmit(LOG_ERR, "close: %s", strerror(errno));
    logEmit(LOG_ERR, "cannot close records file: %s", coll->md5sumsDB);
    goto error;
  }

  // index the record tree
  if (!diseaseCacheTree(coll)) goto error;
  destroyRecordTree(coll->cacheTree->recordTree);
  coll->cacheTree->recordTree = tree;
  coll->cacheTree->recordTree->messageType = DISK;
  curr = NULL;

  while((record = rgNext_r(coll->cacheTree->recordTree->records, &curr)) 
	!= NULL) {
    if (!addCacheEntry(coll, record)) goto error;
  }
   
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to load records");   
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
  logEmit(LOG_DEBUG, "file loop on %s collection (%s)", 
	  coll->label, strCF(collFiles));

  for (i=iCTLG; i<=iCACH; ++i) {
    if (collFiles & (1<<i)) {
      if (!callback(coll, i)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "file loop fails");   
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
  char* path = NULL;
  int l = 0;
  int i = 0;

  logEmit(LOG_DEBUG, "load %s files", strCF(1<<fileIdx));

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
    logEmit(LOG_ERR, "%s", "metadata file not handle"); 
    goto error;
  }

  l = strlen(path);
  if (!(path = catString(path, "00.txt"))) goto error;

  // load from part files
  do {
    if (!sprintf(path+l, "%02i.txt", i)) goto error;
    if (access(path, R_OK) != 0) break;
    if (!parser(coll, path)) goto error;
  }
  while (++i < 100);
  coll->fileState[fileIdx] = LOADED;

  // load last addon
  if (!sprintf(path+l, "%s", "NN.txt")) goto error;
  if (access(path, R_OK) == 0) {
    if (!parser(coll, path)) goto error;
    i=1;

    // cgi and server only read the meta-data
    if (!env.noCollCvs) {
      // force to re-serialize and commit it
      coll->fileState[fileIdx] = MODIFIED;
    }
  }  
  
  rc = i>0;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "loadCvFiles fails");   
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
static int 
loadColl(Collection* coll, int fileIdx)
{
  int rc = FALSE;
  int err = 0;

  checkCollection(coll);
  if (fileIdx < iCTLG || fileIdx > iCACH) goto error;
  logEmit(LOG_DEBUG, "do load %s collection (%s)", 
	  coll->label, strCF(1<<fileIdx));

  if ((err = pthread_mutex_lock(&coll->mutex[fileIdx])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
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
      coll->fileState[fileIdx] = LOADED;

      // cgi and server only read the meta-data
      if (!env.noCollCvs) {
	if (!upgradeCollection(coll)) goto error2;
	coll->fileState[iSERV] = MODIFIED;
      }

      break;
    case iCACH:
      if (!loadRecords(coll)) goto error2;	
      coll->fileState[fileIdx] = LOADED;
      break;
    default:
      goto error2;
    }
  }
  
  // in use +1
  ++coll->cptInUse[fileIdx];

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&coll->mutex[fileIdx])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "loadColl fails");   
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
  char* path = NULL;
  FILE* fd = NULL;
  int fileIdx = 0, i = 0, l = 0;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "%s", "loadCollectionNbSteps");

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
      if (!(path = catString(path, "00.txt"))) goto error;
      
      // part files
      do {
	if (!sprintf(path+l, "%02i.txt", i)) goto error;
	if (access(path, R_OK) != 0) break;
	
	// roughtly estimate the number of steps (number of lines)
	fd = fopen(path, "r");
	while (EOF != (fscanf(fd, "%*[^\n]"), fscanf(fd, "%*c")))
	  ++env.progBar.max;
	fclose(fd);
      }
      while (++i < 100);
      
      // load last addon
      if (!sprintf(path+l, "%s", "NN.txt")) goto error;
      if (access(path, R_OK) == 0) {
	fd = fopen(path, "r");
	while (EOF != (fscanf(fd, "%*[^\n]"), fscanf(fd, "%*c")))
	  ++env.progBar.max;
	fclose(fd);
      }
      path = destroyString(path);
    }    
  }

  logEmit(LOG_INFO, "estimate %i steps for load", env.progBar.max);
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "loadCollectionNbSteps fails");   
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
  int doProgBar = FALSE;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "load %s collection (%s)", 
	  coll->label, strCF(collFiles));

  if (!expandCollection(coll)) goto error;

  if (!env.noCollCvs) {
    if (coll->toUpdate && collFiles & (CTLG | EXTR | SERV)) {
      if (callUpdate(coll->user)) coll->toUpdate = FALSE;
    }
  }

  // only progbar for client (check logs facility)
  doProgBar = (!env.noRegression && !strncmp(env.logFacility, "file", 4));

  if (doProgBar) {
    if (!loadCollectionNbSteps(coll, collFiles)) goto error;
    startProgBar("load");
  }

  if (!collectionLoop(coll, collFiles, loadColl)) goto error;

  if (doProgBar) stopProgBar();

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to load collection");   
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
static int 
wasModifiedColl(Collection* coll, int i)
{
  int rc = FALSE;
  int err = 0;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "do set %s collection as modified (%s)", 
	  coll->label, strCF(1<<i));

  if ((err = pthread_mutex_lock(&coll->mutex[i])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  
  coll->fileState[i] = MODIFIED;
  
  if ((err = pthread_mutex_unlock(&coll->mutex[i])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to set collection as modified");
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
  logEmit(LOG_DEBUG, "set %s collection as modified (%s)", 
	  coll->label, strCF(collFiles));

  if (!loadCollection(coll, collFiles)) goto error;
  if (!collectionLoop(coll, collFiles, wasModifiedColl)) goto error2;

  rc = TRUE;
 error2:
  releaseCollection(coll, collFiles);
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to set collection as modified");
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
  logEmit(LOG_DEBUG, "do release %s collection (%s)", 
	  coll->label, strCF(1<<i));

  if ((err = pthread_mutex_lock(&coll->mutex[i])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }
  
  // in use -1
  --coll->cptInUse[i];

  // assert cpt >= 0
  if (coll->cptInUse[i] < 0) {
    logEmit(LOG_WARNING, "cptInUse for %s = %i !", 
	    strCF(1<<i), coll->cptInUse[i]);
  }

  if ((err = pthread_mutex_unlock(&coll->mutex[i])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to do release collection");
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
  logEmit(LOG_DEBUG, "release %s collection (%s)", 
	  coll->label, strCF(collFiles));

 if (!collectionLoop(coll, collFiles, releaseColl)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to release collection");
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
  Configuration* conf = NULL;
  int change = FALSE;

  logEmit(LOG_DEBUG, "%s", "save configuration");
  if (!(conf = getConfiguration())) goto error;

  if (conf->fileState[iCONF] == MODIFIED) {
    if (!expandConfiguration()) goto error;
    if (!populateConfiguration()) goto error;
    if (!serializeConfiguration(conf)) goto error;
    conf->fileState[iCONF] = LOADED;
    change = TRUE;
    conf->toHup = TRUE;
  }

  if (conf->fileState[iSUPP] == MODIFIED) {
    if (!serializeSupports()) goto error;
    conf->fileState[iSUPP] = LOADED;
    change = TRUE;
  }

  // commit changes
  if (change && !env.noCollCvs) {
    callCommit(env.confLabel, NULL);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to save configuration");   
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

  checkCollection(coll);
  logEmit(LOG_DEBUG, "do save %s collection (%s)", 
	  coll->label, strCF(1<<i));

  if ((err = pthread_mutex_lock(&coll->mutex[i])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
    goto error;
  }

  // save if modifyed and no more used
  if (coll->fileState[i] == MODIFIED && coll->cptInUse[i] == 0) {
    switch (i) {
    case iCTLG:
      if (!serializeCatalogTree(coll)) goto error2;
      coll->toCommit = TRUE;
      break;
    case iEXTR:
      if (!serializeExtractTree(coll)) goto error2;
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
			       coll->md5sumsDB, NULL)) goto error2;
      if (!unLockCache(coll)) goto error2;
      break;
    default:
      goto error2;
    }
    coll->fileState[i] = LOADED;
  }
  else {
    logEmit(LOG_INFO, "%s", "do not save collection as...");
    if (coll->fileState[i] != MODIFIED) {
      logEmit(LOG_INFO, "%s", "... not modified");
    }
    if (coll->cptInUse[i]) {
      logEmit(LOG_INFO, "... still used by %i functions", 
	      coll->cptInUse[i]);
    }
  }

  rc = TRUE;
 error2:
  if ((err = pthread_mutex_unlock(&coll->mutex[i])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to save collection");
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
  logEmit(LOG_DEBUG, "%s", "saveCollectionNbSteps");

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

  logEmit(LOG_INFO, "estimate %i steps for save", env.progBar.max);
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "saveCollectionNbSteps fails");   
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
  Configuration* conf = NULL;
  int doProgBar = FALSE;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "save %s collection (%s)", 
	  coll->label, strCF(collFiles));

  if (!(conf = getConfiguration())) goto error;

  // only progbar for client (check logs facility)
  doProgBar = (!env.noRegression && !strncmp(env.logFacility, "file", 4));

  if (doProgBar) {
    if (!saveCollectionNbSteps(coll, collFiles)) goto error;
    startProgBar("save");
  }

  if (!collectionLoop(coll, collFiles, saveColl)) goto error;

  if (doProgBar) stopProgBar();

  // commit changes
  if (coll->toCommit && !env.noCollCvs) {
    if (callCommit(coll->user, coll->userFingerPrint)) {
      conf->toHup = TRUE;
      coll->toCommit = FALSE;
      coll->toUpdate = TRUE;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to save collection");
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
  Configuration* conf = NULL;
  Collection* coll = NULL;
  RGIT* curr = NULL;
  
  logEmit(LOG_DEBUG, "%s", "clientSaveAll");
  if (!(conf = env.confTree)) goto end;
  if (!saveConfiguration(env.commandLine)) goto error;

  while ((coll = rgNext_r(conf->collections, &curr)) != NULL) {
    if (!saveCollection(coll, CTLG|EXTR|SERV)) goto error;
  }

  // tell the server we have upgrade files
  if (!env.noRegression && !env.dryRun && conf->toHup) {
    mdtxAsyncSignal(0); // send HUP signal to daemon
    conf->toHup = FALSE;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "clientSaveAll fails");
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
  Configuration* conf = NULL;
  Collection* coll = NULL;
  RGIT* curr = NULL;
  
  logEmit(LOG_DEBUG, "%s", "server save all");
  if (!(conf = env.confTree)) goto end;

  while ((coll = rgNext_r(conf->collections, &curr)) != NULL) {
    if (!saveCollection(coll, CACH)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "server fails to save all");
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
static int 
diseaseColl(Collection* coll, int i)
{
  int rc = FALSE;
  int err = 0;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "disease %s collection (%s)", 
	  coll->label, strCF(1<<i));

  if ((err = pthread_mutex_lock(&coll->mutex[i])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_lock fails: %s", strerror(err));
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

  rc = TRUE; 
 error2:
  if ((err = pthread_mutex_unlock(&coll->mutex[i])) != 0) {
    logEmit(LOG_ERR, "pthread_mutex_unlock fails: %s", strerror(err));
    rc = FALSE;
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to disease collection");
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
  logEmit(LOG_DEBUG, "disease %s collection (%s)", 
	  coll->label, strCF(collFiles));

  if (!env.noRegression) memoryStatus(LOG_NOTICE);
  if (!collectionLoop(coll, collFiles, diseaseColl)) goto error;
  if (!env.noRegression) memoryStatus(LOG_NOTICE);

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to disease collection");
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
  Configuration* conf = NULL;
  Collection* coll = NULL;
  RGIT* curr = NULL;
  
  logEmit(LOG_DEBUG, "%s", "clientDiseaseAll");
  if (!(conf = env.confTree)) goto error; // do not malloc

  if ((coll = rgNext_r(conf->collections, &curr)) != NULL) {
    if (!saveCollection(coll, CTLG|EXTR|SERV)) goto error;
    if (!diseaseCollection(coll, CTLG|EXTR|SERV|CACH)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "clientDiseaseAll fails");
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
  Configuration* conf = NULL;
  Collection* coll = NULL;
  RGIT* curr = NULL;
  
  logEmit(LOG_DEBUG, "%s", "serverDiseaseAll");
  if (!(conf = env.confTree)) goto error; // do not malloc

  if ((coll = rgNext_r(conf->collections, &curr)) != NULL) {
    if (!saveCollection(coll, CACH)) goto error;
    if (!diseaseCollection(coll, CTLG|EXTR|SERV|CACH)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serverDiseaseAll fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxGetCollection
 * Description: Call serializer on all modified files
 * Synopsis   : Collection* mdtxGetCollection(char* label)
 * Input      : char* label: collection label to search for
 * Output     : Collection*: the matching collection ; NULL if not found
 =======================================================================*/
Collection* mdtxGetCollection(char* label)
{
  Collection* rc = NULL;
  Collection* coll = NULL;

  checkLabel(label);  
  logEmit(LOG_DEBUG, "get %s collection", label);

  if (!loadConfiguration(CONF)) goto error;
  if (!(coll = getCollection(label))) goto error;
  if (!expandCollection(coll)) goto error;

  rc = coll;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to get collection");
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxGetSupport
 * Description: Call serializer on all modified files
 * Synopsis   : Support* mdtxGetSupport(char* label)
 * Input      : char* label: support label to search for
 * Output     : Support*: the matching support ; NULL if not found
 =======================================================================*/
Support* mdtxGetSupport(char* label)
{
  Support* rc = NULL;
  Support* supp = NULL;

  checkLabel(label);  
  logEmit(LOG_DEBUG, "get %s support", label);

  if (!allowedUser(env.confLabel)) goto error;
  if (!loadConfiguration(SUPP)) goto error;
  if (!(supp = getSupport(label))) goto error;

  rc = supp;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to get support");
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
  Configuration* conf = NULL;
  Collection* coll = NULL;
  RGIT* curr = NULL;

  logEmit(LOG_DEBUG, "%s", "loop on all collections");

  if (!allowedUser(env.confLabel)) goto error;

  // for all collection
  if (!loadConfiguration(CONF)) goto error;
  conf = getConfiguration();
  if (conf->collections != NULL) {
    while((coll = rgNext_r(conf->collections, &curr)) != NULL) {
      if (!callback(coll->label)) goto error;
      
      // free memory as soon as possible
      if (!saveCollection(coll, CTLG|EXTR|SERV)) goto error;
      if (!diseaseCollection(coll, CTLG|EXTR|SERV|CACH)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_INFO, "%s", "clientLoop fails");
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
  Configuration* conf = NULL;
  Collection* coll = NULL;
  RGIT* curr = NULL;

  logEmit(LOG_DEBUG, "%s", "loop on all collections (2)");

  if (!allowedUser(env.confLabel)) goto error;

  // for all collection
  if (!loadConfiguration(CONF)) goto error;
  conf = getConfiguration();
  if (conf->collections != NULL) {
    while((coll = rgNext_r(conf->collections, &curr)) != NULL) {
      if (!callback(coll)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_INFO, "%s", "serverLoop fails");
  } 
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
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
  //fprintf(stderr, "\n\t\t");

  mdtxOptions();
  //fprintf(stderr, "  ---\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for open/close module.
 * Synopsis   : ./openClose
 * Input      : 
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = NULL;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!mdtxGetSupport("SUPP11_logo.png")) goto error;
  if (!(coll = mdtxGetCollection("coll1"))) goto error;
  if (!loadCollection(coll, CTLG|EXTR|SERV|CACH)) goto error;
  if (!releaseCollection(coll, CTLG|EXTR|SERV|CACH)) goto error;
  if (!diseaseCollection(coll, CTLG|EXTR|SERV|CACH)) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  freeConfiguration();
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
