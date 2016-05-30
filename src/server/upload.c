/*=======================================================================
 * Project: MediaTeX
 * Module : cache
 *
 * Manage local cache directory and DB

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
 * Function   : cacheUpload
 * Description: call extract to upload a file on cache
 * Synopsis   : static int 
 *              cacheUpload(Collection* coll, off_t need, int* rcode)
 * Input      : Collection* collection: collections to use
 *              Record*: file to upload
 * Output     : TRUE on success
 =======================================================================*/
static int
cacheUpload(Collection* coll, Record* record)
{
  int rc = FALSE;
  ExtractData data;
  char* ptr = 0;
  char buf[MAX_SIZE_STRING];
  struct tm date;

  if (isEmptyString(record->extra)) {
    logMain(LOG_ERR, "please provide a file to upload");
    goto error;
  }

  logMain(LOG_DEBUG, "upload file: %s", record->extra);
  checkCollection(coll);
  memset(&data, 0, sizeof(ExtractData));
  if (!(data.toKeeps = createRing())) goto error;
  data.context = X_NO_REMOTE_COPY;
  data.coll = coll;

  // (double) check we get a final supplies
  if (record->extra[0] != '/' || 
      // and not a directory name (ending by '/')
      record->extra[strlen(record->extra)] == '/') {
    logMain(LOG_ERR, "not a final supply: %s", record->extra);
    goto error;
  }

  // find an eventual target path
  for (ptr = record->extra; *ptr && *ptr != ':'; ++ptr);
  if (!*ptr) {
    // no target provided: use AAAAMM/basename
    if (localtime_r(&record->date, &date) == (struct tm*)0) {
      logMain(LOG_ERR, "localtime_r returns on error");
      goto error;
    }
    while (*--ptr != '/');

    if (snprintf(buf, MAX_SIZE_STRING, ":%04i-%02i%s",
		 date.tm_year + 1900, date.tm_mon+1, ptr)
	>= MAX_SIZE_STRING - strlen(record->extra)) {
      logMain(LOG_ERR, "path into cache too long");
      goto error;
    }

    if (!(record->extra = catString(record->extra, buf))) goto error;
  }

  // do the upload
  if (!extractArchive(&data, record->archive, TRUE)) goto error;
  if (!extractDelToKeeps(coll, data.toKeeps)) goto error;
  if (!data.found) goto error;
  

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "cacheUpload fails");
  } 
  destroyOnlyRing(data.toKeeps);
  return rc;
}

/*=======================================================================
 * Function   : uploadFinaleArchive
 * Description: Upload a new final supply into the cache
 * Synopsis   : int uploadFinaleArchive(ArchiveTree* finalSupplies)
 * Input      : Connexion* connexion
 * Output     : TRUE on success
 =======================================================================*/
int 
uploadFinaleArchive(Connexion* connexion)
{
  int rc = FALSE;
  Collection* coll = 0;
  Record* record = 0;
  AVLNode* node = 0;
  
  static char status[][64] = {
    "210 ok",
    "310 empty message",
    "332 message do not provide a final supply %s",
    "313 already exists %s:%lli"
  };

  logMain(LOG_DEBUG, "uploadFinaleArchive");
  coll = connexion->message->collection;
  checkCollection(coll);
  
  // check we get a final supplies
  if (!connexion->message->records ||
      !avl_count(connexion->message->records)) {
    sprintf(connexion->status, "%s", status[1]);
    goto error;
  }

  if (!(node = connexion->message->records->head)) goto error;
  record = node->item;

  if (getRecordType(record) != FINAL_SUPPLY) {
    sprintf(connexion->status, status[2], record->extra);
    goto error;
  }

  if (!loadCollection(coll, EXTR | CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;
 
  // check archive is not already there
  if ((record->archive->localSupply)) {
    sprintf(connexion->status, status[3],
	    record->archive->hash, record->archive->size);
    goto error3;
  }

  // push provided final supplies into the cache
  if (!addCacheEntry(coll, record)) goto error3;
  avl_unlink_node(connexion->message->records, node); // consume record
  remind(node);
  free(node);
  
  // extract the final supply into the cache
  if (!(cacheUpload(coll, record))) goto error4;
  
  sprintf(connexion->status, "%s", status[0]);
  rc = TRUE;
 error4:
  if (!delCacheEntry(coll, record)) rc = FALSE;
 error3:
  if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, EXTR | CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "uploadFinaleArchive fails");
  }
  return rc;
} 

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
