/*=======================================================================
 * Version: $Id: cgiSrv.c,v 1.4 2015/06/30 17:37:37 nroche Exp $
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

//#include <sys/socket.h> // shutdown

static char message[][64] = {
  "000 no error\n",               // not used
  "100 nobody\n",                 // not used (non sens)
  "200 ok\n",                     // not used (need to add the path)
  "300 not here\n",
  "400 internal error\n",

  "301 collection not managed\n" // cgiErrno = 5
  "302 no extraction plan\n"     // cgiErrno = 6 (not use in fact)
};

/*=======================================================================
 * Function   : extractLocalArchive
 * Description: Get or extract one archive into the local cache
 * Synopsis   : Archive* extractLocalArchive(Archive* archive)
 * Input      : Archive* archive = what we are looking for
 * Output     : Archive* = copy of the archive researched
 =======================================================================*/
int 
extractCgiArchive(Collection* coll, Archive* archive, int* found)
{
  int rc = FALSE;
  Configuration* conf = 0;
  ExtractData data;

  logEmit(LOG_DEBUG, "%s", "local extraction");
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
    logEmit(LOG_ERR, "%s", "local extraction fails");
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
  Collection* coll = 0;
  Record* record = 0;
  Record* record2 = 0;
  Archive* archive = 0;
  int found = FALSE;
  char buffer[256];
  char* extra = 0;
  int cgiErrno = 0;

  logEmit(LOG_DEBUG, "%s", "cgiServer: serving cgi query");

  /* logEmit(LOG_INFO, "%s", "input archive tree:"); */
  /* serializeRecordTree(recordTree, 0); */

  // get the archiveTree's collection
  if ((coll = recordTree->collection) == 0) {
    logEmit(LOG_ERR, "%s", "unknown collection for archiveTree");
    cgiErrno = 5;
    goto error;
  }

  if (isEmptyRing(recordTree->records)) {
    logEmit(LOG_ERR, "%s", "please provide a Record tree for cgiServer");
    cgiErrno = 4;
    goto error;
  }

  // only process the first archive
  if (!(record = (Record*)recordTree->records->head->it)) goto error;
  if (!(archive = record->archive)) goto error;

  /* logEmit(LOG_INFO, "%s", "first archive:"); */
  /* serializeRecord(recordTree, record); */

  if (!lockCacheRead(coll)) goto error;

  if (isEmptyString(record->extra) || !strncmp(record->extra, "!w", 2)) {

    // synchronous query: search/extraction
    cgiErrno = 3;
    if (!extractCgiArchive(coll, archive, &found)) goto error2;
    if (!found) {
      logEmit(LOG_NOTICE, "not found %s:%i", archive->hash, archive->size);
    }
    else {
      logEmit(LOG_NOTICE, "query for %s", archive->localSupply->extra);
      sprintf(buffer, "200 %s/%s\n", 
	      coll->cacheUrl, archive->localSupply->extra);

      // tcpWrite only deal with sockets (on unit test we use stdout)
      logEmit(LOG_INFO, "write to socket: %s", buffer);
      if (!env.noRegression) {
	tcpWrite(connexion->sock, buffer, strlen(buffer));
      }
      cgiErrno = 2;
    }
  }
  else {
    // register query: merge archive trees
    logEmit(LOG_NOTICE, "final demand for %s:%lli %s", 
	    archive->hash, archive->size, record->extra);
    cgiErrno = 4;
    if (!(extra = createString(record->extra))) goto error2;
    if (!(record2 = addRecord(coll, coll->localhost, archive, 
			      DEMAND, extra))) goto error2;
    if (!addCacheEntry(coll, record2)) goto error2;

    logEmit(LOG_INFO, "write to socket: 200 \n");
    if (!env.noRegression) {
      tcpWrite(connexion->sock, "200 \n", 5);
    }

    cgiErrno = 2;
  }
      
  rc = TRUE;
 error2:
  if (!unLockCache(coll)) rc = FALSE;
 error:
  if (cgiErrno != 2) {
    sprintf(buffer, "%s", message[cgiErrno]);

    logEmit(LOG_INFO, "write to socket: %s", buffer);
    if (!env.noRegression) {
      tcpWrite(connexion->sock, buffer, strlen(buffer));
    }
  }

  if (connexion->sock != STDOUT_FILENO && 
      shutdown(connexion->sock, SHUT_WR) == -1) {
    logEmit(LOG_ERR, "shutdown fails: %s", strerror(errno));
  }
  if (cgiErrno == 4) {
    logEmit(LOG_ERR, "%s", "fails to serve cgi query");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
