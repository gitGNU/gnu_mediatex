/*=======================================================================
 * Version: $Id: cgiSrv.c,v 1.8 2015/08/17 01:31:53 nroche Exp $
 * Project: MediaTeX
 * Module : cgi-server
 *
 * Manage cgi queries

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
  *found = FALSE;
  data.coll = coll;
  data.context = X_CGI;
  if (!(data.toKeeps = createRing())) goto error;

  if (!(conf = getConfiguration())) goto error;
  if (!loadCollection(coll, SERV | EXTR | CACH)) goto error;
  data.target = archive;
  if (!extractArchive(&data, archive)) goto error2;

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
 * Synopsis   : int cgiServer(ArchiveTree* demands, int fd)
 * Input      : ArchiveTree* demands = what we have reveive from socket
 *              int fd = the socket for replying
 * Output     : TRUE on success

 * Note: Only the first archive is processed
 =======================================================================*/
int cgiServer(RecordTree* recordTree, Connexion* connexion)
{
  int rc = FALSE;
  int cgiErrno = 4;
  Collection* coll = 0;
  Record* record = 0;
  Record* record2 = 0;
  Archive* archive = 0;
  int found = FALSE;
  char buffer[576] = ""; // text + cache url + MAX_SIZE_STRING
  char* extra = 0;

  // text + cache url + MAX_SIZE_STRING
  static char message[][576] = {
    "200 ok %s/%s",
    "301 no collection",
    "302 empty message",
    "303 not found %s:%lli",
    "400 internal error",   
  };

  logMain(LOG_DEBUG, "cgiServer");
  sprintf(buffer, "%s", message[cgiErrno=4]);

  // get the archiveTree's collection
  if ((coll = recordTree->collection) == 0) {
    sprintf(buffer, "%s", message[cgiErrno=1]);
    goto error;
  }

  if (isEmptyRing(recordTree->records)) {
    sprintf(buffer, "%s", message[cgiErrno=2]);
    goto error;
  }

  // only process the first archive
  if (!(record = (Record*)recordTree->records->head->it)) goto error;
  if (!(archive = record->archive)) goto error;

  if (!lockCacheRead(coll)) goto error;

  // first query without mail
  if (isEmptyString(record->extra) || !strncmp(record->extra, "!w", 2)) {

    // synchronous query: search/extraction
    if (!extractCgiArchive(coll, archive, &found)) goto error2;
    if (!found) {
      sprintf(buffer, message[cgiErrno=3], archive->hash, archive->size);
    }
    else {
      sprintf(buffer, message[cgiErrno=0],
	      coll->cacheUrl, archive->localSupply->extra);
    }
  }
  else {
    // second query providing a mail
    if (!(extra = createString(record->extra))) goto error2;
    if (!(record2 = addRecord(coll, coll->localhost, archive, 
			      DEMAND, extra))) goto error2;
    if (!addCacheEntry(coll, record2)) goto error2;
    sprintf(buffer, message[cgiErrno=0], "", 0);
    buffer[6] = 0; // no archive need to be specified
  }
      
  logMain(LOG_NOTICE, "%s", buffer);
  rc = TRUE;
 error2:
  if (!unLockCache(coll)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", buffer);
  }
  if (!env.noRegression) {
    strcpy(buffer + strlen(buffer), "\n");
    tcpWrite(connexion->sock, buffer, strlen(buffer));
  }
#warning needed? (not in cache.c)
  if (connexion->sock != STDOUT_FILENO && 
      shutdown(connexion->sock, SHUT_WR) == -1) {
    logMain(LOG_ERR, "shutdown fails: %s", strerror(errno));
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
