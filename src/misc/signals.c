/*=======================================================================
 * Version: $Id: signals.c,v 1.1 2014/10/13 19:39:35 nroche Exp $
 * Project: MediaTeX
 * Module : signal
 *
 * Signal manager

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

#include "log.h"
#include "signals.h"

#include <unistd.h>
#include <errno.h>
#include <pthread.h>

sigset_t signalsToManage;
static sigset_t backupALL;

/*=======================================================================
 * Function   : disableALL
 * Description: Block all signal we will manage in dedicated threads
 * Synopsis   : static int disableALL()
 * Input      : N/A
 * Output     : True on success
 * Note       : This has to be call in main thread so as new threads
 *              inherit this mask
 =======================================================================*/
static int 
disableALL()
{
  int rc = TRUE;

  // we don't want to be disturb by the following signals:
  rc=rc&& (sigemptyset(&signalsToManage) == 0);
  rc=rc&& (sigaddset(&signalsToManage, SIGHUP) == 0);
  rc=rc&& (sigaddset(&signalsToManage, SIGUSR1) == 0);
  rc=rc&& (sigaddset(&signalsToManage, SIGTERM) == 0);
  rc=rc&& (sigaddset(&signalsToManage, SIGSEGV) == 0);
  rc=rc&& (sigaddset(&signalsToManage, SIGINT) == 0);
  rc=rc&& (sigaddset(&signalsToManage, SIGALRM) == 0);
  if (!rc) goto error;
  
  
  if (sigprocmask(SIG_BLOCK, &signalsToManage, &backupALL)) {
    logEmit(LOG_ERR, "sigprocmask fails: %s", strerror(errno));
    rc = FALSE;
  }

 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "disableALL fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : reEnableALL
 * Description: use to re-enable the defaut behaviour before exiting
 * Synopsis   : int reEnableALL()
 * Input      : N/A
 * Output     : True on success
 =======================================================================*/
int 
reEnableALL()
{
  int rc = TRUE;

  rc=rc&& (sigprocmask(SIG_SETMASK, &backupALL, NULL) == 0);
  return rc;
}

/*=======================================================================
 * Function   : enableAlarm
 * Description: accept only SIGALRM on the calling thread
 * Synopsis   : int enableAlarm()
 * Input      : N/A
 * Output     : True on success
 =======================================================================*/
int 
enableAlarm()
{
  int rc = TRUE;
  sigset_t mask;
  int err = 0;

  rc=rc&& (sigfillset(&mask) == 0);
  rc=rc&& (sigdelset(&mask, SIGALRM) == 0);
  if (!rc) goto error;

  if ((err = pthread_sigmask(SIG_SETMASK, &mask, NULL))) {
    logEmit(LOG_ERR, "pthread_sigmask fails: %s", strerror(err));
    rc = FALSE;
  }

 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "enableAlarm fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : disableAlarm
 * Description: do not accept any more SIGALRM on the calling thread
 * Synopsis   : int disableAlarm()
 * Input      : N/A
 * Output     : True on success
 =======================================================================*/
int 
disableAlarm()
{
  int rc = FALSE;
  int err = 0;

  if ((err = pthread_sigmask(SIG_SETMASK, &signalsToManage, NULL))) {
    logEmit(LOG_ERR, "pthread_sigmask fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "enableAlarm fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : manageSignals
 * Description: Start a thread manager that handle signals
 * Synopsis   : int manageSignals(void* manager(void*))
 * Input      : manager: the function to be run by the thread
 * Output     : True on success
 =======================================================================*/
int
manageSignals(void* manager(void*), pthread_t* thread)
{
  int rc = TRUE;

  logEmit(LOG_DEBUG, "%s", "manageSignals");
 
  // manager will handle SIGHUP,SIGUSR1,SIGTERM,SIGSEGV,SIGINT ...
  rc=rc&& disableALL();

  // ... but not SIGALRM !
  rc=rc&& (sigdelset(&signalsToManage, SIGALRM) == 0);
  if (!rc) goto error;

  rc = pthread_create(thread, NULL, manager, (void *)NULL);
  if (rc != 0) {
    logEmit(LOG_ERR, "pthread_create fails: %s", strerror(rc));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "manageSignals fails");
  }
  return rc;
}


/************************************************************************/

#ifdef utMAIN
#include "command.h"
GLOBAL_STRUCT_DEF;

int running = TRUE;

void* 
empty(void* arg)
{
  (void) arg;
  return (void*)TRUE;
}

/*=======================================================================
 * Function   : exemple sigManager
 * Description: Exemple for a signal manager
 * Synopsis   : void* sigManager(void* arg)
 * Input      : void* arg
 * Output     : NULL
 * Note       : Not interrupted by signals as sigwaitinfo wait for them
 *              and wake-up when a new signal is available
 =======================================================================*/
void* 
sigManager(void* arg)
{
  int sigNumber = 0;

  (void) arg;
  logEmit(LOG_NOTICE, "%s", "please send me HUP, USR1 or TERM signals:");
  logEmit(LOG_DEBUG, "- kill -SIGHUP %i", getpid());
  logEmit(LOG_DEBUG, "- kill -SIGUSR1 %i", getpid());
  logEmit(LOG_DEBUG, "- kill -SIGTERM %i", getpid());

  while (running) {
    if ((sigNumber = sigwaitinfo(&signalsToManage, NULL)) == -1) {
      logEmit(LOG_ERR, "sigwait fails: %s", strerror(errno));
      goto error;
    }
  
    logEmit(LOG_NOTICE, "signal reçu: %i", sigNumber);
    switch (sigNumber) {
    case SIGHUP:
      logEmit(LOG_NOTICE, "%s", "=> HUP");
      break;
    case SIGUSR1:
      logEmit(LOG_NOTICE, "%s", "=> SIGUSR1");
      break;
    case SIGTERM:
      logEmit(LOG_NOTICE, "%s", "=> SIGTERM");
      running = FALSE;
      break;
    case SIGSEGV:
      logEmit(LOG_NOTICE, "%s", "=> SIGSEGV");
      reEnableALL();
      kill(getpid(), SIGSEGV);
    case SIGINT:
      logEmit(LOG_NOTICE, "%s", "=> SIGINT");
      reEnableALL();
      kill(getpid(), SIGINT);
      break;
    default:
      logEmit(LOG_NOTICE, "%s", "=> ???");
    }
    fflush(stdout);
  }

  return (void*)TRUE;
 error:
  running = FALSE;
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
  miscUsage(programName);

  miscOptions();
  //fprintf(stderr, "\t\t---\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for signal module
 * Synopsis   : ./utsignal
 * Input      : Please send signals as it is prompted
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  pthread_t thread;
  int err = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS;
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {0, 0, 0, 0}
  };
  
  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {      
      GET_MISC_OPTIONS;
    }
    if (rc) goto optError;
  }

  if ((err = pthread_create(&thread, NULL, empty, (void *)NULL))) {
    logEmit(LOG_ERR, "pthread_create fails: %s", strerror(err));
    goto error;
  }

  if ((err = pthread_join(thread, NULL))) {
    logEmit(LOG_ERR, "pthread_join fails: %s", strerror(err));
    goto error;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!manageSignals(sigManager, &thread)) goto error;
  while(running) {
    usleep(200000);
  }

  if ((err = pthread_join(thread, NULL))) {
    logEmit(LOG_ERR, "pthread_join fails: %s", strerror(err));
    goto error;
  }
  /************************************************************************/

  rc = TRUE;
 error:
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


