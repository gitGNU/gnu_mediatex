/*=======================================================================
 * Version: $Id: notify.c,v 1.1 2014/10/13 19:39:55 nroche Exp $
 * Project: MediaTeX
 * Module : server/notify

 * Send cache index to other servers 

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
#include "../misc/md5sum.h"
#include "../misc/address.h"
#include "../memory/cacheTree.h"
#include "../parser/serverFile.tab.h"
#include "../parser/confFile.tab.h"
#include "../common/connect.h"
#include "../common/extractScore.h"
#include "../common/openClose.h"
#include "cache.h"
#include "notify.h"

#ifndef utMAIN
#include <sys/socket.h> //
#include <netinet/in.h> // inet_ntoa
#include <arpa/inet.h>  //
#endif

int notifyArchive(NotifyData* data, Archive* archive);

/*=======================================================================
 * Function   : notifyContainer
 * Description: Single container notifyion
 * Synopsis   : int notifyContainer
 * Input      : Collection* coll = context
 *              Container* container = what to notify
 * Output     : int *found = TRUE if notifyed
 *              FALSE on error
 =======================================================================*/
int notifyContainer(NotifyData* data, Container* container)
{
  int rc = FALSE;
  Archive* archive = NULL;
  RGIT* curr = NULL;

  logEmit(LOG_DEBUG, "notify a container %s/%s:%lli", 
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size);

  data->found = FALSE;
  checkCollection(data->coll);

  if (isEmptyRing(container->parents)) goto end;
  data->found = TRUE;

  // we notify all parents (only one for TGZ, several for CAT...)
  while((archive = rgNext_r(container->parents, &curr))) {
    if (!notifyArchive(data, archive)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "notifyContainer fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : notifyArchive
 * Description: Single archive notifyion
 * Synopsis   : int notifyArchive
 * Input      : Collection* coll = context
 *              Archive* archive = what to notify
 * Output     : int *found = TRUE if notifyed
 *              FALSE on error
 =======================================================================*/
int notifyArchive(NotifyData* data, Archive* archive)
{
  int rc = FALSE;
  FromAsso* asso = NULL;
  RGIT* curr = NULL;

  logEmit(LOG_DEBUG, "notify an archive: %s:%lli", 
	  archive->hash, archive->size);

  data->found = FALSE;
  checkCollection(data->coll);

  // No good, as it may be a nat client...
  /* // stop if available from network */
  /* if (archive->remoteSupplies->nbItems > 0) { */
  /*   data->found = TRUE; */
  /*   goto end; */
  /* } */
  
  // look for a matching local supply
  if (archive->state >= AVAILABLE) {
    data->found = TRUE;
    if (!keepArchive(data->coll, archive, REMOTE_DEMAND)) goto error;
    if (!rgInsert(data->toNotify, archive->localSupply)) goto error;
    goto end;
  }

  // continue searching as we stop at the first container already there
  while(!data->found && (asso = rgNext_r(archive->fromContainers, &curr))) {
    if (!notifyContainer(data, asso->container)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "notifyArchive fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : getWantedArchives
 * Description: copy all wanted archives into a ring
 * Synopsis   : RG* getWantedArchives(Collection* coll)
 * Input      : Collection* coll
 * Output     : a ring of arhives, NULL o error
 =======================================================================*/
RG* 
getWantedRemoteArchives(Collection* coll)
{
  RG* rc = NULL;
  RG* ring = NULL;
  Archive* archive = NULL;
  Record* record = NULL;
  RGIT* curr = NULL;
  RGIT* curr2 = NULL;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "%s", "getWantedArchives");

  if (!(ring = createRing())) goto error;

  while((archive = rgNext_r(coll->cacheTree->archives, &curr))) {
    if (archive->state < WANTED) continue; // no remote demand

    // look for a remote demand
    while((record = rgNext_r(archive->demands, &curr2))) {
      if (getRecordType(record) != REMOTE_DEMAND) continue;
      if (!rgInsert(ring, archive)) goto error;
      break;
    }
  }

  rc = ring;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "getWantedRemoteArchives fails");
    destroyOnlyRing(ring);
  }
  return rc;
} 

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : TRUE on success
 =======================================================================*/
int addFinalDemands(NotifyData* data)
{
  int rc = FALSE;
  Collection* coll = NULL;
  Archive* archive = NULL;
  Record* record = NULL;
  Record* demand = NULL;
  char* extra = NULL;
  RGIT* curr = NULL;
  RGIT* curr2 = NULL;

  coll = data->coll;
  checkCollection(coll);
  logEmit(LOG_DEBUG, "%s", "addFinalDemands");

  while((archive = rgNext_r(coll->cacheTree->archives, &curr)) 
	!= NULL) {
    if (archive->state != WANTED) continue;
    while((record = rgNext_r(archive->demands, &curr2)) != NULL) {
      
      // final demand: add it
      if (getRecordType(record) == FINALE_DEMAND) {

	if (!(extra = createString("!wanted"))) goto error;
	if (!(demand = newRecord(coll->localhost, archive, 
				 DEMAND, extra))) goto error;
	extra = NULL;
	if (!rgInsert(data->toNotify, demand)) goto error;
	demand = NULL;
	break;
      }
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "addFinalDemands fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : TRUE on success
 =======================================================================*/
int addBadTopLocalSupplies(NotifyData* data)
{
  int rc = FALSE;
  Collection* coll = NULL;
  Archive* archive = NULL;
  RGIT* curr = NULL;

  coll = data->coll;
  checkCollection(coll);
  logEmit(LOG_DEBUG, "%s", "addBadTopLocalSupplies");

  // add local iso if do not own a good score
  while((archive = rgNext_r(coll->cacheTree->archives, &curr)) 
	!= NULL) {
    if (archive->state < AVAILABLE) continue;
    if (archive->fromContainers->nbItems != 0) continue;
    if (archive->extractScore > 10) 
      continue;
    /* TODO: check minGeoDup and nb REMOTE_SUPPLY
       if (archive->extractScore <
       coll->serverTree->scoreParam.badScore && ...) continue
    */

    if (!keepArchive(data->coll, archive, REMOTE_DEMAND)) goto error;
    if (!rgInsert(data->toNotify, archive->localSupply)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "addBadTopLocalSupplies fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : extractArchives
 * Description: Try to extract all the archives one by one
 * Synopsis   : int extractRemoteArchives(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
RG* buildNotifyRings(Collection* coll, RG* records)
{
  RG* rc = NULL;
  RG* archives = NULL;
  Archive* archive = NULL;
  RGIT* curr = NULL;
  NotifyData data;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "buildNotifyRings %s collection", coll->label);
  data.coll = coll;
  data.toNotify = records;
  if (!(archives = getWantedRemoteArchives(coll))) goto error;

  // for each cache entry, add local-supplies
  while((archive = rgNext_r(archives, &curr)) != NULL) {
    if (!notifyArchive(&data, archive)) goto error;
  }

  // add final demands
  if (!addFinalDemands(&data)) goto error;

  // add top archives with bad score
  if (!computeExtractScore(data.coll)) goto error;
  if (!addBadTopLocalSupplies(&data)) goto error;

  rc = data.toNotify;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "buildNotifyRings fails");
  }
  destroyOnlyRing(archives);
  return rc;
} 

/*=======================================================================
 * Function   : sendRemoteNotifyServer
 * Description: Notify a server about a collection
 * Synopsis   : int sendRemoteNotify(Server* server, 
 *                                   RecordTree* recordTree,
 *                                   Server* server
 * Input      : Collection* collection
 *              Server* server = server to call
 *              RecordTree* recordTree = content to send
 *              Server* origin = overriding caller (when Nat)
 * Output     : TRUE on success
 =======================================================================*/
int sendRemoteNotifyServer(Server* server, RecordTree* recordTree, 
			   Server* origin)
{
  int rc = FALSE;
  Collection* coll = NULL;
  int socket = -1;
  char* serverFP = NULL;

  checkServer(server);
  if (recordTree == NULL) goto error;
  coll = recordTree->collection;

  logEmit(LOG_DEBUG, "sendRemoteNotifyServer for %s/%s:%i",
 	  coll->label, server->host, server->mdtxPort);
  
  if (!env.dryRun) {
    // get a socket to the server
    if ((socket = connectServer(server)) == -1) {
      logEmit(LOG_NOTICE, "cannot connect %s", server->host);
      goto end;
    }
  }
    
  // send the archive tree
  if (origin != NULL) serverFP = origin->fingerPrint; // masquerade
  if (env.dryRun) {
    serializeRecordTree(recordTree, NULL, serverFP);
  }
  else {
    if (!upgradeServer(socket, recordTree, serverFP)) goto error;
  }

  logEmit(LOG_NOTICE, "%s:%i notified", server->host, server->mdtxPort);
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "sendRemoteNotifyServer fails");
  }
  if (socket != -1) close(socket);
  return rc;
}

/*=======================================================================
 * Function   : sendRemoteNotify
 * Description: Notify each server one by one
 * Synopsis   : int sendRemoteNotify(Collection* coll)
 * Input      : Collection* coll: collection to notify
 * Output     : TRUE on success
 =======================================================================*/
int sendRemoteNotify(Collection* coll)
{
  int rc = FALSE;
  Server* localhost = NULL;
  Server* target = NULL;
  RecordTree* recordTree = NULL;
  Record* record = NULL;
  RGIT* curr = NULL;

  logEmit(LOG_DEBUG, "sendRemoteNotify for %s collections", 
	  coll->label);

  if (!(recordTree = createRecordTree())) goto error;
  recordTree->collection = coll;
  recordTree->messageType = NOTIFY;
  //recordTree->doCypher = FALSE;

  if (!loadCollection(coll, SERV|EXTR|CACH)) goto error;
  if (!(localhost = getLocalHost(coll))) goto error2;
  if (!lockCacheRead(coll)) goto error2;

  // build records to send (local-supplies + local-demands)
  if (!buildNotifyRings(coll, recordTree->records)) goto error3;

  // note: we may send an empty ring in order to tell we no more look 
  // for any record

  // for all servers sharing the collection
  while((target=rgNext_r(coll->serverTree->servers, &curr))) {
    if (target->isLocalhost) continue;

    // skip server not directly connected to us
    if (!rgShareItems(localhost->networks, target->networks)) continue;

    if (!sendRemoteNotifyServer(target, recordTree, NULL)) goto error3;
  }

  rc = TRUE;
 error3:
  if (!unLockCache(coll)) rc = FALSE;

  // free local demands
  curr = NULL;
  while((record = rgNext_r(recordTree->records, &curr)) != NULL) {
    if (getRecordType(record) == LOCALE_DEMAND) {
      destroyRecord(record);
      curr->it = NULL;
    }
  }

 error2:
  if (!releaseCollection(coll, SERV|EXTR|CACH)) goto error;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "sendRemoteNotify %s collection fails", 
	    coll->label);
  }
  recordTree->records = destroyOnlyRing(recordTree->records);
  destroyRecordTree(recordTree);
  return rc;
}

/*=======================================================================
 * Function   : acceptRemoteNotify
 * Description: Receive a remote server notification
 * Synopsis   : int acceptRemoteNotify(RecordTree* tree,
 *                                                 Connexion* connexion)
 * Input      : RecordTree* tree = the remote cache index
 *              Connexion* connexion = gives the remote IP address
 * Output     : N/A
 * Note       : STILL A LOT TO DO HERE
 =======================================================================*/
int acceptRemoteNotify(RecordTree* tree, Connexion* connexion)
{
  int rc = FALSE;
  RG* records = NULL;
  Record* record = NULL;
  Collection* coll = NULL;
  Server* source = NULL;
  Server* target = NULL;
  Server* localhost = NULL;
  RGIT* curr = NULL;

  coll = tree->collection;
  source = connexion->server;
  checkCollection(coll);
  checkServer(source);
  logEmit(LOG_DEBUG, "%s", "acceptRemoteNotify");

  if (!loadCollection(tree->collection, CACH|SERV)) goto error;

  if (!(localhost = getLocalHost(coll))) goto error2; 
  
  // first: if localhost is a gateway...
  if (!isEmptyRing(coll->localhost->gateways)) {
    curr = NULL;

    // ...forward "same" message to our Nat clients
    while((target=rgNext_r(coll->serverTree->servers, &curr))) {

      // skip server already connected to the source
      if (rgShareItems(source->networks, target->networks)) continue;

      // skip server if not on our gateway networks
      if (!rgShareItems(localhost->gateways, target->networks)) continue;

      logEmit(LOG_NOTICE, "forward message to %s:%i nat client",
	      target->host, target->mdtxPort);
      if (!sendRemoteNotifyServer(target, tree, source)) goto error2;
    }
  }

  // secondly: update our cache
  if (!lockCacheRead(coll)) goto error2;

  // del all records we have from the calling server
  curr = NULL;
  records = coll->cacheTree->recordTree->records;
  while((record = rgNext_r(records, &curr)) != NULL) {  
    if (record->server != source) continue;
    if (!delCacheEntry(coll, record)) goto error3;
  }
		
  // add calling server's records
  rgRewind(tree->records);
  while ((record = rgNext(tree->records)) != NULL) {
    if (!addCacheEntry(coll, record)) goto error3;
    tree->records->curr->it = NULL; // consume the record from the tree
  }
  
  rc = TRUE;
 error3:
    if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(tree->collection, CACH|SERV)) rc = FALSE;
 error:
  if (!rc) {
    logEmit(LOG_DEBUG, "%s", "acceptRemoteNotify fails");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "../misc/address.h"
#include "../common/openClose.h"
#include "utFunc.h"
GLOBAL_STRUCT_DEF;
int running = TRUE;

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
 * modif      : 2012/05/01
 * Description: Unit test for cache module.
 * Synopsis   : ./utcache
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
  Collection* coll = NULL;
  Archive* archive = NULL;
  Server* server = NULL;
  Record* record = NULL;
  char* extra = NULL;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"input-rep", required_argument, NULL, 'd'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
    case 'd':
      if(optarg == NULL || *optarg == (char)0) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input repository\n",
		programName);
	rc = EINVAL;
	break;
      }
      strncpy(inputRep, optarg, strlen(optarg)+1);
      break; 

      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll = mdtxGetCollection("coll1"))) goto error;
  if (!getLocalHost(coll)) goto error;
  utLog("%s", "Clean the cache:", NULL);
  if (!utCleanCaches()) goto error;

  utLog("%s", "add a demands and local supplies:", NULL);
  
  // remote demand
  if (!(archive = 
	addArchive(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075))) 
    goto error;
  if (!(server = addServer(coll, "7af51aceb06864e690fa6a9e00000001")))
    goto error;
  if (!(extra = createString("!wanted"))) goto error;
  if (!(record = addRecord(coll, server, archive, DEMAND, extra)))
    goto error;
  if (!addCacheEntry(coll, record)) goto error;
  
  // local demand
  if (!(extra = createString("test@test"))) goto error;
  if (!(record = addRecord(coll, coll->localhost, archive, DEMAND, extra)))
    goto error;
  if (!addCacheEntry(coll, record)) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tgz")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "we have :", coll);
  
  utLog("%s", "here we send", NULL);
  if (!sendRemoteNotify(coll)) goto error;
  
  //utLog("%s", "here we receive", NULL);
  // todo: copy the ring and clean the cache
  //if (!acceptRemoteNotify(diff, LOCALHOST)) goto error;

  utLog("%s", "Clean the cache:", NULL);
  if (!utCleanCaches()) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  freeConfiguration();
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
