/*=======================================================================
 * Project: MediaTeX
 * Module : notify

 * Send cache index to other servers 

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
  while ((archive = rgNext_r(container->parents, &curr))) {
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
  
  // look for a matching local supply
  if (archive->state >= AVAILABLE) {
    data->found = TRUE;
    
    logMain(LOG_INFO, "found a local supply to notify:");
    logRecord(LOG_MAIN, LOG_INFO, archive->localSupply);
    if (!avl_insert(data->toNotify, archive->localSupply)) {
      if (errno != EEXIST) {
	logMain(LOG_ERR, "fails to add record");
	goto error;
      }
    }
    goto end;
  }

  // continue searching deeper if needed
  while (!data->found && 
	(asso = rgNext_r(archive->fromContainers, &curr))) {
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
 * Function   : getWantedRemoteArchives
 * Description: copy all remotely wanted archives into a ring
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
  AVLNode* node = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  logMain(LOG_DEBUG, "getWantedRemoteArchives");

  if (!(ring = createRing())) goto error;

  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    if (archive->state < WANTED) continue; // no remote demand

    // look for a remote-demands
    curr = 0;
    while ((record = rgNext_r(archive->demands, &curr))) {
      if (getRecordType(record) != REMOTE_DEMAND) continue;
      
      logMain(LOG_INFO, "working on remote demand:");
      logRecord(LOG_MAIN, LOG_INFO, record);
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
 * Function   : addFinalDemands
 * Description: Add final demands to the message to send
 * Synopsis   : int addFinalDemands(NotifyData* data)
 * Input      : NotifyData* data
 * Output     : TRUE on success
 =======================================================================*/
int 
addFinalDemands(NotifyData* data)
{
  int rc = FALSE;
  Collection* coll = 0;
  Server* localhost = 0;
  Archive* archive = 0;
  Record* record = 0;
  Record* demand = 0;
  char* extra = 0;
  AVLNode* node = 0;
  RGIT* curr = 0;
 
  coll = data->coll;
  checkCollection(coll);
  logMain(LOG_DEBUG, "addFinalDemands");
  if (!(localhost = getLocalHost(coll))) goto error;

  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    if (archive->state != WANTED) continue;
    
    // add (a uniq) final demand
    curr = 0;
    while ((record = rgNext_r(archive->demands, &curr))) {
      if (getRecordType(record) == FINAL_DEMAND) {

	logMain(LOG_INFO, "found a final demand to notify:");
	logRecord(LOG_MAIN, LOG_INFO, record);
	if (!(extra = createString("!wanted"))) goto error;
	if (!(demand = newRecord(coll->localhost, archive, DEMAND, extra))) 
	  goto error;
	extra = 0;
	if (!avl_insert(data->toNotify, demand)) {
	  if (errno != EEXIST) {
	    logMain(LOG_ERR, "fails to add record");
	    goto error;
	  }
	  destroyRecord(demand);
	}
	demand = 0;
	break;
      }
    }

    // if localhost is a gateway...
    if (isEmptyRing(coll->localhost->gateways)) continue;

    // ...relay NAT client's final demand too (as our)
    curr = 0;
    while ((record = rgNext_r(archive->demands, &curr))) {
      if (getRecordType(record) == REMOTE_DEMAND &&
	rgShareItems(localhost->gateways, record->server->networks)) {
	
	logMain(LOG_INFO, "found a remote demand to manage:");
	logRecord(LOG_MAIN, LOG_INFO, record);
	if (!(extra = createString("!wanted"))) goto error;
	if (!(demand = newRecord(coll->localhost, record->archive,
				 DEMAND, extra))) 
	  goto error;
	extra = 0;
	if (!avl_insert(data->toNotify, demand)) {
	   if (errno != EEXIST) {
	    logMain(LOG_ERR, "fails to add record");
	    goto error;
	   }
	   destroyRecord(demand);
	}
	demand = 0;
      }
    }
  }
 
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "addFinalDemands fails");
  }
  destroyRecord(demand);
  return rc;
} 

/*=======================================================================
 * Function   : addBadTopLocalSupplies
 * Description: Check for top containers having bad score (as iso)
 * Synopsis   : int addBadTopLocalSupplies(NotifyData* data)
 * Input      : NotifyData* data
 * Output     : TRUE on success
 =======================================================================*/
int 
addBadTopLocalSupplies(NotifyData* data)
{
  int rc = FALSE;
  Collection* coll = 0;
  Archive* archive = 0;
  AVLNode* node = 0;

  coll = data->coll;
  checkCollection(coll);
  logMain(LOG_DEBUG, "addBadTopLocalSupplies");

  // add local top containers having a bad score
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    if (archive->state < AVAILABLE) continue;
    if (!isBadTopContainer(coll, archive)) continue;
    
    logMain(LOG_INFO, "found a bad score's top container to notify:");
    logRecord(LOG_MAIN, LOG_INFO, archive->localSupply);
    if (!avl_insert(data->toNotify, archive->localSupply)) {
      if (errno != EEXIST) {
	logMain(LOG_ERR, "fails to add record");
	goto error;
      }
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "addBadTopLocalSupplies fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : buildNotifyRings
 * Description: build records to send (local supplies + local demands)
 * Synopsis   : RG* buildNotifyRings(Collection* coll, RG* records)
 * Input      : Collection* coll
 *              RG* records: the record's ring to push into
 * Output     : TRUE on success
 =======================================================================*/
int
buildNotifyRings(Collection* coll, AVLTree* records)
{
  int rc = FALSE;
  RG* archives = 0;
  Archive* archive = 0;
  RGIT* curr = 0;
  NotifyData data;

  checkCollection(coll);
  logMain(LOG_DEBUG, "buildNotifyRings %s collection", coll->label);
  data.coll = coll;
  data.toNotify = records;

  // work on archive wanted by remote servers
  if (!(archives = getWantedRemoteArchives(coll))) goto error;

  // for each of them, try to add local-supplies
  while ((archive = rgNext_r(archives, &curr))) {
    if (!notifyArchive(&data, archive)) goto error;
  }

  // next, add final demands (on localserver)
  if (!addFinalDemands(&data)) goto error;

  // and, add top archives with bad score
  if (!computeExtractScore(data.coll)) goto error;
  if (!addBadTopLocalSupplies(&data)) goto error;

  rc = TRUE;
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
  
  // get a socket to the server
  if ((socket = connectServer(server)) == -1) {
    logMain(LOG_NOTICE, "cannot connect %s", server->host);
    goto end;
  }
    
  // send the archive tree
  if (origin) serverFP = origin->fingerPrint; // masquerade
  if (!upgradeServer(socket, recordTree, serverFP)) goto error;
  
  logMain(LOG_NOTICE, "%s:%i notified", server->host, server->mdtxPort);
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "sendRemoteNotifyServer fails");
  }
  if (!env.dryRun && socket != -1) close(socket);
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
  AVLNode* node = 0;

  logMain(LOG_DEBUG, "sendRemoteNotify for %s collections", 
	  coll->label);

  if (!(recordTree = createRecordTree())) goto error;
  recordTree->collection = coll;
  recordTree->messageType = NOTIFY;
  //recordTree->doCypher = FALSE;

  if (!loadCollection(coll, SERV|EXTR|CACH)) goto error;
  if (!(localhost = getLocalHost(coll))) goto error2;
  if (!lockCacheRead(coll)) goto error2;

  // build records to send (local supplies + local demands)
  if (!buildNotifyRings(coll, recordTree->records)) goto error3;

  // note: we may send an empty ring in order to tell we no more look 
  // for any record

  // for all servers sharing the collection
  while ((target=rgNext_r(coll->serverTree->servers, &curr))) {
    if (target->isLocalhost) continue;

    // skip server not directly connected to us
    if (!rgShareItems(localhost->networks, target->networks)) continue;
    if (!sendRemoteNotifyServer(target, recordTree, 0)) goto error3;
  }

  rc = TRUE;
 error3:
  if (!unLockCache(coll)) rc = FALSE;

  // free local demands
   for (node = recordTree->records->head; node; node = node->next) {
    record = node->item;
    if (getRecordType(record) == LOCAL_DEMAND) {
      destroyRecord(record);
      node->item = 0;
    }
  }

 error2:
  if (!releaseCollection(coll, SERV|EXTR|CACH)) goto error;
 error:
  if (!rc) {
    logMain(LOG_ERR, "sendRemoteNotify %s collection fails", 
	    coll->label);
  }
  recordTree->records->freeitem = 0;
  avl_free_nodes(recordTree->records);
  recordTree->records->freeitem = (void(*)(void*)) destroyRecord;
  destroyRecordTree(recordTree);
  return rc;
}

/*=======================================================================
 * Function   : acceptRemoteNotify
 * Description: Receive a remote server notification
 * Synopsis   : int acceptRemoteNotify(RecordTree* tree,
 *                                                 Connexion* connexion)
 * Input      : Connexion* connexion = gives the remote IP address
 * Output     : TRUE on success
 =======================================================================*/
int acceptRemoteNotify(Connexion* connexion)
{
  int rc = FALSE;
  AVLTree* records = 0;
  Record* record = 0;
  Collection* coll = 0;
  Server* source = 0;
  Server* target = 0;
  Server* localhost = 0;
  RGIT* curr = 0;
  AVLNode* node = 0;

  static char status[][8] = {
    "240 ok"
  };

  logMain(LOG_DEBUG, "acceptRemoteNotify");
  coll = connexion->message->collection;
  source = connexion->server;
  checkCollection(coll);
  checkServer(source);

  if (!loadCollection(coll, CACH)) goto error;
  if (!(localhost = getLocalHost(coll))) goto error2;
  
  // first: if localhost is a gateway...
  if (!isEmptyRing(coll->localhost->gateways)) {
    curr = 0;

    // ...forward the message to the Nat clients
    while ((target=rgNext_r(coll->serverTree->servers, &curr))) {

      // skip server already connected to the source
      if (rgShareItems(source->networks, target->networks)) continue;

      // skip server if not on our gateway networks
      if (!rgShareItems(localhost->gateways, target->networks)) continue;

      logMain(LOG_NOTICE, "forward message to %s:%i nat client",
	      target->host, target->mdtxPort);
      if (!sendRemoteNotifyServer(target, connexion->message, source)) 
	goto error2;
    }
  }

  // secondly: update our cache
  if (!lockCacheRead(coll)) goto error2;

  // del all records we previously get from the calling server
  records = coll->cacheTree->recordTree->records;
  for (node = records->head; node; node = node->next) {
    record = node->item;
    if (record->server != source) continue;
    if (!delCacheEntry(coll, record)) goto error3;
  }
		
  // add new fresh records provided by the calling server
  for (node = connexion->message->records->head; node; node = node->next) {
    record = node->item;
    if (!addCacheEntry(coll, record)) goto error3;
    node->item = 0;
  }
  // record remains into the cache ; tree free by caller
  connexion->message->records->freeitem = 0;
  avl_free_nodes(connexion->message->records);
  connexion->message->records->freeitem = (void(*)(void*)) destroyRecord;
    
  sprintf(connexion->status, "%s", status[0]);
  rc = TRUE;
 error3:
    if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, CACH)) rc = FALSE;
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
