/*=======================================================================
 * Project: MediaTeX
 * Module : cgi-server
 *
 * Manage cgi queries

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

/*=======================================================================
 * Function   : extractLocalArchive
 * Description: Get or extract one archive into the local cache
 * Synopsis   : Archive* extractLocalArchive(Archive* archive)
 * Input      : Archive* archive = what we are looking for
 * Output     : int* found = TRUE if a local supply is available
 *              TRUE on success
 =======================================================================*/
int 
extractCgiArchive(Collection* coll, Archive* archive, int* found)
{
  int rc = FALSE;
  Configuration* conf = 0;
  ExtractData data;

  logMain(LOG_DEBUG, "extractCgiArchive");
  if (!(data.toKeeps = createRing())) goto error;
  data.coll = coll;
  data.context = X_NO_REMOTE_COPY;
  *found = FALSE;

  if (!(conf = getConfiguration())) goto error;
  if (!loadCollection(coll, SERV | EXTR | CACH)) goto error;
  data.target = archive;
  if (!extractArchive(&data, archive)) goto error2;
#warning How to deliver archive ? (set toKeep date)
  
  *found = data.found;
  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV | EXTR | CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractCgiArchive fails");
  }
  destroyOnlyRing(data.toKeeps);
  return rc;
} 

/*=======================================================================
 * Function   : cgiServer
 * Description: Deal with the CGI client
 * Synopsis   : int cgiServer(Connexion* connexion)
 * Input      : Connexion* connexion
 * Output     : TRUE on success
 =======================================================================*/
int 
cgiServer(Connexion* connexion)
{
  int rc = FALSE;
  Collection* coll = 0;
  Record* record = 0;
  Record* record2 = 0;
  Archive* archive = 0;
  AVLNode* node = 0;
  int found = FALSE;
  char* extra = 0;

  static char status[][32] = {
    "120 not found %s:%lli",
    "220 ok %s/cache/%s",
    "221 ok",
    "320 empty message",
  };

  logMain(LOG_DEBUG, "cgiServer");
  coll = connexion->message->collection;
  checkCollection(coll);

  if (!avl_count(connexion->message->records)) {
    sprintf(connexion->status, "%s", status[3]);
    goto error;
  }

  // only process the first archive
  if (!(record = connexion->message->records->head->item)) goto error;
  if (!(archive = record->archive)) goto error;
  if (!lockCacheRead(coll)) goto error;

  // Cgi-server handle 2 calls:
  if (isEmptyString(record->extra) || !strncmp(record->extra, "!w", 2)) {

    // case1: query without mail => try to extract archive
    if (!extractCgiArchive(coll, archive, &found)) goto error2;
    if (!found) {
      sprintf(connexion->status, status[0], archive->hash, archive->size);
    }
    else {
      sprintf(connexion->status, status[1],
	      coll->localhost->url, archive->localSupply->extra);
    }
  }
  else {
    // case2: query providing a mail => register the query
    //  we may handle several records here (at least for audit)
    for (node = connexion->message->records->head; node;
	 node = node->next) {
      record = node->item;
      if (!(archive = record->archive)) goto error2;
      if (!(extra = createString(record->extra))) goto error2;
      if (!(record2 = addRecord(coll, coll->localhost, archive, 
				DEMAND, extra))) goto error2;
      if (!addCacheEntry(coll, record2)) goto error2;
    }
    sprintf(connexion->status, "%s", status[2]);
  }
      
  rc = TRUE;
 error2:
  if (!unLockCache(coll)) rc = FALSE;
 error:
 if (!rc) {
    logMain(LOG_ERR, "cgiServer fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
