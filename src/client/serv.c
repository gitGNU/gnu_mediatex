/*=======================================================================
 * Project: MediaTeX
 * Module : serv
 *
 * Manage servers.txt modifications

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
#include <sys/sem.h>

typedef union semun {
  int val;
  struct semid_ds *buffer;
  unsigned short int *table;
} semun_t;

/*=======================================================================
 * Function   : setConcurentAccessLock
 * Description: Set the concurent access lock (top level lock)
 * Synopsis   : static int setConcurentAccessLock()
 * Input      : N/A
 * Output     : TRUE on success
 * Note       : try ipcs if dead-locked
 =======================================================================*/
static int setConcurentAccessLock()
{
  Configuration* conf = 0;
  int rc = FALSE;
  key_t key;
  semun_t u_semun;
  unsigned short table[1];
  int uid = getuid();

  logMain(LOG_DEBUG, "initialise the concurrent access"); 

  if (!(conf = getConfiguration())) goto error;
  if (conf->sem) goto end;
  
  // force becoming mdtx user 
  if (!becomeUser(env.confLabel, FALSE)) goto error;
  
  // Create a new IPC key base on an i-node + minor peripherical
  // number and a project number
  if ((key = ftok(conf->confFile, COMMON_OPEN_CLOSE_PROJECT_ID)) == -1) {
    logMain(LOG_ERR, "ftok fails to create IPC key: %s", strerror(errno)); 
    logMain(LOG_NOTICE, "ftok using path: %s", conf->confFile); 
    goto error;
  }

  logMain(LOG_DEBUG, "client ipc key: {%s, %i}", 
	  conf->confFile, COMMON_OPEN_CLOSE_PROJECT_ID); 
  logMain(LOG_DEBUG, "client ipc key: 0x%x", key); 

  // Try to match an existing semaphore with the key
  if (( conf->sem = semget(key, 1, 0)) == -1) {

    // Create a new semaphore matched with the key
    if ((conf->sem = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600)) == -1) {
      logMain(LOG_ERR, "semget fails matching 0x%x IPC key: %s", 
	      strerror(errno)); 
      logMain(LOG_NOTICE, "(semaphore as gone)");
      goto error;
    }
  
    // Initialize the semaphore(s) values
    table[0] = 1;
    u_semun.table = table;
    if (semctl(conf->sem, 0, SETALL, u_semun) <0) {
      logMain(LOG_ERR, "semctl fails to initialize semaphore value: %s", 
	      strerror(errno)); 
      goto error;
    }
  }

 end:
  rc = TRUE;
 error:
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logMain(LOG_ERR, "fails to set concurrent access lock"); 
  }
  return rc;
}

/*=======================================================================
 * Function   : clientWriteLock
 * Description: Get the concurent access lock (top level lock)
 * Synopsis   : int clientWriteLock()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int clientWriteLock()
{
  int rc = FALSE;
  Configuration* conf = 0;
  struct sembuf sembuf;
  int uid = getuid();

  logMain(LOG_DEBUG, "get the concurrent access"); 

  if (!(conf = getConfiguration())) goto error;
  if (!setConcurentAccessLock()) goto error;
  if (conf->sem == 0) {
    logMain(LOG_ERR, "concurrent access lock is not initialized"); 
    goto error;
  }

  // force becoming mdtx user 
  if (!becomeUser(env.confLabel, FALSE)) goto error;

  // P (may I ?)
  logMain(LOG_DEBUG, "get writter lock");
  sembuf.sem_num = 0;
  sembuf.sem_op = -1;
  sembuf.sem_flg = SEM_UNDO | IPC_NOWAIT; // for crash recovery...
  //... + do not asleep process if not available but exit with EAGAIN
  if (semop(conf->sem, &sembuf, 1) < 0) {
    if (errno == EAGAIN) {
      logMain(LOG_ERR, 
	      "cannot take concurent access as it is already locked");
    }
    else {
      logMain(LOG_ERR, "semop fails asking for semaphore: %s", 
	      strerror(errno)); 
    }
    goto error;
  }

  // Parse files
  
  rc = TRUE;
 error:
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logMain(LOG_ERR, "fails to get writter lock"); 
  }
  return rc;
}

/*=======================================================================
 * Function   : clientWriteUnlock
 * Description: Release the concurent access lock (top level lock)
 * Synopsis   : int clientWriteUnlock()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int clientWriteUnlock()
{
  int rc = FALSE;
  Configuration* conf = 0;
  union semun arg;
  int uid = getuid();

  logMain(LOG_DEBUG, "release the concurrent access"); 

  if (!(conf = getConfiguration())) goto error;
  if (conf->sem == 0) {
    logMain(LOG_ERR, "concurrent access lock already unlock"); 
    goto error;
  }

  // force becoming mdtx user 
  if (!becomeUser(env.confLabel, FALSE)) goto error;
  
  /*
    As concurent process exit without waiting for the mutex, 
    we do not need to release first and the free if no more used
  */

  // Free semaphore IPC ressource
  arg.val = 0;
  if (semctl(conf->sem, 0, IPC_RMID, arg) == -1) {
    logMain(LOG_ERR, "semctl fails: %s", strerror(errno)); 
    goto error;
  }

  conf->sem = 0;
  rc = TRUE;
 error:
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logMain(LOG_ERR, "fails to release the writter lock"); 
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxUpdate
 * Description: indirectely call pull.sh
 * Synopsis   : int mdtxUpdate(char* label)
 * Input      : char* label: the collection to pull
 * Output     : TRUE on success

 * Note       : workaround as it should be implicitly done 
 *              (see common/openClose.h::loadCollection)
 =======================================================================*/
int
mdtxUpdate(char* label)
{
  int rc = FALSE;
  Collection* coll = 0;

  checkLabel(label);
  logMain(LOG_DEBUG, "update collection");

  if (!(coll = mdtxGetCollection(label))) goto error;
  if (!callCommit(coll->user, "Manual user edition")) goto error;
  if (!callPull(coll->user)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to update collection");
  }
  return(rc);
}

/*=======================================================================
 * Function   : mdtxCommit
 * Description: indirectely call push.sh
 * Synopsis   : int mdtxCommit(char* label)
 * Input      : char* label: the collection to push
 * Output     : TRUE on success

 * Note       : workaround as it should be implicitly done 
 *              (see common/openClose.h::loadCollection)
 =======================================================================*/
int
mdtxCommit(char* label)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;

  checkLabel(label);
  logMain(LOG_DEBUG, "commit collection");

  if (!(conf = getConfiguration())) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;
  if (!callCommit(coll->user, 0)) goto error;
  if (!callPush(coll->user)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to commit collection");
  }
  return(rc);
}


/*=======================================================================
 * Function   : mdtxUpgrade
 * Description: Upgrade the DB by serializing it
 * Synopsis   : int mdtxUpgrade(char* label)
 * Input      : char* label = collection to upgrade
 * Output     : TRUE on success

 * Note       : This fonction has to be used to rewrite CTLG and EXTR 
 *              metadata files.
 *              Else, so a to save CPU, these files will rewritten
 *              only when a new last file (ending with NN instead of a
 *              number) is provided.
 =======================================================================*/
int 
mdtxUpgrade(char* label)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;

  checkLabel(label);

  if (!(conf = getConfiguration())) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;

  // force metadata to be loaded and latter saved.
  if (!loadCollection(coll, CTLG|EXTR|SERV)) goto error;
  if (!wasModifiedCollection(coll, CTLG|EXTR|SERV)) goto error2;

  rc = TRUE;
 error2:
  if (!releaseCollection(coll, CTLG|EXTR|SERV)) goto error;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUpgrade fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : addKey
 * Description: add a proxy
 * Synopsis   : int addKey(char* label, char* proxy)
 * Input      : char* label = collection to modify
 *              char* path = fingerprint to add
 * Output     : TRUE on success

 * Note       : used to accept a server subscription to a collection
 =======================================================================*/
int 
addKey(char* label, char* path)
{
  int rc = FALSE;
  Collection* coll = 0;
  char* key = 0;
  char hash[MAX_SIZE_MD5+1];
  ServerTree* servers = 0;
  Server* server = 0;

  if (label == 0) goto error;
  if (path == 0) goto error;
  logMain(LOG_INFO, "add %s key to %s collection", path, label);

  // parse server DB for merge
  if (!(coll = mdtxGetCollection(label))) goto error;
  servers = coll->serverTree;

  // look if key is ourself
  if (!(key = readPublicKey(path))) goto error;
  if (!getFingerPrint(key, hash)) goto error;
  if (!strncmp(coll->userFingerPrint, hash, MAX_SIZE_MD5)) {
    logMain(LOG_WARNING, "key \"%s\" is our %s collection key", 
	    hash, coll->label);
    goto error;
  }

  // look if key is already shared by the collection
  if (!loadCollection(coll, SERV)) goto error;
  rgRewind(servers->servers);
  while ((server = rgNext(servers->servers))) {
    if (!strncmp(server->fingerPrint, hash, MAX_SIZE_MD5)) break;
  }

  if (server) {
    logMain(LOG_WARNING, "key \"%s\" is already added to %s collection", 
	    hash, coll->label);
    goto error2;
  }

  // add a new server entry
  if (!(server = addServer(coll, hash))) goto error2;
  server->userKey = key;
  key = 0;

  // needed a second time
  if (!upgradeSshConfiguration(coll)) goto error2;

  if (!wasModifiedCollection(coll, SERV)) goto error2;

  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "cannot add %s key to %s collection", path, label);
  }
  if (key) free(key);
  return rc;
}


/*=======================================================================
 * Function   : delKey
 * Description: del a proxy
 * Synopsis   : int delKey(char* label, char* proxy)
 * Input      : char* label = collection to modify
 *              char* proxy = fingerprint to add
 * Output     : TRUE on success

 * Note       : used to revoke a server from a collection
 =======================================================================*/
int 
delKey(char* label, char* key)
{
  int rc = FALSE;
  Collection* coll = 0;
  char hash[MAX_SIZE_MD5+1];
  ServerTree* servers = 0;
  Server* server = 0;

  if (label == 0) goto error;
  if (key == 0) goto error;
  logMain(LOG_INFO, "del %s key from %s collection", key, label);

  // we are expecting a fingerprint
  strncpy(hash, key, MAX_SIZE_MD5+1);

  // parse server DB for merge
  if (!(coll = mdtxGetCollection(label))) goto error;
  servers = coll->serverTree;

  // look if key is ourself
  if (!strncmp(coll->userFingerPrint, hash, MAX_SIZE_MD5)) {
    logMain(LOG_WARNING, "key \"%s\" is our %s collection key", 
	    hash, coll->label);
    goto error;
  }

  // look if key is shared by the collection
  if (!loadCollection(coll, SERV)) goto error;
  rgRewind(servers->servers);
  while ((server = rgNext(servers->servers))) {
    if (!strncmp(server->fingerPrint, hash, MAX_SIZE_MD5)) break;
  }

  if (server == 0) {
    logMain(LOG_WARNING, 
	    "key \"%s\" not share with %s collection", 
	    hash, coll->label);
    goto error2;
  }
   
  // we delete the server
  if (!delServer(coll, server)) goto error2;

  // needed a second time
  if (!upgradeSshConfiguration(coll)) goto error2; 

  if (!wasModifiedCollection(coll, SERV)) goto error2;

  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "cannot del %s key to %s collection", key, coll);
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
