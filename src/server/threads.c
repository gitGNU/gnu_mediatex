/*=======================================================================
 * Project: MediaTeX
 * Module : threads

 * Handle sockets and signals

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
#include "server/mediatex-server.h"

// global variables
int hold = FALSE;

static int isMutexInitialized = FALSE;
static pthread_mutex_t jobsMutex;
static pthread_attr_t taskAttr;
int taskSocketNumber = 0;
int taskSignalNumber = 0;

/*=======================================================================
 * Function   : initMutex
 * Description: Initialize internal mutex
 * Synopsis   : static int initMutex()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
static int 
initMutex()
{
  int rc = TRUE;

  if (!isMutexInitialized) {
    if ((rc = pthread_mutex_init(&jobsMutex, (pthread_mutexattr_t*)0))) {
      logMain(LOG_INFO, "pthread_mutex_init: %s", strerror(rc));
      rc = FALSE;
      goto error;
    }
    isMutexInitialized = TRUE;
    rc = TRUE;
  }
 error:
  return rc;
}

/*=======================================================================
 * Function   : initThreadParamaters
 * Description: Initialize thread parameters
 * Synopsis   : static int initThreadParamaters(pthread_attr_t *attr)
 * Input      : pthread_attr_t *attr = the thread parameters
 * Output     : TRUE on success
 =======================================================================*/
static int 
initThreadParamaters(pthread_attr_t *attr)
{
  int rc = 0;
    
  if((rc = pthread_attr_init(attr))) {
    logMain(LOG_ERR, "Create thread attributes: %d (%s)",
	    rc, strerror(rc));
    goto error;
  }
  
  if((rc = pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED))) {
    logMain(LOG_ERR, "fails to set thread detach attr: %d (%s)",
	    rc, strerror(rc));
    goto error;
  }

  // pthread_attr_setstackaddr ?
  // pthread_attr_setstacksize ?

  return TRUE;
 error:
  return FALSE;
};


/*=======================================================================
 * Function   : sigManager
 * Description: Signal handler calling the callback function
 * Synopsis   : void* sigManager(void* arg)
 * Input      : void* arg
 * Output     : alway 0
 * Note       : Concurency with serverManager (not with ourself)
                May force exit (if it fails softly)
 =======================================================================*/
void* 
sigManager(void* arg)
{
  int rc = 0;
  pthread_t signalTaskThread;
  int sigNumber = -1;
  int try = 5;
  struct sockaddr_in address;
  int port = 0;
  sigset_t mask;

  (void) arg;

  if (!initMutex()) goto error;

  // signal we are looking for:
  if (sigemptyset(&mask)) goto error;
  if (sigaddset(&mask, SIGHUP)) goto error;
  if (sigaddset(&mask, SIGUSR1)) goto error;
  if (sigaddset(&mask, SIGTERM)) goto error;
  if (sigaddset(&mask, SIGSEGV)) goto error;
  if (sigaddset(&mask, SIGINT)) goto error;

  while (env.running) {
    sigNumber = -1;
    if ((sigNumber = sigwaitinfo(&mask, 0)) == -1) {
      if (errno == EINTR) continue; // so as to manage debugging with gdb
      logMain(LOG_ERR, "sigwait fails: %s", strerror(errno));
      goto error;
    }
    
    switch (sigNumber) {

    case SIGSEGV:
      logMain(LOG_ERR, "segfault (SIGSEGV)");
      reEnableALL();
      kill(getpid(), SIGSEGV);
      goto error;
      break;
    case SIGINT:
      logMain(LOG_ERR, "stopped by user (SIGINT)");
      reEnableALL();
      kill(getpid(), SIGINT);
      goto error;
      break;

    case SIGHUP:
      // wait until no more job are running
      hold = TRUE;
      logMain(LOG_NOTICE, "accepting signal HUP");
    retry1:
      while (taskSocketNumber > 0 || taskSignalNumber > 0)
	usleep(100000);

      pthread_mutex_lock(&jobsMutex);
      if (taskSocketNumber > 0) {
	pthread_mutex_unlock(&jobsMutex);
	goto retry1;
      }

      if (!hupManager()) exit(1); // force exit if reload fails;
      pthread_mutex_unlock(&jobsMutex);
      if (!env.noRegression) {
	memoryStatus(LOG_NOTICE, __FILE__, __LINE__);
      }
      hold = FALSE;
      continue;

    case SIGTERM:
      env.running = FALSE;

      // wait until no more job are running
      hold = TRUE;
      logMain(LOG_NOTICE, "accepting signal TERM");
    retry2:
      while (taskSocketNumber > 0 || taskSignalNumber > 0)
	usleep(100000);

      pthread_mutex_lock(&jobsMutex);
      if (taskSocketNumber > 0) {
	pthread_mutex_unlock(&jobsMutex);
	goto retry2;
      }

      // assert no more job will be launched
      taskSocketNumber = MAX_TASK_SOCKET_THREAD;
      taskSignalNumber = MAX_TASK_SIGNAL_THREAD; // (not needed here)
      pthread_mutex_unlock(&jobsMutex);

      if (!termManager()) exit(2); // force exit if it fails;;

      // connect ouself in order to force exit from accept
      rc = TRUE;
      rc=rc&& (port = getConfiguration()->mdtxPort);
      rc=rc&& buildSocketAddressEasy(&address, 0x7f000001, 
				      getConfiguration()->mdtxPort);
      rc=rc&& connectTcpSocket(&address);
      if (!rc) exit(3); // force exit if socket fails

    case SIGUSR1:
      logMain(LOG_NOTICE, "accepting signal USR1");    
      break;

    default:
      logMain(LOG_INFO, "receive unexpected signal %i", sigNumber);
      continue;
    }

    // wait for a dedicated thread allocation
    while (taskSignalNumber+1 > MAX_TASK_SIGNAL_THREAD) {
      if (!env.running) goto error;
      logMain(LOG_WARNING, "pending USR1 signal");
      sleep(20);
    };

    pthread_mutex_lock(&jobsMutex);
    ++taskSignalNumber; // manage concurency against ending signal jobs
    pthread_mutex_unlock(&jobsMutex);

    // create dedicated thread for signal query
    while ((rc = pthread_create(&signalTaskThread, &taskAttr,
  				signalJob, 0))) {
      logMain(LOG_ERR, "pthread_create fails: %s", strerror(rc));
      if (rc == EAGAIN && try--) {
  	usleep(100000);
  	continue;
      }
      taskSignalNumber--;
      break;
    }
  }

 error:
  return (void*)0;
}

/*=======================================================================
 * Function   : signalJobEnds
 * Description: Function to be called by finishing signal job
 * Synopsis   : int signalJobEnds()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
void signalJobEnds()
{
  pthread_mutex_lock(&jobsMutex);
  taskSignalNumber--;
  pthread_mutex_unlock(&jobsMutex);

  if (!env.noRegression) {
    memoryStatus(LOG_INFO, __FILE__, __LINE__);
  }
}


/*=======================================================================
 * Function   : serverManager
 * Description: Get accepted socket and call dedicated callback function
 * Synopsis   : int serverManager(int sock, 
 *                                  struct sockaddr_in* address_accepted)
 * Input      : int sock = the connexion socket 
 *              struct sockaddr_in* address_accepted = the socket address
 * Output     : TRUE en success 
 * Note       : Concurency with signalanager and ourself
 =======================================================================*/
int 
serverManager(int sock, struct sockaddr_in* address_accepted)
{
  int rc = FALSE;
  pthread_t socketTaskThread;
  int try = 5;
  Connexion* connexion = 0;
  int port = -1;

  // connexion informations 
  if (!(connexion = (Connexion*)malloc(sizeof(Connexion)))) {
    logMain(LOG_ERR, "malloc cannot create connexion objet: %s", 
	    strerror(errno));
    goto error;
  }

  memset(connexion, 0, sizeof (struct Connexion));
  connexion->sock = sock;
  connexion->ipv4 = ntohl(address_accepted->sin_addr.s_addr);
  connexion->host = getHostNameByAddr(&address_accepted->sin_addr);
  port = ntohs(address_accepted->sin_port);

  // wait for a dedicated thread allocation
 retry:
  while (hold || taskSocketNumber+1 > MAX_TASK_SOCKET_THREAD) {
    if (!env.running) goto end;
    logMain(LOG_WARNING, "pending connexion from %s:%u (%s)",
	    inet_ntoa(address_accepted->sin_addr),
	    port, connexion->host);
    sleep(20);
  };

  // manage concurency with ourself, SIGHUP and SIGTERM
  pthread_mutex_lock(&jobsMutex);
  if (hold || taskSocketNumber+1 > MAX_TASK_SOCKET_THREAD) {
    pthread_mutex_unlock(&jobsMutex);
    goto retry;
  }
  ++taskSocketNumber;
  pthread_mutex_unlock(&jobsMutex);

  logMain(LOG_INFO, "accepting connexion from %s:%u (%s)",
	  inet_ntoa(address_accepted->sin_addr),
	  port, connexion->host);
  
  // create dedicated thread for socket query
  while ((rc = pthread_create(&socketTaskThread, &taskAttr, 
			      socketJob, (void *)connexion))) {
    logMain(LOG_ERR, "pthread_create fails: %s", strerror(rc));
    if (rc == EAGAIN && try--) {
      usleep(100000);
      continue;
    }
    pthread_mutex_lock(&jobsMutex);
    taskSocketNumber--; // manage concurency against ending server jobs
    pthread_mutex_unlock(&jobsMutex);
    rc = FALSE;
    goto error;
  }

  // thread had consumed the connexion variables
  connexion = 0;
  sock = 0;
 end:
  rc = TRUE;
 error:
  if (sock) close(sock);
  if (connexion) {
    if (connexion->host) free(connexion->host);
    free(connexion);
  }
  return rc;
}

/*=======================================================================
 * Function   : socketJobEnds
 * Description: Function to be called by threads having finish socket job
 * Synopsis   : int socketJobEnds()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
void socketJobEnds(Connexion* connexion)
{
  pthread_mutex_lock(&jobsMutex);
  taskSocketNumber--;
  pthread_mutex_unlock(&jobsMutex);

  // connexion variable is managed the calling thread
  if (connexion) {
    if (connexion->sock) close(connexion->sock);
    if (connexion->host) free(connexion->host);
    free(connexion);
  }

  if (!env.noRegression) {
    memoryStatus(LOG_INFO, __FILE__, __LINE__);
  }

  // usefull for gdb when debuging several threads. else get:
  // Cannot get thread event message: debugger service failed
  /* sleep(300); */
}


/*=======================================================================
 * Function   : mainLoop
 * Description: Initialize the server connexions
 * Synopsis   : int mainLoop()
 * Input      : N/A
 * Output     : FALSE on error
 =======================================================================*/
int 
mainLoop()
{
  int rc = FALSE;
  pthread_t thread;
  int err = 0;
  char service[32] = "[place to write port number]";
  struct sockaddr_in address;
 
  // initializing 
  if (!initThreadParamaters(&taskAttr)) goto error;
  if (!mdtxShmInitialize()) goto error;
  if (!manageSignals(sigManager, &thread)) goto error;

  // convert port into char*
  if (sprintf(service, "%i", getConfiguration()->mdtxPort) < 0) {
    logMain(LOG_ERR, "cannot convert %i port number into service", 
	    getConfiguration()->mdtxPort);
    goto error;
  }
  
  // build the listenning socket for both remote and local connection
  if (!buildSocketAddress(&address, 0, "tcp", service)) {
    logMain(LOG_ERR, "error while building socket address");
    goto error;
  }
  
  // main loop (listen to socket)
  logMain(LOG_NOTICE, "Daemon (%i) started", getpid());
  if (!acceptTcpSocket(&address, serverManager)) goto error;
  
  rc = TRUE;
 error:
  env.running = FALSE;
  if (!kill(getpid(), SIGTERM) == -1) {
    logMain(LOG_ERR, "killing signal handler failed: %s", errno);
    rc = FALSE;
  }
  if (!mdtxShmFree()) rc = FALSE;
  if (thread && (err = pthread_join(thread, 0))) {
    logMain(LOG_ERR, "pthread_join fails: %s", strerror(err));
    goto error;
  }
  pthread_attr_destroy(&taskAttr);
  return rc;
}


/*=======================================================================
 * Function   : checkMessage
 * Description: Common operation on message
 * Synopsis   : int checkMessage(Connexion* con)
 * Input      : Connexion* con
 * Output     : set con->tree
 *              set con->server
 *              TRUE on succes
 =======================================================================*/
int checkMessage(Connexion* con)
{
  int rc = FALSE;
  Collection* coll = 0;
  Server* server = 0;
  Record* record = 0;
  RGIT* curr = 0;
  AVLNode* node = 0;

  static char status[][64] = {
   "302 message without collection",
   "303 message from server '%s' not registered into %s collection",
   "304 message contains a record not related to author %s but %s",
   "401 fails to read message",
   "402 fails to load %s collection's serverTree",
   "403 fails to get localhost",
   "404 fails to load %s collection's cache"
  };

  logMain(LOG_DEBUG, "checkMessage");
  sprintf(con->status, "%s", status[3]);
  con->server = 0;

  logRecordTree(LOG_MAIN, LOG_NOTICE, con->message, 
		con->message->fingerPrint);

  if (!(coll = con->message->collection)) {
    sprintf(con->status, "%s", status[0]);
    goto error;
  }

  if (!loadCollection(coll, SERV)) {
    sprintf(con->status, status[5], coll->label);
    goto error;
  }

  // minimal upgrade
  if (!getLocalHost(coll)) {
    sprintf(con->status, "%s", status[6]);
    goto error2;
  }
  
  // match server from fingerprint in message header
  // (may be masqueraded by a Nat server)
  while ((server = rgNext_r(coll->serverTree->servers, &curr))) {
    if (!(strncmp(con->message->fingerPrint, server->fingerPrint, 
		  MAX_SIZE_MD5))) break;
  }
  if (server == 0) {
    sprintf(con->status, status[1], 
	    con->message->fingerPrint, coll->label);
    goto error2;
  }

  // check all records are related to the message's author
  if (avl_count(con->message->records)) {
    for (node = con->message->records->head; node; node = node->next) {
      record = node->item;
      if (server != record->server) {
	sprintf(con->status, status[2], 
		con->message->fingerPrint, record->server->fingerPrint);
	goto error2;
      }
    }
  }
  
  // load md5sum file
  if (!loadCache(coll)) {
    sprintf(con->status, status[6], coll->label);
    goto error2;
  }

  con->server = server;
  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "checkMessage fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
