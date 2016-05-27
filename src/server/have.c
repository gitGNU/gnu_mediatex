/*=======================================================================
 * Project: MediaTeX
 * Module : have
 *
 * Manage extraction from removable device

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

int haveArchive(ExtractData* data, Archive* archive);

/*=======================================================================
 * Function   : haveContainer
 * Description: up to down recursive match for wanted records
 * Synopsis   : int haveContainer
 * Input      : Collection* coll = context
 *              Container* container = what to have
 * Output     : int *found = TRUE if we ewtract something
 *              FALSE on error
 =======================================================================*/
int haveContainer(ExtractData* data, Container* container)
{
  int rc = FALSE;
  FromAsso* asso = 0;
  AVLNode *node = 0;
  int found = FALSE;

  checkCollection(data->coll);
  logMain(LOG_DEBUG, "have a container %s/%s:%lli", 
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size);
 
  // look for wanted record depp into all childs
  for(node = container->childs->head; node; node = node->next) {
    asso = node->item;

    if (!haveArchive(data, asso->archive)) goto error;
    if (data->found) found = TRUE;
  }  

  data->found = found;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "haveContainer fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : haveArchive
 * Description: up to down recursive match for wanted records
 * Synopsis   : int haveArchive
 * Input      : Collection* coll = context
 *              Archive* archive = what to have
 * Output     : int *found = TRUE if haveed
 *              FALSE on error
 =======================================================================*/
int haveArchive(ExtractData* data, Archive* archive)
{
  int rc = FALSE;
  int doExtract = FALSE;

  logMain(LOG_DEBUG, "have an archive: %s:%lli", 
	  archive->hash, archive->size);

  data->found = FALSE;
  checkCollection(data->coll);

  // exit condition
  if (archive->state == AVAILABLE) {
    data->found = TRUE;
    goto end;
  }

  doExtract = (archive->state == WANTED ||
	       // or top container having bad score
	       (archive->state < WANTED &&
		archive->fromContainers->nbItems == 0 &&
		archive->extractScore <= 
		data->coll->serverTree->scoreParam.maxScore /2));
  
  // perform deepest extraction first 
  if (archive->toContainer) {
    if (!haveContainer(data, archive->toContainer)) goto error;
  }

  // wanted archive may become available but we may want
  // to copy it into the cache and/or to deliver it
  if (doExtract) {
    logMain(LOG_NOTICE, "have content to extract: %s:%lli", 
	    archive->hash, archive->size);
    data->target = archive;
    if (!extractArchive(data, archive)) goto error;
    if (data->found) {
      if (!deliverArchive(data->coll, archive)) goto error;
    }
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "haveArchive fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : extractFinaleArchives
 * Description: Try to extract archives using the image provided
 * Synopsis   : int extractFinaleArchives(Connexion* connexion)
 * Input      : Connexion* connexion
 * Output     : TRUE on success
 =======================================================================*/
int 
extractFinaleArchives(Connexion* connexion)
{
  int rc = FALSE;
  ExtractData data;
  Record* record = 0;

  static char status[][64] = {
    "230 ok",
    "330 empty message",
    "331 message do not provide a final supply %s"
  };

  logMain(LOG_DEBUG, "remote extraction");
  checkCollection(connexion->message->collection);
  memset(&data, 0, sizeof(ExtractData));
  if (!(data.toKeeps = createRing())) goto error;
  data.coll = connexion->message->collection;
  data.context = X_NO_REMOTE_COPY;
  
  // check we get a final supplies
  if (isEmptyRing(connexion->message->records)) {
    sprintf(connexion->status, "%s", status[1]);
    goto error;
  }
  if (!(record = rgHead(connexion->message->records))) goto error;
  if (getRecordType(record) != FINAL_SUPPLY) {
    sprintf(connexion->status, status[2], record->extra);
    goto error;
  }

  if (!loadCollection(data.coll, SERV | EXTR | CACH)) goto error;
  if (!lockCacheRead(data.coll)) goto error2;

  // push provided final supplies into the cache
  if (!addCacheEntry(data.coll, record)) goto error3;
  rgRemove(connexion->message->records);

  // up-down recursive match for wanted content from the final supply
  if (!haveArchive(&data, record->archive)) goto error4;

  sprintf(connexion->status, "%s", status[0]);
  rc = TRUE;
 error4:
  if (!delCacheEntry(data.coll, record)) rc = FALSE;
 error3:
  if (!unLockCache(data.coll)) rc = FALSE;
 error2:
  if (!releaseCollection(data.coll, SERV | EXTR | CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractFinaleArchives fails");
  }
  destroyOnlyRing(data.toKeeps);
  return rc;
} 

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

