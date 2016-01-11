/*=======================================================================
 * Version: $Id: utsignals.c,v 1.5 2015/10/20 19:41:46 nroche Exp $
 * Project: MediaTeX
 * Module : signal
 *
 * Thread manager that handle signals

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
#include <pthread.h>

int running = TRUE;

/*=======================================================================
 * Function   : sigalarmManager
 * Description: Used to interrupt call to connectTcpSocket
 *              in order to stop connexion when server is not responding
 * Synopsis   : static void sigalarmManager(int unused)
 * Input      : int unused: not used
 * Output     : N/A
 =======================================================================*/
static void 
sigalarmManager(int unused)
{
   logMain(LOG_NOTICE, "sigalarmManager get %i", unused);
   return;
}

/*=======================================================================
 * Function   : empty
 * Description: Thread that do nothing
 * Synopsis   : void* empty(void* arg)
 * Input      : void* arg: thread parameter from pthread_create
 * Output     : N/A
 =======================================================================*/
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
 * Output     : 0
 * Note       : Not interrupted by signals as sigwaitinfo wait for them
 *              and wake-up when a new signal is available
 =======================================================================*/
void* 
sigManager(void* arg)
{
  int sigNumber = 0;
  sigset_t mask;

  // this thread inherit bloked signals:
  //  SIGHUP, SIGUSR1, SIGTERM, SIGSEGV, SIGINT and SIGALRM

  // signal we are looking for:
  if (sigemptyset(&mask)) goto error;
  if (sigaddset(&mask, SIGHUP)) goto error;
  if (sigaddset(&mask, SIGUSR1)) goto error;
  if (sigaddset(&mask, SIGTERM)) goto error;
  if (sigaddset(&mask, SIGSEGV)) goto error;
  if (sigaddset(&mask, SIGINT)) goto error;
 
  (void) arg;
  logMain(LOG_NOTICE, "please send me HUP, USR1 or TERM signals:");
  logMain(LOG_DEBUG, "- kill -SIGHUP %i", getpid());
  logMain(LOG_DEBUG, "- kill -SIGUSR1 %i", getpid());
  logMain(LOG_DEBUG, "- kill -SIGTERM %i", getpid());

  while (running) {

    // suspends  execution of the calling thread until one of the
    // signals in set is pending
    if ((sigNumber = sigwaitinfo(&mask, 0)) == -1) {
      logMain(LOG_ERR, "sigwait fails: %s", strerror(errno));
      goto error;
    }
  
    logMain(LOG_NOTICE, "signal reçu: %i", sigNumber);
    switch (sigNumber) {
    case SIGHUP:
      logMain(LOG_NOTICE, "=> HUP");
      break;
    case SIGUSR1:
      logMain(LOG_NOTICE, "=> SIGUSR1");
      break;
    case SIGTERM:
      logMain(LOG_NOTICE, "=> SIGTERM");
      running = FALSE;
      break;
    case SIGSEGV:
      logMain(LOG_NOTICE, "=> SIGSEGV");
      reEnableALL();
      kill(getpid(), SIGSEGV);
    case SIGINT:
      logMain(LOG_NOTICE, "=> SIGINT");
      reEnableALL();
      kill(getpid(), SIGINT);
      break;
    default:
      logMain(LOG_NOTICE, "=> ???");
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
  fprintf(stderr, "\n\t\t[ -a ]");

  miscOptions();
  fprintf(stderr, "  ---\n"
	  "  -r, --raise-alarm\tbe interrupted by alarm signal\n");
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
  int raiseAlarm = FALSE;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"r";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"raise-alarm", no_argument, 0, 'r'},
    {0, 0, 0, 0}
  };
  
  // import mdtx environment
  env = envUnitTest;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {      

    case 'r':
      raiseAlarm = TRUE; // be blocked and interrupted by alarm signal
      break;

      GET_MISC_OPTIONS;
    }
    if (rc) goto optError;
  }
  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  // test alarm
  if (!enableAlarm(sigalarmManager)) goto error;
  alarm(1);
  if (raiseAlarm) getchar();
  if (!disableAlarm()) goto error;

  // test threads "rendez-vous"
  if ((err = pthread_create(&thread, 0, empty, (void *)0))) {
    logMain(LOG_ERR, "pthread_create fails: %s", strerror(err));
    goto error;
  }

  if ((err = pthread_join(thread, 0))) {
    logMain(LOG_ERR, "pthread_join fails: %s", strerror(err));
    goto error;
  }

  // test thread signal manager
  if (!manageSignals(sigManager, &thread)) goto error;
  while (running) {
    usleep(200000);
  }

  if ((err = pthread_join(thread, 0))) {
    logMain(LOG_ERR, "pthread_join fails: %s", strerror(err));
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

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */


