/*=======================================================================
 * Project: MediaTeX
 * Module : connect
 *
 * test for connect

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

#include "mediatex.h"

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
  env = envUnitTest;
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
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
  strncpy(tree->fingerPrint, coll->userFingerPrint, MAX_SIZE_MD5);

  // new server as localhost:12345
  if (!(server = addServer(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"))) 
    goto error;
  strncpy(server->host, "localhost", MAX_SIZE_HOST);
  server->mdtxPort = 12345;

  // record 1
  if (!(archive = addArchive(coll, "hash1", 123))) goto error;
  if (!(extra = createString("path1"))) goto error;
  if (!(record = newRecord(server, archive, DEMAND, extra))) goto error;
  if (!avl_insert(tree->records, record)) {
    logMain(LOG_ERR, "cannot add record (already there?)");
    goto error;
  }

  // record 2
  if (!(archive = addArchive(coll, "hash2", 456))) goto error;
  if (!(extra = createString("path2"))) goto error;
  if (!(record = newRecord(server, archive, DEMAND, extra))) goto error;
  if (!avl_insert(tree->records, record)) {
    logMain(LOG_ERR, "cannot add record (already there?)");
    goto error;
  }

  // check alarm signal do not break the stack
  if ((socket = connectServer(server)) == -1) {
    if ((socket = connectServer(server)) == -1) {
      if ((socket = connectServer(server)) == -1) goto end;
    }
  }

  // send the record tree to a listening server
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

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

