/*=======================================================================
 * Version: $Id: mediatexd.c,v 1.4 2015/08/09 11:12:35 nroche Exp $
 * Project: MediaTeX
 * Module : server software
 *
 * Server software's main function

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
#include "server/mediatex-server.h"
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
  Configuration* conf = 0;
  long int rc = FALSE;
  int loop = FALSE;
  ShmParam param;
  int rc3 = MDTX_DONE;

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


  if (!serverLoop(cleanCacheTree)) goto error;
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
 * Function   : matchServer
 * Description: match a server from IP
 * Synopsis   : void* socketJob(void* arg)
 * Input      : void* arg = (Connexion*) connection structure
 * Output     : N/A
 =======================================================================*/
int matchServer(RecordTree* tree, Connexion* connexion)
{
  int rc = FALSE;
  Collection* coll = 0;
  Server* server = 0;
  //Server* relayed = 0;
  Record* record = 0;
  RGIT* curr = 0;
  struct tm date;

  coll = tree->collection;
  connexion->server = 0;
  checkCollection(coll);
  logMain(LOG_DEBUG, "matchServer");

  if (tree->messageType != NOTIFY && isEmptyRing(tree->records)) {
    logMain(LOG_INFO, "receive empty ring");
    goto error;
  }

  // match server from fingerprint in message header
  // (may be masqueraded by a Nat server)
  while((server = rgNext_r(coll->serverTree->servers, &curr))) {
    if (!(strncmp(tree->fingerPrint, server->fingerPrint, MAX_SIZE_HASH)))
      break;
  }
  if (server == 0) {
    logMain(LOG_WARNING, 
	    "server %s is not register into collection %s",
	    connexion->host, coll->label);
    goto error;
  }

  // display message for debugging
  logMain(LOG_INFO, "receive message about server %s (%s)", 
	  server->host, server->fingerPrint);

  if (isEmptyRing(tree->records)) {
    logMain(LOG_INFO, "receive empty content");
  }
  else {
    curr = 0;
    while((record = rgNext_r(tree->records, &curr))) {
      localtime_r(&record->date, &date);
      logMain(LOG_INFO, "%c "
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
  /*   logMain(LOG_NOTICE, "receive message from Nat server"); */

  /*   // note: get a private incoming IP so we cannot match it. */
  /*   // just check all records have the same server's fingerprint */
  /*   curr = 0; */
  /*   relayed = rgNext_r(tree->records, &curr); */
  /*   logMain(LOG_NOTICE, "original message from %s (%s)", */
  /* 	    relayed->host, relayed->fingerPrint); */
  /*   while((record = rgNext_r(tree->records, &curr))) { */
  /*     if (relayed != record->server) { */
  /* 	logMain(LOG_WARNING,  */
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
  /*     logMain(LOG_WARNING,  */
  /* 	      "ip from %s do not match server fingerprint on message: %s", */
  /* 	      connexion->host, record->server->fingerPrint); */
  /*     goto error; */
  /*   } */
  /* } */
  
  // check all records are related to the calling server 
  curr = 0;
  if (isEmptyRing(tree->records)) {
    while((record = rgNext_r(tree->records, &curr))) {
      if (server != record->server) {
	logMain(LOG_WARNING, 
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
    logMain(LOG_ERR, "matchServer fails");
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
  Connexion* connexion = 0;
  RecordTree* tree = 0;

  connexion = (Connexion*)arg; // to be free as we own it as a thread
  logMain(LOG_DEBUG, "socketJob %i", me);
  
  // read the socket
  if ((tree = parseRecords(connexion->sock))
      == 0) {
    logMain(LOG_ERR, "fail to parse RecordTree");
    goto error;
  }

  // match server
  checkCollection(tree->collection);
  if (!loadCollection(tree->collection, SERV)) goto error;
  if (!getLocalHost(tree->collection)) goto error2; // minimal upgrade
  if (!(matchServer(tree, connexion))) goto error2;

  logMain(LOG_INFO, "%s: %s message from %s server (%s)",
	  tree->collection->label, strMessageType(tree->messageType),
	  connexion->server->fingerPrint, connexion->host);

  switch (tree->messageType) {

  case UPLOAD:
    logMain(LOG_NOTICE, "socketJob %i: UPLOAD", me);
    if (!uploadFinaleArchive(tree, connexion)) goto error2;
    break;

  case CGI:
    logMain(LOG_NOTICE, "socketJob %i: CGI", me);
    if (!cgiServer(tree, connexion)) goto error2;
    break;
    
  case HAVE:
    logMain(LOG_NOTICE, "socketJob %i: HAVE", me);
    if (!extractFinaleArchives(tree, connexion)) goto error2;
    break;
    
  case NOTIFY:
    logMain(LOG_NOTICE, "socketJob %i: NOTIFY (from %s)",
	    me, connexion->host);
    if (!acceptRemoteNotify(tree, connexion)) goto error2;
    break;
    
  default:
    logMain(LOG_INFO, "server do not accept %s messages",
	    strMessageType(tree->messageType));
    goto error2;
  }
  
  if (!cleanCacheTree(tree->collection)) goto error;
  rc = TRUE;
 error2:
  if (!releaseCollection(tree->collection, SERV)) rc = FALSE;
 error:
  if (rc) {
    logMain(LOG_NOTICE, "socketJob %i: success", me);
  } else {
    logMain(LOG_ERR, "socketJob %i: fails", me);
  }
  tree = destroyRecordTree(tree);
  
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
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
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
