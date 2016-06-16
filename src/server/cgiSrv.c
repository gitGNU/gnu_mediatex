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
 * Function   : excape_url
 * Description: do URL encode
 * Synopsis   : void escapeUrl(char* url, char* escapedUrl)
 * Input      : char* url: url to encode
 *              char* escapedUrl: allocated buffer for the resulting url
 * Output     : N/A
 * Note       : table built using:
 * int i, j, v, n = 0;
 *  for (i=0; i<16; ++i) {
 *    for (j=0; j<16; ++j) {
 *      v = (isalnum(n) || n=='*' || n=='-' || n=='.' || n=='_' )?
 *	  n:(n==' ')?'+':0;
 *      printf("%3i,", v);
 *      ++n;
 *    }
 *    printf("\n");
 * }
 * Thanks     : Alian 
 *            (http://stackoverflow.com/questions/5842471/c-url-encoding)
 =======================================================================*/
void escapeUrl(char* url, char* escapedUrl)
{
  static char escapeUrlTable[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    43,  0,  0,  0,  0,  0,  0,  0,  0,  0, 42,  0,  0, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,  0,  0,  0,  0,  0,  0,
    0, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,  0,  0,  0,  0, 95,
    0, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
    112,113,114,115,116,117,118,119,120,121,122,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  };
  unsigned char* ptr = (unsigned char*)url;

  for (; *ptr; ptr++){
    if (escapeUrlTable[*ptr]) {
      sprintf(escapedUrl, "%c", escapeUrlTable[*ptr]);
    }
    else {
      sprintf(escapedUrl, "%%%02X", *ptr);
    }
    while (*++escapedUrl);
  }
}

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
  ExtractData data;
  Record* record2 = 0;
  char* extra = 0;
 
  logMain(LOG_DEBUG, "extractCgiArchive");
  memset(&data, 0, sizeof(ExtractData));
  if (!(data.toKeeps = createRing())) goto error;
  data.coll = coll;
  data.scpContext = X_NO_REMOTE_COPY;
  data.cpContext =  X_NO_LOCAL_COPY;
  *found = FALSE;

  if (!loadCollection(coll, SERV | EXTR | CACH)) goto error;
  
  // add a temporary local demand (for deliver process)
  if (!(extra = createString("!wanted"))) goto error2;
  if (!(record2 = addRecord(coll, coll->localhost, archive, 
			      DEMAND, extra))) goto error2;
  extra = 0;
  if (!addCacheEntry(coll, record2)) goto error2;
  
  if (!extractArchive(&data, archive, FALSE)) goto error2;
  if (!extractDelToKeeps(coll, data.toKeeps)) goto error;

  // remove local demand (delRecord done by cleanCacheTree)
  if (!delCacheEntry(coll, record2)) goto error2;
  
  *found = data.found;
  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV | EXTR | CACH)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractCgiArchive fails");
  }
  extra = destroyString(extra);
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
    "220 ok %s/cache/", // ++"%s"
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
      sprintf(connexion->status, status[1], coll->localhost->url);

      // escape url into status[1] string
      extra = connexion->status;
      while (*++extra);
      escapeUrl(archive->localSupply->extra, extra);
      extra = 0;
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
      extra = 0;
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
  extra = destroyString(extra);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
