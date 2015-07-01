/*=======================================================================
 * Version: $Id: utFunc.c,v 1.1 2015/07/01 10:50:08 nroche Exp $
 * Project: MediaTeX
 * Module : utfunc
 *
 * Functions only used by unit tests

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

#include "mediatex.h"
#include "server/utFunc.h"

/*=======================================================================
 * Function   : utCleanCache
 * Description: clean the physical caches
 * Synopsis   : int utCleanCache(void)
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
utCleanCaches(void)
{
  int rc = FALSE;
 
  logEmit(LOG_NOTICE, "%s", "clean the cache");
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;
  char* argv[] = {"/bin/bash", "-c", 0, 0};
  char* cmd = "rm -fr ";

  logEmit(LOG_NOTICE, "%s", "clean cache for all collections");

  // for all collection
  if (!loadConfiguration(CFG)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!conf->collections) goto error;
  while((coll = rgNext_r(conf->collections, &curr))) {
    if (!diseaseCacheTree(coll)) goto error;
    if (!(argv[2] = createString(cmd))) goto error;
    if (!(argv[2] = catString(argv[2], coll->cacheDir))) goto error;
    if (!(argv[2] = catString(argv[2], "/*"))) goto error;
    if (!execScript(argv, 0, 0, FALSE)) goto error;
    argv[2] = destroyString(argv[2]);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to clean the cache");
  } 
  argv[2] = destroyString(argv[2]);
  return rc;
}

/*=======================================================================
 * Function   : utCopyFileOnCache
 * Description: copy a file to the physical cache
 * Synopsis   : int utCopyFileOnCache(Collection* coll, char* file)
 * Input      : Collection* coll : to know the collection's cache to use
 *              char* srcdir : the current dir in use by autotools
 *              char* file : the file basename
 * Output     : TRUE on success
 * Note       : stars do not works as we do not use bash context
 =======================================================================*/
int 
utCopyFileOnCache(Collection* coll, char* srcdir, char* srcfile)
{
  int rc = FALSE;

  char* argv[] = {"/bin/cp", "-f", 0, 0, 0};

  logEmit(LOG_NOTICE, "copy %s to the %s cache", srcfile, coll->label);

  if (!(argv[2] = createString(srcdir))) goto error;
  if (!(argv[2] = catString(argv[2], "/../misc/"))) goto error;
  if (!(argv[2] = catString(argv[2], srcfile))) goto error;

  argv[3] = coll->cacheDir;
  if (!execScript(argv, 0, 0, FALSE)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "fails to copy %s to the %s cache",
	    srcfile, coll->label);
  } 
  if (argv[2]) destroyString(argv[2]);
  return rc;
}

/*=======================================================================
 * Function   : utNewRecord
 * Description: add a demand
 * Synopsis   : int utNewRecord(Collection* coll, char* hash, off_t size)
 * Input      : Collection* coll : to know the collection's cache to use
 *              char* hash: md5sum file's hash
 *              off_t size: file's size
 * Output     : TRUE on success
 =======================================================================*/
Record*
utNewRecord(Collection* coll, char* hash, off_t size, 
	    Type type, char* extra)
{
  Record* rc = 0;
  Archive* archive = 0;
  Record *record = 0;
 
  logEmit(LOG_NOTICE, "add a generic demand for", hash);
  checkCollection(coll);

  // assert we have the localhost server object
  if (!getLocalHost(coll)) goto error;

  if (!(archive = addArchive(coll, hash, size))) goto error;
  if (!(record = addRecord(coll, coll->localhost, archive, type, extra)))
    goto error;

  rc = record;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "fails to add a generic demand for", hash);
  }
  return rc;
}

/*=======================================================================
 * Function   : utDemand
 * Description: add a demand
 * Synopsis   : int utToKeep(Collection* coll, char* hash, off_t size)
 * Input      : Collection* coll : to know the collection's cache to use
 *              char* hash: md5sum file's hash
 *              off_t size: file's size
 * Output     : TRUE on success
 =======================================================================*/
int
utDemand(Collection* coll, char* hash, off_t size, char* mail)
{
  int rc = FALSE;
  Record *record = 0;
  char* extra = 0;
 
  logEmit(LOG_NOTICE, "ask for %s file", hash);
  checkCollection(coll);

  if (!(extra = createString(mail))) goto error;
  if (!(record = utNewRecord(coll, hash, size, DEMAND, extra))) goto error;
  if (!addCacheEntry(coll, record)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "fails to ask for %s file", hash);
  }
  return rc;
}

/*=======================================================================
 * Function   : ask4logo
 * Description: Build a simple CGI client query
 * Synopsis   : RecordTree* ask4logo()
 * Input      : N/A
 * Output     : RecordTree* = what the CGI may ask to our server
 =======================================================================*/
RecordTree* ask4logo(Collection* coll, char* mail)
{
  RecordTree *rc = 0;
  Record *record = 0;
  RecordTree *tree = 0;
  char* extra = 0;

  logEmit(LOG_NOTICE, "%s", "ask for logo.png");

  if (mail != 0) {
    if (!(extra = createString(mail))) goto error;
  }
  else {
    if (!(extra = createString("!wanted"))) goto error;
  }

  if (!(record = 
       utNewRecord(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075, 
		   DEMAND, extra)))
    goto error;

  if ((tree = createRecordTree()) == 0) goto error;
  tree->collection = coll;
  tree->messageType = CGI;
  strncpy(tree->fingerPrint, coll->userFingerPrint, MAX_SIZE_HASH);

  if (!rgInsert(tree->records, record)) goto error;
  record = 0;

  rc = tree;
  tree = 0;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to ask for logo.png");
  }
  tree = destroyRecordTree(tree);
  record = destroyRecord(record);
  return rc;
}

/*=======================================================================
 * Function   : providePart1
 * Description: Build a simple HAVE query
 * Synopsis   : RecordTree* 
 * Input      : N/A
 * Output     : RecordTree* = what the mdtx client may ask to our server
 =======================================================================*/
RecordTree* providePart1(Collection* coll, char* path)
{
  RecordTree *rc = 0;
  Record *record = 0;
  RecordTree *tree = 0;
  char* extra = 0;

  logEmit(LOG_NOTICE, "%s", "provide logoP1.cat");

  if (!(extra = createString(path))) goto error;
  if (!(record = 
	utNewRecord(coll, "1a167d608e76a6a4a8b16d168580873c", 20480, 
		   SUPPLY, extra))) goto error;

  if ((tree = createRecordTree()) == 0) goto error;
  tree->collection = coll;
  tree->messageType = HAVE;
  strncpy(tree->fingerPrint, coll->userFingerPrint, MAX_SIZE_HASH);

  if (!rgInsert(tree->records, record)) goto error;
  record = 0;


  rc = tree;
  tree = 0;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to provide logoP1.cat");
  }
  tree = destroyRecordTree(tree);
  record = destroyRecord(record);
  return rc;
}

/*=======================================================================
 * Function   : providePart2
 * Description: Build a simple HAVE query
 * Synopsis   : RecordTree* 
 * Input      : N/A
 * Output     : RecordTree* = what the mdtx client may ask to our server
 =======================================================================*/
RecordTree* providePart2(Collection* coll, char* path)
{
  RecordTree *rc = 0;
  Record *record = 0;
  RecordTree *tree = 0;
  char* extra = 0;
  
  logEmit(LOG_NOTICE, "%s", "provide logoP2.cat");

  if (!(extra = createString(path))) goto error;
  if (!(record = 
       utNewRecord(coll, "c0c055a0829982bd646e2fafff01aaa6", 4066, 
		   SUPPLY, extra)))
    goto error;

  if ((tree = createRecordTree()) == 0) goto error;
  tree->collection = coll;
  tree->messageType = HAVE;
  strncpy(tree->fingerPrint, coll->userFingerPrint, MAX_SIZE_HASH);

  if (!rgInsert(tree->records, record)) goto error;
  record = 0;

  rc = tree;
  tree = 0;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to provide logoP2.cat");
  }
  tree = destroyRecordTree(tree);
  record = destroyRecord(record);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
