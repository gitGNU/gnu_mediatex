/*=======================================================================
 * Version: $Id: utlocks.c,v 1.1 2015/07/01 10:49:55 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
 *
 * locks on files

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
GLOBAL_STRUCT_DEF;

int running = TRUE;

void* 
sigManager(void* arg)
{
  int sigNumber = 0;
  sigset_t mask;
  int rc = TRUE;

  // signal we are looking for:
  rc=rc&& (sigemptyset(&mask) == 0);
  rc=rc&& (sigaddset(&mask, SIGHUP) == 0);
  rc=rc&& (sigaddset(&mask, SIGUSR1) == 0);
  rc=rc&& (sigaddset(&mask, SIGTERM) == 0);
  rc=rc&& (sigaddset(&mask, SIGSEGV) == 0);
  rc=rc&& (sigaddset(&mask, SIGINT) == 0);
  if (!rc) goto error;

  (void) arg;
  logEmit(LOG_NOTICE, "%s", "please send me HUP, USR1 or TERM signals:");
  logEmit(LOG_NOTICE, "- kill -SIGHUP %i", getpid());
  logEmit(LOG_NOTICE, "- kill -SIGUSR1 %i", getpid());
  logEmit(LOG_NOTICE, "- kill -SIGTERM %i", getpid());

  if ((sigNumber = sigwaitinfo(&mask, 0)) == -1) {
    logEmit(LOG_ERR, "sigwait fails: %s", strerror(errno));
    rc = FALSE;
  }
 
 error:
  if (!rc) {
    logEmit(LOG_ERR, "sigManager fails");
  }
  running = FALSE;
  return (void*)rc;
}

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static 
void usage(char* programName)
{
  miscUsage(programName);
  fprintf(stderr, "\n\t\t -i file -p perm");

  miscOptions();
  fprintf(stderr, "  ---\n"
	  "  -i, --input-file\tinput device to compute checksums on\n"
	  "  -p, --perm\t\tread/write mode: R or W\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for locks module
 * Synopsis   : ./utlocks -d ici -u toto -g toto -p 777
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputPath = 0;
  int mode = F_UNLCK;
  int fd = -1;
  pthread_t thread;
  int err = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"i:p:";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"input-file", required_argument, 0, 'i'},
    {"perm", required_argument, 0, 'p'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'i':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the input device\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((inputPath = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the input device path: %s\n", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(inputPath, optarg, strlen(optarg)+1);
      break;

    case 'p':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the permission\n",
		programName);
	rc = EINVAL;
	break;
      }
      switch (*optarg) {
      case 'r':
      case 'R':
	mode = F_RDLCK;
	break;
      case 'w':
      case 'W':
	mode = F_WRLCK;
	break;
      default:
	fprintf(stderr, "unknown lock mode: '%s'\n", optarg);
	rc = EINVAL;
      }
      break;

      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  /*
  logEmit(LOG_DEBUG, "%s", "For information, constants are:");
  logEmit(LOG_DEBUG, "F_UNLCK = %i", F_UNLCK);
  logEmit(LOG_DEBUG, "F_RDLCK = %i", F_RDLCK);
  logEmit(LOG_DEBUG, "F_WRLCK = %i", F_WRLCK);
  */

  if (inputPath == 0) {
    usage(programName);
    logEmit(LOG_ERR, "%s", "Please provide a file to lock");
    goto error;
  }

  if (mode != F_RDLCK && mode != F_WRLCK) {
    usage(programName);
    logEmit(LOG_ERR, "%s", "Please provide a lock mode");
    goto error;
  }
  
  // file descriptor to lock 
  // (the opening mode must be compatible with lock mode)
  if ((fd = open(inputPath, O_RDWR)) < 0) {
    logEmit(LOG_ERR, "open failed on %s: %s", inputPath, strerror(errno));
    goto error;
  }

  // lock
  if (!lock(fd, mode)) goto error;

  // infinite loop (file descriptor should not be available)
  if (!manageSignals(sigManager, &thread)) goto error;
  while (running) usleep(200000);

  // unlock
  if (!unLock(fd)) goto error;

  if ((err = pthread_join(thread, 0))) {
    logEmit(LOG_ERR, "pthread_join fails: %s", strerror(err));
    goto error;
  }
  /************************************************************************/

  rc = TRUE;
 error:
  if (inputPath) free(inputPath);
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