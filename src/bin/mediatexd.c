/*=======================================================================
 * Version: $Id: mediatexd.c,v 1.2 2014/11/13 16:36:16 nroche Exp $
 * Project: MediaTeX
 * Module : server software
 *
 * Server software's main function

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
#include "../misc/command.h"
#include "../misc/setuid.h"
#include "../misc/signals.h"
#include "../misc/tcp.h"
#include "../misc/shm.h"
#include "../parser/confFile.tab.h"
#include "../parser/extractFile.tab.h"
#include "../parser/recordList.tab.h"
#include "../common/register.h"
#include "../common/connect.h"
#include "../common/openClose.h"
#include "../server/cache.h"
#include "../server/deliver.h"
#include "../server/extract.h"
#include "../server/cgiSrv.h"
#include "../server/have.h"
#include "../server/notify.h"

#include <pthread.h>
#include <errno.h>
#include <sys/types.h> // 
#include <sys/stat.h>  // open 
#include <fcntl.h>     //

GLOBAL_STRUCT_DEF_BIN;

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
  Configuration* conf = NULL;
  long int rc = FALSE;
  int loop = FALSE;
  ShmParam param;
  int rc3 = MDTX_DONE;

  (void) arg;
  logEmit(LOG_DEBUG, "signalJob: %i", me);
  if (!(conf = getConfiguration())) goto error;

  do {
    int rc2 = MDTX_DONE;
    loop = FALSE;
    if (!shmRead(conf->confFile, MDTX_SHM_BUFF_SIZE,
		 mdtxShmRead, (void*)&param))
      goto error;
    
    if (param.buf[MDTX_SAVEMD5] == MDTX_QUERY) {
      logEmit(LOG_NOTICE, "signalJob %i: SAVEMD5", me);
      if (!serverSaveAll()) rc2 = MDTX_ERROR;
      param.flag = MDTX_SAVEMD5;
      loop = TRUE;
      goto quit;
    }
    
    if (param.buf[MDTX_EXTRACT] == MDTX_QUERY) {
      logEmit(LOG_NOTICE, "signalJob %i: EXTRACT", me);
      if (!serverLoop(extractArchives)) rc2 = MDTX_ERROR;
      param.flag = MDTX_EXTRACT;
      loop = TRUE;
      goto quit;
    }
    
    if (param.buf[MDTX_NOTIFY] == MDTX_QUERY) {
      logEmit(LOG_NOTICE, "signalJob %i: NOTIFY", me);
      if (!serverLoop(sendRemoteNotify)) rc2 = MDTX_ERROR;
      param.flag = MDTX_NOTIFY;
      loop = TRUE;
      goto quit;
    }

    if (param.buf[MDTX_DELIVER] == MDTX_QUERY) {
      logEmit(LOG_NOTICE, "signalJob %i: DELIVER", me);
      if (!serverLoop(deliverMails)) rc2 = MDTX_ERROR;
      param.flag = MDTX_DELIVER;
      loop = TRUE;
      goto quit;
    }

  quit:
    if (loop) {
      if (!shmWrite(conf->confFile, MDTX_SHM_BUFF_SIZE,
		    (rc2 == MDTX_DONE)?mdtxShmDisable:mdtxShmError,
		    (void*)&param))
	goto error;
    }
    if (rc2 == MDTX_ERROR) rc3 = MDTX_ERROR;
  } while (loop);


  if (!serverLoop(cleanCacheTree)) goto error;
  rc = TRUE;
 error:
  if (rc) {
    if (rc3) {
      logEmit(LOG_NOTICE, "signalJob %i: success", me);
    } else {
      logEmit(LOG_ERR, "signalJob %i: fails", me);
    }
  } else {
    logEmit(LOG_ERR, "signalJob %i: internal fails", me);
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
    logEmit(LOG_ERR, "%s", "Fails to save md5sums before reloading");
    goto error;
  }

  freeConfiguration();

  if (!loadConfiguration(CONF)) {
    logEmit(LOG_ERR, "%s", "HUP: fails to reload configuration");
    goto error;
  }
  
  if (!quickScanAll()) {
    logEmit(LOG_ERR, "%s", "HUP: fails to scan caches");
    goto error;
  }

  rc = TRUE;
  logEmit(LOG_NOTICE, "%s", "daemon is HUP");
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "daemon fails to update: exiting");
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
    logEmit(LOG_ERR, "%s", "Fails to save md5sums while exiting");
    goto error;
  }

  logEmit(LOG_NOTICE, "%s", "mdtx-cache-daemon exiting");
  rc = TRUE;
 error:
  return rc;
}


/*=======================================================================
 * Function   : matchServer
 * Description: match a server from IP
 * Synopsis   : void* socketJob(void* arg)
 * Input      : void* arg = (Connexion*) connection structure
 * Output     : N/A
 =======================================================================*/
int matchServer(RecordTree* tree, Connexion* connexion)
{
  int rc = FALSE;
  Collection* coll = NULL;
  Server* server = NULL;
  //Server* relayed = NULL;
  Record* record = NULL;
  RGIT* curr = NULL;
  struct tm date;

  coll = tree->collection;
  connexion->server = NULL;
  checkCollection(coll);
  logEmit(LOG_DEBUG, "%s", "matchServer");

  if (tree->messageType != NOTIFY && isEmptyRing(tree->records)) {
    logEmit(LOG_INFO, "%s", "receive empty ring");
    goto error;
  }

  // match server from fingerprint in message header
  // (may be masqueraded by a Nat server)
  while((server = rgNext_r(coll->serverTree->servers, &curr))) {
    if (!(strncmp(tree->fingerPrint, server->fingerPrint, MAX_SIZE_HASH)))
      break;
  }
  if (server == NULL) {
    logEmit(LOG_WARNING, 
	    "server %s is not register into collection %s",
	    connexion->host, coll->label);
    goto error;
  }

  // display message for debugging
  logEmit(LOG_INFO, "receive message about server %s (%s)", 
	  server->host, server->fingerPrint);

  if (isEmptyRing(tree->records)) {
    logEmit(LOG_INFO, "%s", "receive empty content");
  }
  else {
    curr = NULL;
    while((record = rgNext_r(tree->records, &curr))) {
      localtime_r(&record->date, &date);
      logEmit(LOG_INFO, "%c "
	      "%04i-%02i-%02i,%02i:%02i:%02i "
	      "%*s %*s %*llu %s\n",
	      (record->type & 0x3) == DEMAND?'D':
	      (record->type & 0x3) == SUPPLY?'S':'?',
	      date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	      date.tm_hour, date.tm_min, date.tm_sec,
	      MAX_SIZE_HASH, record->server->fingerPrint, 
	      MAX_SIZE_HASH, record->archive->hash, 
	      MAX_SIZE_SIZE, (long long unsigned int)record->archive->size,
	      record->extra?record->extra:"");
    }
  }    

  /* // match a Nat server */
  /* if (rgHaveItem(coll->localhost->natServers, server)) { */
  /*   logEmit(LOG_NOTICE, "%s", "receive message from Nat server"); */

  /*   // note: get a private incoming IP so we cannot match it. */
  /*   // just check all records have the same server's fingerprint */
  /*   curr = NULL; */
  /*   relayed = rgNext_r(tree->records, &curr); */
  /*   logEmit(LOG_NOTICE, "%s", "original message from %s (%s)", */
  /* 	    relayed->host, relayed->fingerPrint); */
  /*   while((record = rgNext_r(tree->records, &curr))) { */
  /*     if (relayed != record->server) { */
  /* 	logEmit(LOG_WARNING,  */
  /* 		"message contains records related to other host: %s", */
  /* 		record->server->fingerPrint); */
  /* 	goto error; */
  /*     } */
  /*   } */
  /*   goto end; */
  /* } */

  /* // match server from incomming IP (only if not 127.0.0.1) */
  /* if (connexion->ipv4 != ntohl(0x0100007f)) { */
  /*   if (server->address.sin_family == 0 &&  */
  /* 	!buildServerAddress(server)) goto error; */
  /*   if (ntohl(server->address.sin_addr.s_addr) != connexion->ipv4) { */
  /*     logEmit(LOG_WARNING,  */
  /* 	      "ip from %s do not match server fingerprint on message: %s", */
  /* 	      connexion->host, record->server->fingerPrint); */
  /*     goto error; */
  /*   } */
  /* } */
  
  // check all records are related to the calling server 
  curr = NULL;
  if (isEmptyRing(tree->records)) {
    while((record = rgNext_r(tree->records, &curr))) {
      if (server != record->server) {
	logEmit(LOG_WARNING, 
		"message contains a record related to an other host: %s",
		record->server->fingerPrint);
	goto error;
      }
    }
  }
  
  //end:
  connexion->server = server;
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "matchServer fails");
  }
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
  Connexion* connexion = NULL;
  RecordTree* tree = NULL;

  connexion = (Connexion*)arg; // to be free as we own it as a thread
  logEmit(LOG_DEBUG, "socketJob %i", me);
  
  // read the socket
  if ((tree = parseRecordList(connexion->sock))
      == NULL) {
    logEmit(LOG_ERR, "%s", "fail to parse RecordTree");
    goto error;
  }

  // match server
  checkCollection(tree->collection);
  if (!loadCollection(tree->collection, SERV)) goto error;
  if (!getLocalHost(tree->collection)) goto error2; // minimal upgrade
  if (!(matchServer(tree, connexion))) goto error2;

  logEmit(LOG_INFO, "%s: %s message from %s server (%s)",
	  tree->collection->label, strMessageType(tree->messageType),
	  connexion->server->fingerPrint, connexion->host);

  switch (tree->messageType) {

  case UPLOAD:
    logEmit(LOG_NOTICE, "socketJob %i: UPLOAD", me);
    if (!uploadFinaleArchive(tree, connexion)) goto error2;
    break;

  case CGI:
    logEmit(LOG_NOTICE, "socketJob %i: CGI", me);
    if (!cgiServer(tree, connexion)) goto error2;
    break;
    
  case HAVE:
    logEmit(LOG_NOTICE, "socketJob %i: HAVE", me);
    if (!extractFinaleArchives(tree, connexion)) goto error2;
    break;
    
  case NOTIFY:
    logEmit(LOG_NOTICE, "socketJob %i: NOTIFY (from %s)",
	    me, connexion->host);
    if (!acceptRemoteNotify(tree, connexion)) goto error2;
    break;
    
  default:
    logEmit(LOG_INFO, "server do not accept %s messages",
	    strMessageType(tree->messageType));
    goto error2;
  }
  
  if (!cleanCacheTree(tree->collection)) goto error;
  rc = TRUE;
 error2:
  if (!releaseCollection(tree->collection, SERV)) rc = FALSE;
 error:
  if (rc) {
    logEmit(LOG_NOTICE, "socketJob %i: success", me);
  } else {
    logEmit(LOG_ERR, "socketJob %i: fails", me);
  }
  tree = destroyRecordTree(tree);
  
  // connexion variable is manage by this thread
  if (connexion) {
    if (connexion->sock) close(connexion->sock);
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
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
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
    logEmit(LOG_ERR, "%s",  "do not lunch me as root");
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
