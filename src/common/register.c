/*=======================================================================
 * Version: $Id: register.c,v 1.3 2015/06/03 14:03:34 nroche Exp $
 * Project: MediaTeX
 * Module : bus/register
 
 * Manage simple interaction between wrapper and server

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

#include "../misc/log.h"
#include "../misc/shm.h"
#include "../memory/strdsm.h"
#include "../memory/confTree.h"
#include "register.h"

#include <signal.h>

/*=======================================================================
 * Function   : mdtxShmCopy
 * Description: SHM callback function
 *              Erase SHM memory by copying input buffer.
 * Synopsis   : static void mdtxShmCopy(void *buffer, size, void* arg)
 * Input      : void *buffer: SHM buffer
 *              int shmSize: SHM size
 *              void* arg:    input from pointer on ShmParam 
 * Output     : N/A
 =======================================================================*/
void 
mdtxShmCopy(void *buffer, int shmSize, void* arg) 
{
  ShmParam* param = (ShmParam*)arg;

  strncpy((char*)buffer, param->buf, shmSize);
}

/*=======================================================================
 * Function   : mdtxShmRead
 * Description: SHM callback function
 *              Read SHM memory by copying it into the input buffer.
 * Synopsis   : void mdtxShmRead(void *buffer, size, void* arg) 
 * Input      : void *buffer: SHM buffer
 *              int shmSize: SHM size
 *              void* arg:    input from pointer on ShmParam 
 * Output     : N/A
 =======================================================================*/
void 
mdtxShmRead(void *buffer, int shmSize, void* arg) 
{
  ShmParam* param = (ShmParam*)arg;

  strncpy(param->buf, (char*)buffer, shmSize);
  param->buf[MDTX_SHM_BUFF_SIZE] = (char)0;
}
  
/*=======================================================================
 * Function   : mdtxShmEnable
 * Description: SHM callback function
 *              Set one byte to 1 in SHM memory
 * Synopsis   : void mdtxShmEnable(void *buffer, void* arg)
 * Input      : void *buffer: SHM buffer
 *              int shmSize: SHM size
 *              void* arg:    input from pointer on ShmParam 
 * Output     : N/A
 =======================================================================*/
static void 
mdtxShmEnable(void *buffer, int shmSize, void* arg) 
{
  ShmParam* param = (ShmParam*)arg;
  (void) shmSize;

  ((char*)buffer)[param->flag] = MDTX_QUERY;
}

/*=======================================================================
 * Function   : mdtxShmDisable
 * Description: SHM callback function
 *              Set one byte to 0 in SHM memory
 * Synopsis   : void mdtxShmDisable(void *buffer, void* arg)
 * Input      : void *buffer: SHM buffer
 *              int shmSize: SHM size
 *              void* arg:    input from pointer on ShmParam 
 * Output     : N/A
 =======================================================================*/
void 
mdtxShmDisable(void *buffer, int shmSize, void* arg) 
{
  ShmParam* param = (ShmParam*)arg;
  (void) shmSize;

  ((char*)buffer)[param->flag] = MDTX_DONE;
}

/*=======================================================================
 * Function   : mdtxShmError
 * Description: SHM callback function
 *              Set one byte to 2 in SHM memory
 * Synopsis   : void mdtxShmDisable(void *buffer, void* arg)
 * Input      : void *buffer: SHM buffer
 *              int shmSize: SHM size
 *              void* arg:    input from pointer on ShmParam 
 * Output     : N/A
 =======================================================================*/
void 
mdtxShmError(void *buffer, int shmSize, void* arg) 
{
  ShmParam* param = (ShmParam*)arg;
  (void) shmSize;

  ((char*)buffer)[param->flag] = MDTX_ERROR;
}

/*=======================================================================
 * Function   : mdtxShmInitialize
 * Description: Allocate and initialize the share memory to 0
 * Synopsis   : int mdtxShmInitialize()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxShmInitialize()
{
  static char* buffer = "0000\0";
  ShmParam param;

  strncpy(param.buf, buffer, MDTX_SHM_BUFF_SIZE);
  logCommon(LOG_NOTICE, "Initialise SHM using %s conf file", env.confLabel);
  return shmWrite(getConfiguration()->confFile, MDTX_SHM_BUFF_SIZE, 
		  mdtxShmCopy, (void*)&param);
}

/*=======================================================================
 * Function   : mdtxShmFree
 * Description: Free the share memory
 * Synopsis   : int mdtxShmFree()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int
mdtxShmFree()
{
  return shmFree(getConfiguration()->confFile, MDTX_SHM_BUFF_SIZE);
}

/*=======================================================================
 * Function   : mdtxAsynchronousSignal
 * Description: Send a signal to the server
 * Synopsis   : int mdtxSimpleSignal(int signal)
 * Input      : int signal = signal to send to server (HUP if 0)
 * Output     : N/A
 =======================================================================*/
int 
mdtxAsyncSignal(int signal)
{
  int rc = FALSE;
  Configuration* conf = 0;
  FILE* fd = 0;
  int pid;

  // send HUP signal by default
  if (signal == 0) signal = SIGHUP;
  logCommon(LOG_DEBUG, "mdtxAsyncSignal %i", signal);

  // retrieve daemon's pid
  if (!(conf = getConfiguration())) goto error;
  if ((fd = fopen(conf->pidFile, "r")) == 0) {
    logCommon(LOG_INFO, "open %s failed: %s", 
	    conf->pidFile, strerror(errno));
    goto error;
  }

  if (fscanf(fd, "%i", &pid) != 1) {
    logCommon(LOG_INFO, "cannot retrieve pid from %s", conf->pidFile);
    goto error;
  }
  if (fclose(fd) != 0) {
    logCommon(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error;
  }

  // send SIGUSR1 to daemon
  logCommon(LOG_INFO, "sending signal %i to %i", signal, pid);

  if (kill(pid, signal) == -1) {
    logCommon(LOG_ERR, "kill signal to daemon fails: %s", strerror(errno));
    //logCommon(LOG_DEBUG, "%s", "(you need to be logged as mdtx user)");
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_WARNING, "%s", "mdtxAsyncSignal fails");
  }
return rc;
}

/*=======================================================================
 * Function   : mdtxSynchronousSignal
 * Description: Set a register, send SIGUSR1 signal to the daemon and
 *              wait until this register is unset by the daemon.
 * 
 * Synopsis   : int mdtxSignal(int flag)
 * Input      : int flag = register number
 * Output     : N/A
 =======================================================================*/
int mdtxSyncSignal(int flag)
{
  int rc = FALSE;
  Configuration* conf = 0;
  ShmParam param;

  // test if there is or not a pid file
  if (!(conf = getConfiguration())) goto error;
  logCommon(LOG_DEBUG, "mdtxSyncSignal %i", flag);

  if (access(conf->pidFile, R_OK) == -1) {
    logCommon(LOG_WARNING, "access %s failed: %s", 
	    conf->pidFile, strerror(errno));
    logCommon(LOG_WARNING, "daemon looks stopped", flag);
    rc = TRUE;
    goto error;
  }

  // Read register
  if (!shmRead(conf->confFile, MDTX_SHM_BUFF_SIZE, 
	       mdtxShmRead, (void*)&param))
      goto error;

  // Do no set if register is already set
  if (param.buf[flag] == MDTX_QUERY) goto quit;

  // Set register
  logCommon(LOG_INFO, "setting %i register", flag);
  param.flag = flag;
  if (!shmWrite(conf->confFile, MDTX_SHM_BUFF_SIZE, 
		mdtxShmEnable, (void*)&param))
    goto error;

 quit:
  // Send SIGUSR1 signal to the daemon
  if (!mdtxAsyncSignal(SIGUSR1)) goto error;

  // Wait until this register is unset by the daemon
  logCommon(LOG_INFO, "waiting for %i register unset", flag);
  do {
    usleep(200000);
    rc = shmRead(conf->confFile, MDTX_SHM_BUFF_SIZE, 
		 mdtxShmRead, (void*)&param);
  }
  while (rc && (param.buf[flag] == MDTX_QUERY));

  rc = TRUE;
  if (param.buf[flag] != MDTX_DONE) {
    logCommon(LOG_WARNING, "%s", "server fails to perform your query");
  }
 error:
  if (!rc) {
    logCommon(LOG_ERR, "%s", "mdtxSyncSignal fails");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"

#define UNDEF -1
#define INIT  10
#define GET   11
#define FREE  12
#define ERROR 13
GLOBAL_STRUCT_DEF;

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
  fprintf(stderr, "\n\t\t{ -I | -G | -F | -S | -E | -N | -D | -e }");

  mdtxOptions();
  fprintf(stderr, "  ---\n"
	  "  -I, --initialize\tinitialize share memory\n"
	  "  -G, --read-shm\tread share memory\n"
	  "  -F, --free-shm\tfree share memory\n"
	  "  -W, --do-save\t\tsave md5sums.txt file\n"
	  "  -E, --do-extract\tperform extracton\n"
	  "  -N, --do-notify\tperform notification\n"
	  "  -D, --do-deliver\tperform deliver\n"
	  "  -e, --do-errorx\terror test\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for register module.
 * Synopsis   : ./utregister
 * Input      : { -I | -G | -F | -5 | -X | -N | -S }
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  int signal = UNDEF;
  ShmParam param;
  int doError = FALSE;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"IGFWENDe";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"initialize", required_argument, 0, 'I'},
    {"read-shm", required_argument, 0, 'G'},
    {"free-shm", required_argument, 0, 'F'},
    {"do-save", required_argument, 0, 'W'},
    {"do-extract", required_argument, 0, 'E'},
    {"do-notify", required_argument, 0, 'N'},
    {"do-deliver", required_argument, 0, 'D'},
    {"do-error", required_argument, 0, 'e'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.debugCommon = TRUE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {

    case 'I':
      if (signal != UNDEF) rc=1;
      signal = INIT;
      break;

    case 'G':
      if (signal != UNDEF) rc=2;
      signal = GET;
      break;

    case 'F':
      if (signal != UNDEF) rc=3;
      signal = FREE;
      break;

    case 'W':
      if (signal != UNDEF) rc=4;
      signal = MDTX_SAVEMD5;
      break;

    case 'E':
      if (signal != UNDEF) rc=5;
      signal = MDTX_EXTRACT;
      break;

    case 'N':
      if (signal != UNDEF) rc=6;
      signal = MDTX_NOTIFY;
      break;

    case 'D':
      if (signal != UNDEF) rc=7;
      signal = MDTX_DELIVER;
      break;

    case 'e':
      doError = TRUE;
      break;

      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  usleep(50000);
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  switch (signal) {
  case INIT:
    rc = mdtxShmInitialize();
    break;
  case GET:
    if (!(rc = shmRead(getConfiguration()->confFile, MDTX_SHM_BUFF_SIZE,
		       mdtxShmRead, (void*)&param)))
      goto error;
    printf("=> %s\n", param.buf);
    break;
  case FREE:
    rc = mdtxShmFree();
    break;
  case UNDEF:
    usage(programName);
    break;
  default:
    if (doError) {
      param.flag = signal;
      if (!(rc = shmWrite(getConfiguration()->confFile, MDTX_SHM_BUFF_SIZE,
			  mdtxShmError, (void*)&param)))
	goto error;
    }
    else {
      rc = mdtxSyncSignal(signal);
    }
  }
  /************************************************************************/

  freeConfiguration();
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
