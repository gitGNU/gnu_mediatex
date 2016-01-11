/*=======================================================================
 * Version: $Id: utserv.c,v 1.5 2015/10/20 19:41:42 nroche Exp $
 * Project: MediaTeX
 * Module : wrapper/serv
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

#include "mediatex.h"
#include "client/mediatex-client.h"

int running = TRUE;

void* 
sigManager(void* arg)
{
  int sigNumber = 0;
  sigset_t mask;
  (void) arg;

  sigemptyset(&mask);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGUSR1);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGSEGV);
  sigaddset(&mask, SIGINT);

  logMain(LOG_NOTICE, "please send me HUP, USR1 or TERM signals:");
  logMain(LOG_NOTICE, "- kill -SIGHUP %i", getpid());
  logMain(LOG_NOTICE, "- kill -SIGUSR1 %i", getpid());
  logMain(LOG_NOTICE, "- kill -SIGTERM %i", getpid());

  if ((sigNumber = sigwaitinfo(&mask, 0)) == -1) {
    logMain(LOG_ERR, "sigwait fails: %s", strerror(errno));
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
  Collection* coll = 0;
  pthread_t thread;
  int doLock = FALSE;
  int err = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"k";
  struct option longOptions[] = {
    {"lock", no_argument, 0, 'k'},
    MDTX_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
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
    if ((err = pthread_join(thread, 0))) {
      logMain(LOG_ERR, "pthread_join fails: %s", strerror(err));
      goto error;
    }
  }

  // others tests
  if (!(coll = addCollection("coll1"))) goto error;

  logMain(LOG_NOTICE, "*** upgrade:");
  env.noCollCvs = FALSE;
  if (!mdtxUpgrade("coll1")) goto error;

  logMain(LOG_NOTICE, "*** refuse to del localhost key: ");
  if (delKey("coll1", "746d6ceeb76e05cfa2dea92a1c5753cd")) goto error;
    
  logMain(LOG_NOTICE, "*** del a key: ");
  if (!delKey("coll1", "bedac32422739d7eced624ba20f5912e")) goto error;
  
  logMain(LOG_NOTICE, "*** refuse to add localhost key: ");
  if (addKey("coll1", "client/user1Key_rsa.pub")) goto error;

  logMain(LOG_NOTICE, "*** add a key: ");
  if (!addKey("coll1", "client/user3Key_dsa.pub")) goto error;

  logMain(LOG_NOTICE, "*** save and disease test: ");
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

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
