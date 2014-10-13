/*=======================================================================
 * Version: $Id: threads.c,v 1.1 2014/10/13 19:39:55 nroche Exp $
 * Project: MediaTeX
 * Module : server/threads

 * Handle sockets and signals

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
#include "../misc/tcp.h"
#include "../misc/signals.h"
#include "../misc/shm.h"
#include "../memory/confTree.h"
#include "../common/register.h"
#include "threads.h"

#include <pthread.h>
#include <netinet/in.h> // for inet_ntoa
#include <arpa/inet.h>  // for inet_ntoa

// global variables
extern sigset_t signalsToManage;
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
      logEmit(LOG_INFO, "pthread_mutex_init: %s", strerror(rc));
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
    
  if((rc = pthread_attr_init(attr)) != 0) {
    logEmit(LOG_ERR, "Create thread attributes: %d (%s)",
	    rc, strerror(rc));
    goto error;
  }
  
  if((rc = pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED))) {
    logEmit(LOG_ERR, "fails to set thread detach attr: %d (%s)",
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
 * Output     : alway NULL
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
  
  (void) arg;
  
  if (!initMutex()) goto error;

  while (env.running) {
    sigNumber = -1;
    if ((sigNumber = sigwaitinfo(&signalsToManage, NULL)) == -1) {
      if (errno == EINTR) continue; // so as to manage debugging with gdb
      logEmit(LOG_ERR, "sigwait fails: %s", strerror(errno));
      goto error;
    }
    
    switch (sigNumber) {

    case SIGSEGV:
      logEmit(LOG_ERR, "%s", "segfault (SIGSEGV)");
      reEnableALL();
      kill(getpid(), SIGSEGV);
      goto error;
      break;
    case SIGINT:
      logEmit(LOG_ERR, "%s", "stopped by user (SIGINT)");
      reEnableALL();
      kill(getpid(), SIGINT);
      goto error;
      break;

    case SIGHUP:
      // wait until no more job are running
      hold = TRUE;
      logEmit(LOG_NOTICE, "%s", "accepting signal HUP");
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
#ifndef utMAIN
      memoryStatus(LOG_NOTICE);
#endif
      hold = FALSE;
      continue;

    case SIGTERM:
      env.running = FALSE;

      // wait until no more job are running
      hold = TRUE;
      logEmit(LOG_NOTICE, "%s", "accepting signal TERM");
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

#ifdef utMAIN
    case SIGUSR1:
      logEmit(LOG_NOTICE, "%s", "accepting signal USR1");    
      break;
#endif

    default:
      logEmit(LOG_INFO, "receive signal %i", sigNumber);
#ifdef utMAIN
      continue;
#endif
    }

    // wait for a dedicated thread allocation
    while (taskSignalNumber+1 > MAX_TASK_SIGNAL_THREAD) {
      if (!env.running) goto error;
      logEmit(LOG_WARNING, "%s", "pending USR1 signal");
      sleep(20);
    };

    pthread_mutex_lock(&jobsMutex);
    ++taskSignalNumber; // manage concurency against ending signal jobs
    pthread_mutex_unlock(&jobsMutex);

    // create dedicated thread for signal query
    while ((rc = pthread_create(&signalTaskThread, &taskAttr,
  				signalJob, NULL))) {
      logEmit(LOG_ERR, "pthread_create fails: %s", strerror(rc));
      if (rc == EAGAIN && try--) {
  	usleep(100000);
  	continue;
      }
      taskSignalNumber--;
      break;
    }
  }

 error:
  return (void*)NULL;
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
#ifndef utMAIN
  memoryStatus(LOG_NOTICE);
#endif
}


/*=======================================================================
 * Function   : serverManager
 * Description: Get accepted socket and call dedicated callback function
 * Synopsis   : int serverLoop(int sock, 
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
  Connexion* connexion = NULL;
  int port = -1;

  // connexion informations 
  if (!(connexion = (Connexion*)malloc(sizeof(Connexion)))) {
    logEmit(LOG_ERR, "malloc cannot create connexion objet: %s", 
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
    logEmit(LOG_WARNING, "pending connexion from %s:%u (%s)",
	    inet_ntoa(address_accepted->sin_addr),
	    port, connexion->host);
    sleep(2);
  };

  // manage concurency with ourself, SIGHUP and SIGTERM
  pthread_mutex_lock(&jobsMutex);
  if (hold || taskSocketNumber+1 > MAX_TASK_SOCKET_THREAD) {
    pthread_mutex_unlock(&jobsMutex);
    goto retry;
  }
  ++taskSocketNumber;
  pthread_mutex_unlock(&jobsMutex);

  logEmit(LOG_INFO, "accepting connexion from %s:%u (%s)",
	  inet_ntoa(address_accepted->sin_addr),
	  port, connexion->host);
  
  // create dedicated thread for socket query
  while ((rc = pthread_create(&socketTaskThread, &taskAttr, 
			      socketJob, (void *)connexion))) {
    logEmit(LOG_ERR, "pthread_create fails: %s", strerror(rc));
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

  connexion = NULL;
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
 * Description: Function to be called by finishing socket job
 * Synopsis   : int socketJobEnds()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
void socketJobEnds()
{
  pthread_mutex_lock(&jobsMutex);
  taskSocketNumber--;
  pthread_mutex_unlock(&jobsMutex);
#ifndef utMAIN
  memoryStatus(LOG_NOTICE);
#endif
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
    logEmit(LOG_ERR, "cannot convert %i port number into service", 
	    getConfiguration()->mdtxPort);
    goto error;
  }
  
  // build the listenning socket for both remote and local connection
  if (!buildSocketAddress(&address, NULL, "tcp", service)) {
    logEmit(LOG_ERR, "%s", "error while building socket address");
    goto error;
  }
  
  // main loop (listen to socket)
  logEmit(LOG_NOTICE, "Daemon (%i) started", getpid());
  if (!acceptTcpSocket(&address, serverManager)) goto error;
  
  rc = TRUE;
 error:
  env.running = FALSE;
  if (!kill(getpid(), SIGTERM) == -1) {
    logEmit(LOG_ERR, "killing signal handler failed: %s", errno);
    rc = FALSE;
  }
  if (!mdtxShmFree()) rc = FALSE;
  if (thread && (err = pthread_join(thread, NULL))) {
    logEmit(LOG_ERR, "pthread_join fails: %s", strerror(err));
    goto error;
  }
  pthread_attr_destroy(&taskAttr);
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
GLOBAL_STRUCT_DEF;

/*=======================================================================
 * Function   : termManager
 * Description: Callback function for SIGTERM
 * Synopsis   : void termManager()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int
termManager()
{
  logEmit(LOG_NOTICE, "%s", "daemon exiting");
  return TRUE;
}

/*=======================================================================
 * Function   : hupManager
 * Description: Callback function for SIGHUP
 * Synopsis   : void hupManager()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int
hupManager()
{
  logEmit(LOG_NOTICE, "%s", "daemon wake-up");
  return TRUE;
}


/*=======================================================================
 * Function   : signalJob
 * Description: thread callback function for signals
 * Synopsis   : void* signalJob(void* arg)
 * Input      : void* arg = not used
 * Output     : (void*)TRUE on success
 =======================================================================*/
void* 
signalJob(void* arg)
{
  Configuration* conf = NULL;
  int me = 0;
  long int rc = FALSE;
  ShmParam param;

  (void)arg;
  if (!(conf = getConfiguration())) goto error;
  me = taskSignalNumber;
  
  if (!shmRead(conf->confFile, MDTX_SHM_BUFF_SIZE,
  	       mdtxShmRead, (void*)&param))
    goto error;

  logEmit(LOG_INFO, "in  shm: (%s)", param.buf);  

  if (strcmp(param.buf, "0000")) {
    usleep(50000);
    logEmit(LOG_NOTICE, "doing job %i for signal", me);
    usleep(50000);
    logEmit(LOG_NOTICE, "finish job %i for signal", me);
    usleep(50000);
    strcpy(param.buf, "0000");
    rc = shmWrite(conf->confFile, MDTX_SHM_BUFF_SIZE,
    		  mdtxShmCopy, (void*)&param);
  }

  logEmit(LOG_INFO, "out shm: (%s)", param.buf);  
  rc = TRUE;
 error:
  signalJobEnds();
  return (void*)rc;
}


/*=======================================================================
 * Function   : socketJob
 * Description: thread callback function for sockets
 * Synopsis   : void* socketJob(void* arg)
 * Input      : void* arg = Connexion*: the connection data struct
 * Output     : N/A
 =======================================================================*/
void* socketJob(void* arg)
{
  int me = 0;
  long int rc = FALSE;
  Connexion* connexion = NULL;

  me = taskSocketNumber;
  connexion = (Connexion*)arg;

  close(connexion->sock);
  usleep(50000);
  logEmit(LOG_NOTICE, "doing job %i for %s", me, connexion->host);
  usleep(50000);
  logEmit(LOG_NOTICE, "finish job %i for %s", me, connexion->host);
  usleep(50000); 

  rc = TRUE;
  if (connexion) {
    if (connexion->host) free(connexion->host);
    free(connexion);
  }
  socketJobEnds();
  return (void*)rc;
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

  mdtxOptions();
  //fprintf(stderr, "  ---\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : 2011/03/05
 * modif      : 2012/05/01
 * Description: Unit test for threads module.
 * Synopsis   : ./utthreads
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS;
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
  /*
    in ut, 3 telnet without sleep =>
    > [err threads.c] pthread_create fails: Resource temporarily unavailable
    > [err threads.c] pthread_create fails: Resource temporarily unavailable
  */
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!mainLoop()) goto error;
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
