/*=======================================================================
 * Version: $Id: mediatexd.c,v 1.14 2015/10/20 19:41:50 nroche Exp $
 * Project: MediaTeX
 * Module : server software
 *
 * Server software's main function

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

/*
  socket return codes:
  
  1--: negative reply
  2--: ok
  3--: bad request
  4--: internal error
*/

#include "mediatex-config.h"
#include "server/mediatex-server.h"

extern int taskSocketNumber;
extern int taskSignalNumber;


/*=======================================================================
 * Function   : signalJob
 * Description: Thread callback function for signals
 * Synopsis   : void* signalJob(void* arg)
 * Input      : void* arg = not used
 * Output     : (void*)TRUE on success
 =======================================================================*/
void* 
signalJob(void* arg)
{
  int me = taskSignalNumber;
  Configuration* conf = 0;
  Collection* coll = 0;
  long int rc = FALSE;
  int loop = FALSE;
  ShmParam param;
  int rc3 = MDTX_DONE;
  RGIT* curr = 0;

  (void) arg;
  logMain(LOG_DEBUG, "signalJob: %i", me);
  if (!(conf = getConfiguration())) goto error;

  do {
    int rc2 = MDTX_DONE;
    loop = FALSE;
    if (!shmRead(conf->confFile, MDTX_SHM_BUFF_SIZE,
		 mdtxShmRead, (void*)&param))
      goto error;
    
    if (param.buf[MDTX_SAVEMD5] == MDTX_QUERY) {
      logMain(LOG_NOTICE, "signalJob %i: SAVEMD5", me);

      // force writing
      while ((coll = rgNext_r(conf->collections, &curr))) {
	coll->fileState[iCACH] = MODIFIED;
      }
      if (!serverSaveAll()) rc2 = MDTX_ERROR;
      param.flag = MDTX_SAVEMD5;
      loop = TRUE;
      goto quit;
    }
    
    if (param.buf[MDTX_EXTRACT] == MDTX_QUERY) {
      logMain(LOG_NOTICE, "signalJob %i: EXTRACT", me);
      if (!serverLoop(extractArchives)) rc2 = MDTX_ERROR;
      param.flag = MDTX_EXTRACT;
      loop = TRUE;
      goto quit;
    }
    
    if (param.buf[MDTX_NOTIFY] == MDTX_QUERY) {
      logMain(LOG_NOTICE, "signalJob %i: NOTIFY", me);
      if (!serverLoop(sendRemoteNotify)) rc2 = MDTX_ERROR;
      param.flag = MDTX_NOTIFY;
      loop = TRUE;
      goto quit;
    }

    /* if (param.buf[MDTX_DELIVER] == MDTX_QUERY) { */
    /*   logMain(LOG_NOTICE, "signalJob %i: DELIVER", me); */
    /*   if (!serverLoop(deliverMails)) rc2 = MDTX_ERROR; */
    /*   param.flag = MDTX_DELIVER; */
    /*   loop = TRUE; */
    /*   goto quit; */
    /* } */

  quit:
    if (loop) {
      if (!shmWrite(conf->confFile, MDTX_SHM_BUFF_SIZE,
		    (rc2 == MDTX_DONE)?mdtxShmDisable:mdtxShmError,
		    (void*)&param))
	goto error;
    }
    if (rc2 == MDTX_ERROR) rc3 = MDTX_ERROR;
  } while (loop);

  rc = TRUE;
 error:
  if (rc) {
    if (rc3) {
      logMain(LOG_NOTICE, "signalJob %i: success", me);
    } else {
      logMain(LOG_ERR, "signalJob %i: fails", me);
    }
  } else {
    logMain(LOG_ERR, "signalJob %i: internal fails", me);
  }
  signalJobEnds();
  return (void*)rc;
}


/*=======================================================================
 * Function   : hupManager
 * Description: Reload configuration when receving an HUP signal
 * Synopsis   : void hupManager()
 * Input      : N/A
 * Output     : N/A
 * Note       : not rentrant and not designed to support concurrency
 =======================================================================*/
int
hupManager()
{
  int rc = FALSE;
  
  if (!serverSaveAll()) {
    logMain(LOG_ERR, "Fails to save md5sums before reloading");
    goto error;
  }

  freeConfiguration();

  if (!loadConfiguration(CFG)) {
    logMain(LOG_ERR, "HUP: fails to reload configuration");
    goto error;
  }
  
  if (!quickScanAll()) {
    logMain(LOG_ERR, "HUP: fails to scan caches");
    goto error;
  }

  rc = TRUE;
  logMain(LOG_NOTICE, "daemon is HUP");
 error:
  if (!rc) {
    logMain(LOG_ERR, "daemon fails to update: exiting");
  }
  return rc;
}


/*=======================================================================
 * Function   : termManager
 * Description: Exit when receiving TERM signal
 * Synopsis   : void termManager()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int
termManager()
{
  int rc = FALSE;

  if (!serverSaveAll()) {
    logMain(LOG_ERR, "Fails to save md5sums while exiting");
    goto error;
  }

  logMain(LOG_NOTICE, "mdtx-cache-daemon exiting");
  rc = TRUE;
 error:
  return rc;
}


/*=======================================================================
 * Function   : socketJob
 * Description: thread callback function
 * Synopsis   : void* socketJob(void* arg)
 * Input      : void* arg = (Connexion*) connection structure
 * Output     : N/A
 =======================================================================*/
void* 
socketJob(void* arg)
{
  int me = taskSocketNumber;
  long int rc = FALSE;
  Connexion* con = (Connexion*)arg;

  static char status[][32] = {
    "301 message parser error",
    "305 unknown message type: %s",
    "400 internal error",
  };

  logMain(LOG_DEBUG, "socketJob %i", me);
  sprintf(con->status, "%s", status[1]);

  // read the socket
  if ((con->message = parseRecords(con->sock)) == 0) {
    sprintf(con->status, "%s", status[0]);
    goto error;
  }

  // common checks on message
  if (!checkMessage(con)) goto error;

  logMain(LOG_INFO, "%s: %s message from %s server (%s)",
	  con->message->collection->label, 
	  strMessageType(con->message->messageType),
	  con->server->fingerPrint, con->host);

  switch (con->message->messageType) {

  case UPLOAD:
    logMain(LOG_NOTICE, "socketJob %i: UPLOAD", me);
    con->status[1] = '1';
    if (!uploadFinaleArchive(con)) goto error;
    break;

  case CGI:
    logMain(LOG_NOTICE, "socketJob %i: CGI", me);
    con->status[1] = '2';
    if (!cgiServer(con)) goto error;
    break;
    
  case HAVE:
    logMain(LOG_NOTICE, "socketJob %i: HAVE", me);
    con->status[1] = '3';
    if (!extractFinaleArchives(con)) goto error;
    break;
    
  case NOTIFY:
    logMain(LOG_NOTICE, "socketJob %i: NOTIFY (from %s)", me, con->host);
    con->status[1] = '4';
    if (!acceptRemoteNotify(con)) goto error ;
    break;
    
  default:
    sprintf(con->status, status[1], 
	    strMessageType(con->message->messageType));
    goto error;
  }
  
  logMain(LOG_NOTICE, "%s", con->status);
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", con->status);
    logMain(LOG_NOTICE, "still alive");
  }

  // send-back status code into the socket
  strcpy(con->status + strlen(con->status), "\n");
  tcpWrite(con->sock, con->status, strlen(con->status));
  con->message = destroyRecordTree(con->message);

  socketJobEnds(con);
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
  fprintf(stderr,
	  "`" PACKAGE_NAME "' "
	  "is an Electronic Record Management System (ERMS), "
	  "focusing on the archival storage entity define by the `OAIS' "
	  "draft and on the `NF Z 42-013' requirements.\n");

  mdtxUsage(programName);
  mdtxOptions();
  mdtxHelp();
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/11/04
 * Description: MediaTeX daemon
 * Synopsis   : ./mdtxd
 * Input      : sockets, shm and signals
 * Output     : rtfm
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
  env.allocDiseaseCallBack = serverDiseaseAll;
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
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (getuid() == 0) {
    logMain(LOG_ERR,  "do not lunch me as root");
    goto error;
  }
  if (!hupManager()) goto error;
  if (!mdtxCall(2, "adm", "bind")) goto error;
  if (!mainLoop()) goto error;
  /************************************************************************/
  
  if (!mdtxCall(2, "adm", "unbind")) goto error;
  rc = TRUE;
 error:
  freeConfiguration();
  ENDINGS;
  sleep(1);
  rc=!rc;
 optError:
  exit(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
