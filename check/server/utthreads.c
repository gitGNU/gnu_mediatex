/*=======================================================================
 * Project: MediaTeX
 * Module : threads

 * Unit test for threads

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche
 
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
#include "server/mediatex-server.h"
#include "server/utFunc.h"

extern int taskSocketNumber;
extern int taskSignalNumber;


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
  logMain(LOG_NOTICE, "daemon exiting");
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
  logMain(LOG_NOTICE, "daemon wake-up");
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
  Configuration* conf = 0;
  int me = 0;
  long int rc = FALSE;
  ShmParam param;
  char bufWithZeros[REG_SHM_BUFF_SIZE+1];

  (void)arg;
  if (!(conf = getConfiguration())) goto error;
  me = taskSignalNumber;
  memset(bufWithZeros, '0', REG_SHM_BUFF_SIZE);
  bufWithZeros[REG_SHM_BUFF_SIZE] = 0; 
  
  if (!shmRead(conf->confFile, REG_SHM_BUFF_SIZE,
  	       mdtxShmRead, (void*)&param))
    goto error;

  logMain(LOG_INFO, "in  shm: (%s)", param.buf);  

  if (strcmp(param.buf, bufWithZeros)) {
    usleep(50000);
    logMain(LOG_NOTICE, "doing job %i for signal", me);
    usleep(50000);
    logMain(LOG_NOTICE, "finish job %i for signal", me);
    usleep(50000);
    strcpy(param.buf, bufWithZeros);
    rc = shmWrite(conf->confFile, REG_SHM_BUFF_SIZE,
    		  mdtxShmCopy, (void*)&param);
  }

  logMain(LOG_INFO, "out shm: (%s)", param.buf);  
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
  Connexion* connexion = 0;

  me = taskSocketNumber;
  connexion = (Connexion*)arg;

  //close(connexion->sock); // redondant
  usleep(50000);
  logMain(LOG_NOTICE, "doing job %i for %s", me, connexion->host);
  usleep(50000);
  logMain(LOG_NOTICE, "finish job %i for %s", me, connexion->host);
  usleep(50000); 

  rc = TRUE;
  socketJobEnds(connexion);
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
  env = envUnitTest;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
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

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
