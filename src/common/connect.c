/*=======================================================================
 * Project: MediaTeX
 * Module : connect
 *
 * Open and write to servers socket

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche

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

/*=======================================================================
 * Function   : buildServerAddress
 * Description: build server address if not already done
 * Synopsis   : int buildServerAddress(Server* server)
 * Input      : Server* server = server to get IP
 * Output     : TRUE on success
 =======================================================================*/
int 
buildServerAddress(Server* server)
{
  int rc = FALSE;
  char service[10];

  checkServer(server);
  logCommon(LOG_DEBUG, "buildServerAddress");

  // already done
  if (server->address.sin_family) goto end;

  // convert port into char
  if (sprintf(service, "%i", server->mdtxPort) < 0) {
    logCommon(LOG_ERR, "sprintf cannot convert port into service");
    goto error;
  }
    
  // resolve DNS name into an IP
  if (!buildSocketAddress(&server->address,
			  server->host, "tcp", service)) {
    goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
       logCommon(LOG_ERR, "buildServerAddress fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : sigalarmManager
 * Description: Used to interrupt call to connectTcpSocket
 *              in order to stop connexion when server is not responding
 * Synopsis   : static void sigalarmManager(int unused)
 * Input      : int unused: not used
 * Output     : N/A
 =======================================================================*/
static void 
sigalarmManager(int unused)
{
  (void) unused;
  // nothing to do
   logCommon(LOG_INFO, "sigalarmManager get %i", unused);
   return;
}

/*=======================================================================
 * Function   : connectServer
 * Description: build a socket to connect a server
 * Synopsis   : int connectServer(Server* server)
 * Input      : Server* server: server to connect
 * Output     : socket descriptor or -1 on error;
 =======================================================================*/
int 
connectServer(Server* server)
{
  int rc = -1;
  int socket = -1;
  int err = 0;

  checkServer(server);
  logCommon(LOG_DEBUG, "connectServer %s", server->fingerPrint);

  // build server address if not already done
  if (server->address.sin_family == 0 && !buildServerAddress(server))
    goto error;

  /* open connection to server */
  if (!enableAlarm(sigalarmManager)) goto error;
  alarm(1);
  socket = env.dryRun?-2:connectTcpSocket(&server->address);
  err = errno;
  alarm(0);
  if (!disableAlarm()) goto error;

  if (socket == -1) { 
    //logCommon(LOG_DEBUG, "errno=%i: %s", err, strerror(err));
    switch (err) {
    case EINTR:   // interupted by alarm
    case SIGALRM:
      logCommon(LOG_WARNING, "too much time to reach %s:%i", 
	      server->host, server->mdtxPort);
      break;
    default:
      logCommon(LOG_NOTICE, "cannot reach %s:%i",
	      server->host, server->mdtxPort);
      break;
    }
    goto end; // do not display error message */
  }

  rc = socket;
  logCommon(LOG_INFO, "connected to %s (%s)", 
	  server->host, server->fingerPrint);
 error:
  if (rc == -1) {
    logCommon(LOG_ERR, "connectServer fails");
  }
 end:

  return rc;
}

/*=======================================================================
 * Function   : upgradeServer
 * Description: Send records to the server.
 * Synopsis   : int upgradeServer(int socket, 
 *                                   RecordTree* tree, char* fingerPrint)
 * Input      : int socket = socket to use
 *              RecordTree* tree = what we send
 *              fingerPrint      = original fingerprint when Natted
 * Output     : TRUE on success
 =======================================================================*/
int 
upgradeServer(int socket, RecordTree* tree, char* fingerPrint)
{
  int rc = FALSE;
  char* key = 0;

  logCommon(LOG_DEBUG, "upgradeServer");
  checkRecordTree(tree);
  checkCollection(tree->collection);
  
  // log content to send
  logRecordTree(LOG_COMMON, LOG_INFO, tree, fingerPrint);

  // cypher the socket
  key = tree->collection->serverTree->aesKey;
  if (!aesInit(&tree->aes, key, ENCRYPT)) goto error;
  tree->doCypher = env.noRegression?FALSE:TRUE;

  // send content
  tree->aes.fd = socket;
  if (!env.dryRun) {
    if (!serializeRecordTree(tree, 0, fingerPrint)) goto error;

    if (shutdown(socket, SHUT_WR) == -1) {
      logCommon(LOG_ERR, "shutdown fails: %s", strerror(errno));
      goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "upgradeServer fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

