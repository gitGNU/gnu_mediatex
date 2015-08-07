/*=======================================================================
 * Version: $Id: register.c,v 1.5 2015/08/07 17:50:29 nroche Exp $
 * Project: MediaTeX
 * Module : bus/register
 
 * Manage signals from client to server using registers

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
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_WARNING, "mdtxAsyncSignal fails");
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
    logCommon(LOG_WARNING, "server fails to perform your query");
  }
 error:
  if (!rc) {
    logCommon(LOG_ERR, "mdtxSyncSignal fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
