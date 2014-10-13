/*=======================================================================
 * Version: $Id: serv.c,v 1.1 2014/10/13 19:38:48 nroche Exp $
 * Project: MediaTeX
 * Module : wrapper/serv
 *
 * Manage servers.txt modifications

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

#include "../mediatex.h"
#include "../misc/log.h"
#include "../memory/confTree.h"
#include "../common/ssh.h"
#include "../common/openClose.h"

#include <sys/types.h>
#include <sys/ipc.h>
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
  Configuration* conf = NULL;
  int rc = FALSE;
  key_t key;
  semun_t u_semun;
  unsigned short table[1];
  int uid = getuid();

  logEmit(LOG_DEBUG, "%s", "initialise the concurrent access"); 

  if (!(conf = getConfiguration())) goto error;
  if (conf->sem != 0) goto end;
  
  // force becoming mdtx user 
  if (!becomeUser(env.confLabel, FALSE)) goto error;
  
  // Create a new IPC key base on an i-node + minor peripherical
  // number and a project number
  if ((key = ftok(conf->confFile, COMMON_OPEN_CLOSE_PROJECT_ID)) == -1) {
    logEmit(LOG_ERR, "ftok fails to create IPC key: %s", strerror(errno)); 
    logEmit(LOG_NOTICE, "ftok using path: %s", conf->confFile); 
    goto error;
  }

  logEmit(LOG_INFO, "client ipc key: {%s, %i}", 
	  conf->confFile, COMMON_OPEN_CLOSE_PROJECT_ID); 
#ifndef utMAIN
  logEmit(LOG_INFO, "client ipc key: 0x%x", key); 
#endif

  // Try to match an existing semaphore with the key
  if (( conf->sem = semget(key, 1, 0)) == -1) {

    // Create a new semaphore matched with the key
    if ((conf->sem = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600)) == -1) {
      logEmit(LOG_ERR, "semget fails matching 0x%x IPC key: %s", 
	      strerror(errno)); 
      logEmit(LOG_NOTICE, "%s", "(semaphore as gone)");
      goto error;
    }
  
    // Initialize the semaphore(s) values
    table[0] = 1;
    u_semun.table = table;
    if (semctl(conf->sem, 0, SETALL, u_semun) <0) {
      logEmit(LOG_ERR, "semctl fails to initialize semaphore value: %s", 
	      strerror(errno)); 
      goto error;
    }
  }

 end:
  rc = TRUE;
 error:
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to set concurrent access lock"); 
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
  Configuration* conf = NULL;
  struct sembuf sembuf;
  int uid = getuid();

  logEmit(LOG_DEBUG, "%s", "get the concurrent access"); 

  if (!(conf = getConfiguration())) goto error;
  if (!setConcurentAccessLock()) goto error;
  if (conf->sem == 0) {
    logEmit(LOG_ERR, "%s", "concurrent access lock is not initialized"); 
    goto error;
  }

  // force becoming mdtx user 
  if (!becomeUser(env.confLabel, FALSE)) goto error;

  // P (may I ?)
  logEmit(LOG_DEBUG, "%s", "get writter lock");
  sembuf.sem_num = 0;
  sembuf.sem_op = -1;
  sembuf.sem_flg = SEM_UNDO | IPC_NOWAIT; // for crash recovery...
  //... + do not asleep process if not available but exit with EAGAIN
  if (semop(conf->sem, &sembuf, 1) < 0) {
    if (errno == EAGAIN) {
      logEmit(LOG_ERR, "%s", 
	      "cannot take concurent access as it is already locked");
    }
    else {
      logEmit(LOG_ERR, "semop fails asking for semaphore: %s", 
	      strerror(errno)); 
    }
    goto error;
  }

  // Parse files
  
  rc = TRUE;
 error:
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to get writter lock"); 
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
  Configuration* conf = NULL;
  union semun arg;
  int uid = getuid();

  logEmit(LOG_DEBUG, "%s", "release the concurrent access"); 

  if (!(conf = getConfiguration())) goto error;
  if (conf->sem == 0) {
    logEmit(LOG_ERR, "%s", "concurrent access lock already unlock"); 
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
    logEmit(LOG_ERR, "semctl fails: %s", strerror(errno)); 
    goto error;
  }

  conf->sem = 0;
  rc = TRUE;
 error:
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to release the writter lock"); 
  }
  return rc;
}


/*=======================================================================
 * Function   : mdtxUpdate
 * Description: call update.sh
 * Synopsis   : int mdtxUpdate(char* label)
 * Input      : char* label: the collection to update
 * Output     : TRUE on success
 =======================================================================*/
int
mdtxUpdate(char* label)
{
  int rc = FALSE;
  Configuration* conf = NULL;
  Collection* coll = NULL;

  checkLabel(label);
  logEmit(LOG_DEBUG, "%s", "update collection");

  if (!(conf = getConfiguration())) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;
  if (!callUpdate(coll->user)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to update collection");
  }
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
mdtxCommit(char* label)
{
  int rc = FALSE;
  Configuration* conf = NULL;
  Collection* coll = NULL;

  checkLabel(label);
  logEmit(LOG_DEBUG, "%s", "commit collection");

  if (!(conf = getConfiguration())) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;
  if (!callCommit(coll->user, coll->userFingerPrint)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to commit collection");
  }
  return(rc);
}


/*=======================================================================
 * Function   : mdtxUpgrade
 * Description: Upgrade the DB by serializing it
 * Synopsis   : int mdtxUpgrade(char* label)
 * Input      : char* label = collection to upgrade
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxUpgrade(char* label)
{
  int rc = FALSE;
  Configuration* conf = NULL;
  Collection* coll = NULL;

  checkLabel(label);
  logEmit(LOG_DEBUG, "plan to upgrade %s collection", label);

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
    logEmit(LOG_ERR, "%s", "mdtxUpgrade fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : addKey
 * Description: add a proxy
 * Synopsis   : int addKey(char* label, char* proxy)
 * Input      : char* label = collection to modify
 *              char* proxy = fingerprint to add
 * Output     : TRUE on success
 =======================================================================*/
int 
addKey(char* label, char* path)
{
  int rc = FALSE;
  Collection* coll = NULL;
  char* key = NULL;
  char hash[MAX_SIZE_HASH+1];
  ServerTree* servers = NULL;
  Server* server = NULL;

  if (label == NULL) goto error;
  if (path == NULL) goto error;
  logEmit(LOG_INFO, "add %s key to %s collection", path, label);

  // parse server DB for merge
  if (!(coll = mdtxGetCollection(label))) goto error;
  servers = coll->serverTree;

  // look if key is ouself
  if (!(key = readPublicKey(path))) goto error;
  if (!getFingerPrint(key, hash)) goto error;
  if (!strncmp(coll->userFingerPrint, hash, MAX_SIZE_HASH)) {
    logEmit(LOG_WARNING, "key \"%s\" is our %s collection key", 
	    hash, coll->label);
    goto error;
  }

  // look if key is already shared by the collection
  if (!loadCollection(coll, SERV)) goto error;
  rgRewind(servers->servers);
  while ((server = rgNext(servers->servers)) != NULL) {
    if (!strncmp(server->fingerPrint, hash, MAX_SIZE_HASH)) break;
  }

  if (server != NULL) {
    logEmit(LOG_WARNING, "key \"%s\" is already added to %s collection", 
	    hash, coll->label);
    goto error2;
  }

  // add a new server entry
  if (!(server = addServer(coll, hash))) goto error2;
  server->userKey = key;
  key = NULL;
  if (!upgradeSshConfiguration(coll)) goto error2;
  if (!wasModifiedCollection(coll, SERV)) goto error2;

  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV)) rc = FALSE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "cannot add %s key to %s collection", path, label);
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
 =======================================================================*/
int 
delKey(char* label, char* key)
{
  int rc = FALSE;
  Collection* coll = NULL;
  char hash[MAX_SIZE_HASH+1];
  ServerTree* servers = NULL;
  Server* server = NULL;

  if (label == NULL) goto error;
  if (key == NULL) goto error;
  logEmit(LOG_INFO, "del %s key from %s collection", key, label);

  // we are expecting a fingerprint
  strncpy(hash, key, MAX_SIZE_HASH+1);

  // parse server DB for merge
  if (!(coll = mdtxGetCollection(label))) goto error;
  servers = coll->serverTree;

  // look if key is ouself
  if (!strncmp(coll->userFingerPrint, hash, MAX_SIZE_HASH)) {
    logEmit(LOG_WARNING, "key \"%s\" is our %s collection key", 
	    hash, coll->label);
    goto error;
  }

  // look if key is shared by the collection
  if (!loadCollection(coll, SERV)) goto error;
  rgRewind(servers->servers);
  while ((server = rgNext(servers->servers)) != NULL) {
    if (!strncmp(server->fingerPrint, hash, MAX_SIZE_HASH)) break;
  }

  if (server == NULL) {
    logEmit(LOG_WARNING, 
	    "key \"%s\" not share with %s collection", 
	    hash, coll->label);
    goto error2;
  }
   
  // we delete the server
  if (!delServer(coll, server)) goto error2;
  if (!upgradeSshConfiguration(coll)) goto error2;
  if (!wasModifiedCollection(coll, SERV)) goto error2;

  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV)) rc = FALSE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "cannot del %s key to %s collection", key, coll);
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "../misc/signals.h"

GLOBAL_STRUCT_DEF;
int running = TRUE;

void* 
sigManager(void* arg)
{
  int sigNumber = 0;

  (void) arg;
  logEmit(LOG_NOTICE, "%s", "please send me HUP, USR1 or TERM signals:");
  logEmit(LOG_NOTICE, "- kill -SIGHUP %i", getpid());
  logEmit(LOG_NOTICE, "- kill -SIGUSR1 %i", getpid());
  logEmit(LOG_NOTICE, "- kill -SIGTERM %i", getpid());

  if ((sigNumber = sigwaitinfo(&signalsToManage, NULL)) == -1) {
    logEmit(LOG_ERR, "sigwait fails: %s", strerror(errno));
    goto error;
  }
 
  running = FALSE;
  return (void*)TRUE;
 error:
  return (void*)FALSE;
}

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
  fprintf(stderr, " [ -k ]");
  mdtxOptions();
  fprintf(stderr, 
	  "  -k, --lock\t\ttest the concurrent lock\n");
  return;
}

  
/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for cache module.
 * Synopsis   : ./utupgrade
 * Input      : -i mediatex.conf
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = NULL;
  pthread_t thread;
  int doLock = FALSE;
  int err = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"k";
  struct option longOptions[] = {
    {"lock", no_argument, NULL, 'k'},
    MDTX_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
    case 'k':
      doLock = TRUE;
      break;			

      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  // test writter lock
  if (doLock) {
    if (!clientWriteLock()) goto error;

    // infinite loop (file descriptor should not be available)
    if (!manageSignals(sigManager, &thread)) goto error;
    while (running) usleep(200000);
    
    if (!clientWriteUnlock()) goto error;
    if ((err = pthread_join(thread, NULL))) {
      logEmit(LOG_ERR, "pthread_join fails: %s", strerror(err));
      goto error;
    }
  }

  // others tests
  if (!(coll = addCollection("coll1"))) goto error;

  logEmit(LOG_NOTICE, "%s", "*** upgrade:");
  env.noCollCvs = FALSE;
  if (!mdtxUpgrade("coll1")) goto error;

  logEmit(LOG_NOTICE, "%s", "*** refuse to del localhost key: ");
  if (delKey("coll1", "746d6ceeb76e05cfa2dea92a1c5753cd")) goto error;
    
  logEmit(LOG_NOTICE, "%s", "*** del a key: ");
  if (!delKey("coll1", "bedac32422739d7eced624ba20f5912e")) goto error;
  
  logEmit(LOG_NOTICE, "%s", "*** refuse to add localhost key: ");
  if (addKey("coll1", "user1Key_rsa.pub")) goto error;

  logEmit(LOG_NOTICE, "%s", "*** add a key: ");
  if (!addKey("coll1", "user3Key_dsa.pub")) goto error;

  logEmit(LOG_NOTICE, "%s", "*** save and disease test: ");
  if (!saveCollection(coll, SERV)) goto error;
  if (!diseaseCollection(coll, SERV)) goto error;
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
