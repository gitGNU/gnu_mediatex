/*=======================================================================
 * Version: $Id: connect.c,v 1.3 2015/06/03 14:03:33 nroche Exp $
 * Project: MediaTeX
 * Module : server/connect
 *
 * Manage socket connexion to the server and sending recordTree

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

#include "../misc/log.h"
#include "../misc/address.h"
#include "../misc/tcp.h"
#include "../misc/signals.h"
#include "../memory/recordTree.h"
#include "connect.h"

#include <sys/stat.h> // open
#include <fcntl.h>    //
#include <signal.h>

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
  logCommon(LOG_DEBUG, "%s", "buildServerAddress");

  // already done
  if (server->address.sin_family) goto end;

  // convert port into char
  if (sprintf(service, "%i", server->mdtxPort) < 0) {
    logCommon(LOG_ERR, "%s", "sprintf cannot convert port into service");
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
       logCommon(LOG_ERR, "%s", "buildServerAddress fails");
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
   logCommon(LOG_NOTICE, "sigalarmManager get %i", unused);
   return;
}

/*=======================================================================
 * Function   : connectServer
 * Description: build a socket to connect a server
 * Synopsis   : int connectServer(char* host, int port)
 * Input      : char* host = host to connect (may be an IP)
 *              int port   = port to connect
 * Output     : socket descriptor or -1 on error;

 * TODO       : do we not should use a lock to prevent 2 threads to
 *              access the signal manager ?
 =======================================================================*/
int 
connectServer(Server* server)
{
  int rc = -1;
  int socket = -1;
  struct sigaction action;
  int err = 0;

  checkServer(server);
  logCommon(LOG_DEBUG, "connectServer %s", server->fingerPrint);

  // build server address if not already done
  if (server->address.sin_family == 0 && !buildServerAddress(server))
    goto error;

  // set the alarm manager
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  action.sa_handler = sigalarmManager;
  if (sigaction(SIGALRM, &action, 0) != 0) {
    logCommon(LOG_ERR, "%s", "sigaction fails: %s", strerror(errno));
    goto error;
  }

  /* open connection to server */
  if (!enableAlarm()) goto error;
  alarm(1);
  socket = connectTcpSocket(&server->address);
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
      logCommon(LOG_INFO, "cannot reach %s:%i",
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
    logCommon(LOG_ERR, "%s", "connectServer fails");
  }
 end:

  return rc;
}

/*=======================================================================
 * Function   : upgradeServer
 * Description: Send a cache index to the server.
 *              In fact this is the only kind of message the server will
 *              accept from sockets.
 *
 * Synopsis   : int upgradeServer(int socket, RecordTree* tree)
 * Input      : int socket = socket to use
 *              RecordTree* tree = what we send
 *              fingerPrint      = original fingerprint when Natted
 * Output     : TRUE on success

 * Note: call by daemon (notify), have (wrapper) and I guess cgi-client 
 =======================================================================*/
int 
upgradeServer(int socket, RecordTree* tree, char* fingerPrint)
{
  int rc = FALSE;
  Record* record = 0;
  RGIT* curr = 0;
  struct tm date;
#ifndef utMAIN
  char* key = 0;
#endif

  checkRecordTree(tree);
  checkCollection(tree->collection);
  
  logCommon(LOG_DEBUG, "upgradeServer %s %s",
	  tree->collection->label, strMessageType(tree->messageType));
  
#ifndef utMAIN
  // cypher the socket
  key = tree->collection->serverTree->aesKey;
  if (!aesInit(&tree->aes, key, ENCRYPT)) goto error;
  tree->doCypher = TRUE;
#endif

  // send content
  tree->aes.fd = socket;
  if (!serializeRecordTree(tree, 0, fingerPrint)) goto error;

  if (shutdown(socket, SHUT_WR) == -1) {
    logCommon(LOG_ERR, "shutdown fails: %s", strerror(errno));
    goto error;
  }
  
  // add some trace for debugging
  if(tree != 0 && tree->records != 0) {
    while((record = rgNext_r(tree->records, &curr)) != 0) {
      localtime_r(&record->date, &date);
      logCommon(LOG_INFO, "%c "
	      "%04i-%02i-%02i,%02i:%02i:%02i "
	      "%*s %*s %*llu %s",
	      (record->type & 0x3) == DEMAND?'D':
	      (record->type & 0x3) == SUPPLY?'S':'?',
	      date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	      date.tm_hour, date.tm_min, date.tm_sec,
	      MAX_SIZE_HASH, record->server->fingerPrint, 
	      MAX_SIZE_HASH, record->archive->hash, 
	      MAX_SIZE_SIZE, (long long unsigned int)record->archive->size,
	      record->extra?record->extra:"");
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "%s", "upgradeServer fails");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "../parser/confFile.tab.h"
GLOBAL_STRUCT_DEF;

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
  //fprintf(stderr, "\t\t---\n");

  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for connect module.
 * Synopsis   : ./utconnect
 * Input      : N/A
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = 0;
  Collection* coll = 0;
  Archive* archive = 0;
  Server* server = 0;
  Record* record = 0;
  RecordTree* tree = 0;
  char* extra = 0;
  int socket = -1;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS;
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.debugCommon = TRUE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(conf = getConfiguration())) goto error;
  if (!parseConfiguration(conf->confFile)) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!expandCollection(coll)) goto error;

  // new record tree
  if ((tree = createRecordTree())== 0) goto error;
  tree->collection = coll;
  strncpy(tree->fingerPrint, coll->userFingerPrint, MAX_SIZE_HASH);

  // new server as localhost:12345
  if (!(server = addServer(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"))) 
    goto error;
  strncpy(server->host, "localhost", MAX_SIZE_HOST);
  server->mdtxPort = 12345;

  // record 1
  if (!(archive = addArchive(coll, "hash1", 123))) goto error;
  if (!(extra = createString("path1"))) goto error;
  if (!(record = newRecord(server, archive, DEMAND, extra))) goto error;
  if (!rgInsert(tree->records, record)) goto error;

  // record 2
  if (!(archive = addArchive(coll, "hash2", 456))) goto error;
  if (!(extra = createString("path2"))) goto error;
  if (!(record = newRecord(server, archive, DEMAND, extra))) goto error;   
  if (!rgInsert(tree->records, record)) goto error;

  // check alarm signal do not break the stack
  if ((socket = connectServer(server)) == -1) {
    if ((socket = connectServer(server)) == -1) {
      if ((socket = connectServer(server)) == -1) goto end;
    }
  }

  // if there is a server listenning, send the record tree
  if (!upgradeServer(socket, tree, 0)) goto error;
  /************************************************************************/

 end:
  rc = TRUE;
 error:
  tree = destroyRecordTree(tree);
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

