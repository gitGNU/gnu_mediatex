/*=======================================================================
 * Version: $Id: signals.c,v 1.5 2015/07/28 11:45:47 nroche Exp $
 * Project: MediaTeX
 * Module : signal
 *
 * Signal manager

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

#include "mediatex-config.h"

/*=======================================================================
 * Function   : disableALL
 * Description: Block all signal we will manage an other in dedicated 
 *              threads.
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
  sigset_t mask;

  // the calling thread will ignore the following signals:
  rc=rc&& (sigemptyset(&mask) == 0);
  rc=rc&& (sigaddset(&mask, SIGHUP) == 0);
  rc=rc&& (sigaddset(&mask, SIGUSR1) == 0);
  rc=rc&& (sigaddset(&mask, SIGTERM) == 0);
  rc=rc&& (sigaddset(&mask, SIGSEGV) == 0);
  rc=rc&& (sigaddset(&mask, SIGINT) == 0);
  rc=rc&& (sigaddset(&mask, SIGALRM) == 0);
  if (!rc) goto error;
  
  // The pthread_sigmask() function is just like sigprocmask(2), 
  // with the difference that its use in multithreaded programs is 
  // explicitly specified by POSIX.1-2001.
  if (pthread_sigmask(SIG_BLOCK, &mask, 0)) {
    logMisc(LOG_ERR, "sigprocmask fails: %s", strerror(errno));
    rc = FALSE;
  }

 error:
  if (!rc) {
    logMisc(LOG_ERR, "%s", "disableALL fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : reEnableALL
 * Description: Used to re-enable the defaut behaviour before exiting
 * Synopsis   : int reEnableALL()
 * Input      : N/A
 * Output     : True on success
 =======================================================================*/
int 
reEnableALL()
{
  int rc = TRUE;
  sigset_t mask;

  // the calling thread will receive again the following signals:
  rc=rc&& (sigemptyset(&mask) == 0);
  rc=rc&& (sigaddset(&mask, SIGHUP) == 0);
  rc=rc&& (sigaddset(&mask, SIGUSR1) == 0);
  rc=rc&& (sigaddset(&mask, SIGTERM) == 0);
  rc=rc&& (sigaddset(&mask, SIGSEGV) == 0);
  rc=rc&& (sigaddset(&mask, SIGINT) == 0);
  rc=rc&& (sigaddset(&mask, SIGALRM) == 0);
  if (!rc) goto error;

  if (pthread_sigmask(SIG_UNBLOCK, &mask, 0)) {
    logMisc(LOG_ERR, "sigprocmask fails: %s", strerror(errno));
    rc = FALSE;
  }

 error:
  if (!rc) {
    logMisc(LOG_ERR, "%s", "disableALL fails");
  }
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
enableAlarm(void (*sigalarmManager)(int))
{
  int rc = FALSE;
  sigset_t mask;
  struct sigaction action;
  int err = 0;

  // set the alarm manager
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  action.sa_handler = sigalarmManager;
  if (sigaction(SIGALRM, &action, 0) != 0) {
    logMisc(LOG_ERR, "%s", "sigaction fails: %s", strerror(errno));
    goto error;
  }

  if (sigemptyset(&mask)) goto error;
  if (sigaddset(&mask, SIGALRM)) goto error;

  if ((err = pthread_sigmask(SIG_UNBLOCK, &mask, 0))) {
    logMisc(LOG_ERR, "pthread_sigmask fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "%s", "enableAlarm fails");
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
  sigset_t mask;
  int err = 0;

  if (sigemptyset(&mask)) goto error;
  if (sigaddset(&mask, SIGALRM)) goto error;;

  if ((err = pthread_sigmask(SIG_BLOCK, &mask, 0))) {
    logMisc(LOG_ERR, "pthread_sigmask fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "%s", "disableAlarm fails");
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
  int rc = FALSE;
  int err = 0;

  logMisc(LOG_DEBUG, "%s", "manageSignals");
 
  // current thread no more handle 
  //  SIGHUP, SIGUSR1, SIGTERM, SIGSEGV, SIGINT and SIGALRM
  if (!disableALL()) goto error;

  // new thread that will have to explicitely manage (or not) blocked 
  // signals using sigwaitinfo
  if ((err = pthread_create(thread, 0, manager, (void *)0))) {
    logMisc(LOG_ERR, "pthread_create fails: %s", strerror(err));
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "%s", "manageSignals fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */


