/*=======================================================================
 * Version: $Id: notify.c,v 1.12 2015/08/16 20:35:11 nroche Exp $
 * Project: MediaTeX
 * Module : notify

 * Send cache index to other servers 

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
int 
notifyContainer(NotifyData* data, Container* container)
{
  int rc = FALSE;
  Archive* archive = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "notify a container %s/%s:%lli", 
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
    logMain(LOG_ERR, "notifyContainer fails");
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
int 
notifyArchive(NotifyData* data, Archive* archive)
{
  int rc = FALSE;
  FromAsso* asso = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "notify an archive: %s:%lli", 
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
    logMain(LOG_ERR, "notifyArchive fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : getWantedArchives
 * Description: copy all wanted archives into a ring
 * Synopsis   : RG* getWantedArchives(Collection* coll)
 * Input      : Collection* coll
 * Output     : a ring of arhives, 0 o error
 =======================================================================*/
RG* 
getWantedRemoteArchives(Collection* coll)
{
  RG* rc = 0;
  RG* ring = 0;
  Archive* archive = 0;
  Record* record = 0;
  RGIT* curr = 0;
  RGIT* curr2 = 0;

  checkCollection(coll);
  logMain(LOG_DEBUG, "getWantedArchives");

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
    logMain(LOG_ERR, "getWantedRemoteArchives fails");
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
int 
addFinalDemands(NotifyData* data)
{
  int rc = FALSE;
  Collection* coll = 0;
  Archive* archive = 0;
  Record* record = 0;
  Record* demand = 0;
  char* extra = 0;
  RGIT* curr = 0;
  RGIT* curr2 = 0;

  coll = data->coll;
  checkCollection(coll);
  logMain(LOG_DEBUG, "addFinalDemands");

  while((archive = rgNext_r(coll->cacheTree->archives, &curr)) 
	!= 0) {
    if (archive->state != WANTED) continue;
    while((record = rgNext_r(archive->demands, &curr2))) {
      
      // final demand: add it
      if (getRecordType(record) == FINAL_DEMAND) {

	if (!(extra = createString("!wanted"))) goto error;
	if (!(demand = newRecord(coll->localhost, archive, 
				 DEMAND, extra))) goto error;
	extra = 0;
	if (!rgInsert(data->toNotify, demand)) goto error;
	demand = 0;
	break;
      }
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "addFinalDemands fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : addBadTopLocalSupplies
 * Description: Check for top containers having bad score (as iso)
 * Synopsis   : int addBadTopLocalSupplies(NotifyData* data)
 * Input      : NotifyData* data
 * Output     : TRUE on success

 * TODO       : check minGeoDup and nb REMOTE_SUPPLY
 =======================================================================*/
int 
addBadTopLocalSupplies(NotifyData* data)
{
  int rc = FALSE;
  Collection* coll = 0;
  Archive* archive = 0;
  RGIT* curr = 0;

  coll = data->coll;
  checkCollection(coll);
  logMain(LOG_DEBUG, "addBadTopLocalSupplies");

  // add local top containers having a bad score
  while((archive = rgNext_r(coll->cacheTree->archives, &curr))) {
    if (archive->state < AVAILABLE) continue;
    if (archive->extractScore > coll->serverTree->scoreParam.maxScore /2) 
      continue;
    if (archive->fromContainers->nbItems > 0) continue;

    /* check minGeoDup and nb REMOTE_SUPPLY
       if (archive->extractScore <
       coll->serverTree->scoreParam.badScore && ...) continue
    */

    if (!rgInsert(data->toNotify, archive->localSupply)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "addBadTopLocalSupplies fails");
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
RG* 
buildNotifyRings(Collection* coll, RG* records)
{
  RG* rc = 0;
  RG* archives = 0;
  Archive* archive = 0;
  RGIT* curr = 0;
  NotifyData data;

  checkCollection(coll);
  logMain(LOG_DEBUG, "buildNotifyRings %s collection", coll->label);
  data.coll = coll;
  data.toNotify = records;
  if (!(archives = getWantedRemoteArchives(coll))) goto error;

  // for each cache entry, add local-supplies
  while((archive = rgNext_r(archives, &curr))) {
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
    logMain(LOG_ERR, "buildNotifyRings fails");
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
int 
sendRemoteNotifyServer(Server* server, RecordTree* recordTree, 
		       Server* origin)
{
  int rc = FALSE;
  Collection* coll = 0;
  int socket = -1;
  char* serverFP = 0;

  checkServer(server);
  if (recordTree == 0) goto error;
  coll = recordTree->collection;

  logMain(LOG_DEBUG, "sendRemoteNotifyServer for %s/%s:%i",
 	  coll->label, server->host, server->mdtxPort);
  
  if (!env.dryRun) {
    // get a socket to the server
    if ((socket = connectServer(server)) == -1) {
      logMain(LOG_NOTICE, "cannot connect %s", server->host);
      goto end;
    }
  }
    
  // send the archive tree
  if (origin) serverFP = origin->fingerPrint; // masquerade
  if (env.dryRun) {
    serializeRecordTree(recordTree, 0, serverFP);
  }
  else {
    if (!upgradeServer(socket, recordTree, serverFP)) goto error;
  }

  logMain(LOG_NOTICE, "%s:%i notified", server->host, server->mdtxPort);
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "sendRemoteNotifyServer fails");
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
int 
sendRemoteNotify(Collection* coll)
{
  int rc = FALSE;
  Server* localhost = 0;
  Server* target = 0;
  RecordTree* recordTree = 0;
  Record* record = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "sendRemoteNotify for %s collections", 
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

    if (!sendRemoteNotifyServer(target, recordTree, 0)) goto error3;
  }

  rc = TRUE;
 error3:
  if (!unLockCache(coll)) rc = FALSE;

  // free local demands
  curr = 0;
  while((record = rgNext_r(recordTree->records, &curr))) {
    if (getRecordType(record) == LOCAL_DEMAND) {
      destroyRecord(record);
      curr->it = 0;
    }
  }

 error2:
  if (!releaseCollection(coll, SERV|EXTR|CACH)) goto error;
 error:
  if (!rc) {
    logMain(LOG_ERR, "sendRemoteNotify %s collection fails", 
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
  RG* records = 0;
  Record* record = 0;
  Collection* coll = 0;
  Server* source = 0;
  Server* target = 0;
  Server* localhost = 0;
  RGIT* curr = 0;

  coll = tree->collection;
  source = connexion->server;
  checkCollection(coll);
  checkServer(source);
  logMain(LOG_DEBUG, "acceptRemoteNotify");

  if (!loadCollection(tree->collection, CACH)) goto error;

  if (!(localhost = getLocalHost(coll))) goto error2; 
  
  // first: if localhost is a gateway...
  if (!isEmptyRing(coll->localhost->gateways)) {
    curr = 0;

    // ...forward "same" message to our Nat clients
    while((target=rgNext_r(coll->serverTree->servers, &curr))) {

      // skip server already connected to the source
      if (rgShareItems(source->networks, target->networks)) continue;

      // skip server if not on our gateway networks
      if (!rgShareItems(localhost->gateways, target->networks)) continue;

      logMain(LOG_NOTICE, "forward message to %s:%i nat client",
	      target->host, target->mdtxPort);
      if (!sendRemoteNotifyServer(target, tree, source)) goto error2;
    }
  }

  // secondly: update our cache
  if (!lockCacheRead(coll)) goto error2;

  // del all records we have from the calling server
  curr = 0;
  records = coll->cacheTree->recordTree->records;
  while((record = rgNext_r(records, &curr))) {  
    if (record->server != source) continue;
    if (!delCacheEntry(coll, record)) goto error3;
  }
		
  // add calling server's records
  rgRewind(tree->records);
  while ((record = rgNext(tree->records))) {
    if (!addCacheEntry(coll, record)) goto error3;
    tree->records->curr->it = 0; // consume the record from the tree
  }
  
  rc = TRUE;
 error3:
    if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(tree->collection, CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_DEBUG, "acceptRemoteNotify fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
