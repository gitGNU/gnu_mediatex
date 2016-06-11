/*=======================================================================
 * Project: MediaTeX
 * Module : utfunc
 *
 * Functions only used by unit tests

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

#include "mediatex.h"
#include "server/utFunc.h"

static LogSeverity* logSeverityBackup[LOG_MAX_MODULE];

/*=======================================================================
 * Function   : utHideLogs
 * Description: disable logs
 =======================================================================*/
void utHideLogs()
{
  int i = 0;
  
  for (i=0; i<LOG_MAX_MODULE; ++i){
    logSeverityBackup[i] = env.logHandler->severity[i];
    
    if (i == LOG_MAIN) {
      env.logHandler->severity[i] = &LogSeverities[5]; // notice
    }
    else {
      env.logHandler->severity[i] = &LogSeverities[3]; // err
    }
  }
}

/*=======================================================================
 * Function   : utHideLogs
 * Description: re-enable logs
 =======================================================================*/
void utRestoreLogs()
{
  int i = 0;
  
  for (i=0; i<LOG_MAX_MODULE; ++i){
    env.logHandler->severity[i] = logSeverityBackup[i];
  }
}

/*=======================================================================
 * Function   : utCleanCache
 * Description: Clean the physical caches
 =======================================================================*/
int 
utCleanCaches(void)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;
  char* argv[] = {"/bin/bash", "-c", 0, 0};
  char* cmd = "rm -fr ";

  logMain(LOG_NOTICE, "clean cache for all collections");

  // hide the logs
  utHideLogs();

  // for all collection
  if (!loadConfiguration(CFG)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!conf->collections) goto error;
  while ((coll = rgNext_r(conf->collections, &curr))) {

    // set directory paths
    if (!expandCollection(coll)) goto error;

    // clean cache dir
    if (!(argv[2] = createString(cmd))) goto error;
    if (!(argv[2] = catString(argv[2], coll->cacheDir))) goto error;
    if (!(argv[2] = catString(argv[2], "/*"))) goto error;
    if (!execScript(argv, 0, 0, FALSE)) goto error;
    argv[2] = destroyString(argv[2]);

    // clean extraction dir
    if (!(argv[2] = createString(cmd))) goto error;
    if (!(argv[2] = catString(argv[2], coll->extractDir))) goto error;
    if (!(argv[2] = catString(argv[2], "/*"))) goto error;
    if (!execScript(argv, 0, 0, FALSE)) goto error;
    argv[2] = destroyString(argv[2]);

    // empty the cache
    if (!diseaseCacheTree(coll)) goto error;
    
    // hide score computation
    if (!computeExtractScore(coll)) goto error;
  }

  // restore the logs
  utRestoreLogs();

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to clean the cache");
  } 
  argv[2] = destroyString(argv[2]);
  return rc;
}

/*=======================================================================
 * Function   : utCopyFileOnCache
 * Description: Copy a file to the physical cache
 * Input      : Collection* coll : to know the collection's cache to use
 *              char* srcdir : the current dir in use by autotools
 *              char* srcfile : the file basename into misc dir
 * Output     : TRUE on success
 * Note       : stars do not works as we do not use bash context
 =======================================================================*/
int 
utCopyFileOnCache(Collection* coll, char* srcdir, char* srcfile)
{
  int rc = FALSE;
  char* argv[] = {"/bin/cp", "-f", 0, 0, 0};

  utHideLogs();
  logMain(LOG_NOTICE, "copy %s into the %s cache", srcfile, coll->label);

  if (!(argv[2] = createString(srcdir))) goto error;
  if (!(argv[2] = catString(argv[2], "/../misc/"))) goto error;
  if (!(argv[2] = catString(argv[2], srcfile))) goto error;

  argv[3] = coll->cacheDir;
  if (!execScript(argv, 0, 0, FALSE)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to copy %s to the %s cache",
	    srcfile, coll->label);
  } 
  if (argv[2]) destroyString(argv[2]);
  utRestoreLogs();
  return rc;
}

/*=======================================================================
 * Function   : utRemoteDemand
 * Description: build a record on logo using a given server
 =======================================================================*/
Record*
utRemoteDemand(Collection* coll, Server* server)
{
  Record* rc = 0;
  Record *record = 0;
  Archive* archive = 0;
  char* string = 0;
 
  checkCollection(coll);
  checkServer(server);

  if (!(string = createString("!wanted"))) goto error;
  if (!(archive = addArchive(coll,
			     "022a34b2f9b893fba5774237e1aa80ea", 24075)))
    goto error;
  if (!(record = addRecord(coll, server, archive, DEMAND, string)))
    goto error;

  archive = 0;
  rc = record;
 error:
  if (!rc) {
    logMain(LOG_ERR, "utRemoteDemand fails");
  }
  destroyArchive(archive);
  return rc;
}

/*=======================================================================
 * Function   : utLocalRecord
 * Description: Build a record using localhost
 =======================================================================*/
Record*
utLocalRecord(Collection* coll, char* hash, off_t size, 
	      Type type, char* extra)
{
  Record* rc = 0;
  Archive* archive = 0;
  Record* record = 0;
 
  checkCollection(coll);
  if (!getLocalHost(coll)) goto error;

  if (!(archive = addArchive(coll, hash, size))) goto error;
  if (!(record = addRecord(coll, coll->localhost, archive, type, extra)))
    goto error;

  archive = 0;
  rc = record;
 error:
  if (!rc) {
    logMain(LOG_ERR, "utLocalRecord fails");
  }
  destroyArchive(archive);
  return rc;
}

/*=======================================================================
 * Function   : utAddFinalDemand
 * Description: Add a final demand on logo
 =======================================================================*/
int
utAddFinalDemand(Collection* coll)
{
  int rc = FALSE;
  Record *record = 0;
  char* extra = 0;
 
  checkCollection(coll);

  if (!(extra = createString("test@test.com"))) goto error;
  if (!(record = utLocalRecord(coll, 
			       "022a34b2f9b893fba5774237e1aa80ea", 24075,
			       DEMAND, extra))) goto error;
  if (!addCacheEntry(coll, record)) goto error;
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "utAddFinalDemand fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : utAddLocalDemand
 * Description: add a local demand
 =======================================================================*/
int
utAddLocalDemand(Collection* coll, char* hash, off_t size, char* extra)
{
  int rc = FALSE;
  Record *record = 0;
  char* string = 0;
 
  checkCollection(coll);

  if (!(string = createString(extra))) goto error;
  if (!(record = utLocalRecord(coll, hash, size, DEMAND, string))) 
    goto error;
  if (!addCacheEntry(coll, record)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "utAddLocalDemand fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : UtConnexion
 * Description: Simulate an empty message get by the server
 =======================================================================*/
Connexion* 
utConnexion(Collection* coll, MessageType messageType, Server* from)
{
  Connexion* rc = 0;
  RecordTree* tree = 0;
  
  if (!(tree = createRecordTree())) goto error;

  if (!(rc = (Connexion*)malloc(sizeof(Connexion)))) {
    logMain(LOG_ERR, "malloc cannot create connexion objet: %s", 
	    strerror(errno));
    goto error;
  }

  memset(rc, 0, sizeof (struct Connexion));  
  rc->server = from;
  rc->message = tree;
  rc->message->collection = coll;
  rc->message->messageType = messageType;
  strncpy(rc->message->fingerPrint, from->fingerPrint, MAX_SIZE_MD5);

 error:
  if (!rc) {
    logMain(LOG_ERR, "utConnexion fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : utUpoadMessage
 * Description: Build a UPLOAD query for misc/README file 
 *              (extra as parameter)
 =======================================================================*/
Connexion*
utUploadMessage(Collection* coll, char* extra)
{
  Connexion* rc = 0;
  Connexion* con = 0;
  Server* localhost = 0;
  Record *record = 0;
  char* string = 0;

  if (!(string = createString(extra))) goto error;
  if (!(localhost = getLocalHost(coll))) goto error;
  if (!(con = utConnexion(coll, UPLOAD, localhost))) goto error;
  if (!(record = utLocalRecord(coll, 
			       "3f18841537668dcf4fafd1471c64d52d", 1937,
			       SUPPLY, string)))
    goto error;
  string = 0;
  
  if (!avl_insert(con->message->records, record)) {
    logMain(LOG_ERR, "cannot add record (already there?)");
    goto error;
  }
  record = 0;

  rc = con;
  con = 0;
 error:
  if (!rc) {
    logMain(LOG_ERR, "utUploadMessage fails");
  }
  destroyString(string);
  destroyRecord(record);
  if (con) destroyRecordTree(con->message);
  free (con);
  return rc;
}

/*=======================================================================
 * Function   : utCgiMessage
 * Description: Build a CGI query for logo (providing mail or not)
 =======================================================================*/
Connexion*
utCgiMessage(Collection* coll, char* mail)
{
  Connexion* rc = 0;
  Connexion* con = 0;
  Server* localhost = 0;
  Record *record = 0;
  char wanted[] = "!wanted";
  char* string = 0;

  if (!(string = createString(mail?mail:wanted))) goto error;
  if (!(localhost = getLocalHost(coll))) goto error;
  if (!(con = utConnexion(coll, CGI, localhost))) goto error;
  if (!(record = utLocalRecord(coll, 
			       "022a34b2f9b893fba5774237e1aa80ea", 24075,
			       DEMAND, string)))
    goto error;
  string = 0;

  if (!avl_insert(con->message->records, record)) {
    logMain(LOG_ERR, "cannot add record (already there?)");
    goto error;
  }
  record = 0;

  rc = con;
  con = 0;
 error:
  if (!rc) {
    logMain(LOG_ERR, "utCgiMessage fails");
  }
  destroyString(string);
  destroyRecord(record);
  if (con) destroyRecordTree(con->message);
  free (con);
  return rc;
}

/*=======================================================================
 * Function   : utHaveMessage1
 * Description: Build a HAVE query providing part1 (extra as parameter)
 =======================================================================*/
Connexion*
utHaveMessage1(Collection* coll, char* extra)
{
  Connexion* rc = 0;
  Connexion* con = 0;
  Server* localhost = 0;
  Record *record = 0;
  char* string = 0;

  if (!(string = createString(extra))) goto error;
  if (!(localhost = getLocalHost(coll))) goto error;
  if (!(con = utConnexion(coll, HAVE, localhost))) goto error;
  if (!(record = utLocalRecord(coll, 
			       "1a167d608e76a6a4a8b16d168580873c", 20480,
			       SUPPLY, string)))
    goto error;
  string = 0;

  if (!avl_insert(con->message->records, record)) {
    logMain(LOG_ERR, "cannot add record (already there?)");
    goto error;
  }
  record = 0;

  rc = con;
  con = 0;
 error:
  if (!rc) {
    logMain(LOG_ERR, "utHaveMessage1 fails");
  }
  destroyString(string);
  destroyRecord(record);
  if (con) destroyRecordTree(con->message);
  free (con);
  return rc;
}

/*=======================================================================
 * Function   : utHaveMessage2
 * Description: Build a HAVE query providing part2 (extra as parameter)
 =======================================================================*/
Connexion*
utHaveMessage2(Collection* coll, char* extra)
{
  Connexion* rc = 0;
  Connexion* con = 0;
  Server* localhost = 0;
  Record *record = 0;
  char* string = 0;

  if (!(string = createString(extra))) goto error;
  if (!(localhost = getLocalHost(coll))) goto error;
  if (!(con = utConnexion(coll, HAVE, localhost))) goto error;
  if (!(record = utLocalRecord(coll, 
			       "c0c055a0829982bd646e2fafff01aaa6", 4066, 
			       SUPPLY, string)))
    goto error;
  string = 0;

  if (!avl_insert(con->message->records, record)) {
    logMain(LOG_ERR, "cannot add record (already there?)");
    goto error;
  }
  record = 0;

  rc = con;
  con = 0;
 error:
  if (!rc) {
    logMain(LOG_ERR, "utHaveMessage2 fails");
  }
  destroyString(string);
  destroyRecord(record);
  if (con) destroyRecordTree(con->message);
  free (con);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
