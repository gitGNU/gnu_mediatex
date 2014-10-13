
/*=======================================================================
 * Version: $Id: conf.c,v 1.1 2014/10/13 19:38:46 nroche Exp $
 * Project: MediaTeX
 * Module : wrapper/conf
 *
 * Manage mediatex.conf configuration file

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Nicolas Roche
 
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

#include "../mediatex.h"
#include "../misc/log.h"
#include "../misc/command.h"
#include "../memory/confTree.h"
#include "../parser/confFile.tab.h"
#include "../common/openClose.h"
#include "supp.h"
#include "serv.h"
#include "conf.h"

#include <sys/stat.h>
#include <time.h>


/*=======================================================================
 * Function   : newCollection
 * Description: add a collection to a configuration
 * Synopsis   : int addCollection(Collection* coll)
 * Input      : Collection* coll = the collection's labels
 * Output     : TRUE on success
 * Note       : Collection parameter is only used to pass 3 strings.
 *              This function is call to create both local and remote 
 *              collections.
 *              Local collection <=> coll->masterHost=localhost or empty
 =======================================================================*/
int 
mdtxAddCollection(Collection* coll)
{ 
  int rc = FALSE;
  Configuration* conf = NULL;
  Collection* self = NULL;
  char buf[MAX_SIZE_COLL + MAX_SIZE_HOST + 8];
  char* argv[3] = {NULL, NULL, NULL};
  char* cvsFile = NULL;
  struct stat sb;

  checkCollection(coll);
  checkLabel(coll->label);
  logEmit(LOG_DEBUG, "%s", "add a new collection (or re-add it)");

  //if (!allowedUser(env.confLabel)) goto error;
  if (!(conf = getConfiguration())) goto error;
  
  // default values if not provided
  if (isEmptyString(coll->masterLabel) &&
      !(coll->masterLabel = createString(env.confLabel)))
    goto error;
  if (isEmptyString(coll->masterHost)) {
    strncpy(coll->masterHost, "localhost", MAX_SIZE_HOST);
  }

  // call script in order to eventually repare collection
  sprintf(buf, "%s-%s@%s:%i", 
	  coll->masterLabel, coll->label, 
	  coll->masterHost, coll->masterPort);
  argv[0] = createString(conf->scriptsDir);
  argv[0] = catString(argv[0], "/new.sh");
  argv[1] = buf;
  
#ifndef utMAIN
  if (!execScript(argv, NULL, NULL, FALSE)) goto error;
#endif

  // check if script realy success
  if (!loadConfiguration(CONF)) goto error;
  if (!(cvsFile = createString(conf->cvsDir)) 
      || !(cvsFile =  catString(cvsFile, "/"))
      || !(cvsFile =  catString(cvsFile, env.confLabel))
      || !(cvsFile =  catString(cvsFile, "-"))
      || !(cvsFile =  catString(cvsFile, coll->label))
      || !(cvsFile =  catString(cvsFile, "/servers.txt")))
    goto error;
  if (stat(cvsFile, &sb) == -1) {
    logEmit(LOG_INFO, "stat: %s", strerror(errno));
    logEmit(LOG_DEBUG, "(stat was looking for %s)", cvsFile);
    logEmit(LOG_NOTICE,
	    "Please send your collection key to %s server admin",
	    coll->masterHost);
    goto end;
  }

  // register collection into configuration file
  if (!(self = addCollection(coll->label))) goto error;
  self->masterLabel = coll->masterLabel;
  coll->masterLabel = NULL; // consume it
  strncpy(self->masterHost, coll->masterHost, MAX_SIZE_HOST);
  self->masterPort = coll->masterPort;
  if (!expandCollection(self)) goto error;
  if (!populateCollection(self)) goto error;
  conf->fileState[iCONF] = MODIFIED; // upgrade conf

  // register collection into servers.txt file
  if (!loadCollection(self, SERV)) goto error; // upgrade keys
  if (!strncpy(self->masterHost, "localhost", MAX_SIZE_HOST)) {
    if (!(self->serverTree->master = getLocalHost(self))) goto error;
  }
  if (!releaseCollection(self, SERV)) goto error;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to add new collection");
    if (self) delCollection(self);
  }
  argv[0] = destroyString(argv[0]);
  cvsFile = destroyString(cvsFile);
  // coll is freed by parser
  return rc;
}


/*=======================================================================
 * Function   : delCollection
 * Description: del a collection to a configuration
 * Synopsis   : int delCollection(char* label)
 * Input      : char* label = the collection to delete
 * Output     : TRUE on success
 * Note       : We do not use standard functions as we try to never 
 *              fails so as not to let the user in a blocked state.
 =======================================================================*/
int 
mdtxDelCollection(char* label)
{ 
  int rc = FALSE;
  Configuration* conf = NULL;
  Collection* coll = NULL;
  char* argv[3] = {NULL, NULL, NULL};

  logEmit(LOG_DEBUG, "%s", "del a collection");

  //if (!allowedUser(env.confLabel)) goto error;
  if (!(conf = getConfiguration())) goto error;

  // call script without removing ourself from severs.txt
  // so as to eventally repare collection (del/add)
  argv[0] = createString(conf->scriptsDir);
  argv[0] = catString(argv[0], "/free.sh");
  argv[1] = label;
#ifndef utMAIN
  if (!execScript(argv, NULL, NULL, FALSE)) goto error;
#endif

  // upgrade configuration file
  // note: do not expand collection
  if (!loadConfiguration(CONF)) goto error;
  if (!(coll = getCollection(label))) {
    logEmit(LOG_WARNING, "there was no collection named '%s'", label);
    goto end;
  }
  if (!delCollection(coll)) goto error;
  conf->fileState[iCONF] = MODIFIED;
  
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to del collection");
  }
  if (argv[0]) free(argv[0]);
  return rc;
}


/*=======================================================================
 * Function   : listCollection
 * Description: print label of collections in configuration
 * Synopsis   : listCollection()
 * Input      : N/A
 * Output     : number of matching collections
 =======================================================================*/
int
mdtxListCollection()
{
  int rc = FALSE; 
  Configuration* conf = NULL;
  Collection* collection = NULL;
  int nb = 0;

  // search into the configuration
  if (!loadConfiguration(CONF)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (conf->collections != NULL) {

    if (conf->collections != NULL) {
      if (!rgSort(conf->collections, cmpCollection)) {
	logEmit(LOG_ERR, "%s", "fails to sort collections ring");
	goto error;
      }
    }
    
    while((collection = rgNext(conf->collections)) != NULL) {
      printf("%s%s", nb++?" ":"", collection->label);
    }
  }
  printf("\n");
  fflush(stdout);
  rc = TRUE;
 error:
  return rc;
}


/*=======================================================================
 * Function   : shareSupport
 * Description: add support's label into collection's support ring(s)
 * Synopsis   : int shareSupport(char* sLabel, char* cLabel)
 * Input      : char* sLabel = support to share
 *              char* cLabel = collection to share with, NULL for all
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxShareSupport(char* sLabel, char* cLabel)
{
  int rc = FALSE;
  Configuration* conf = NULL;
  Support* supp = NULL;
  Collection* coll = NULL;

  checkLabel(sLabel);
  if (!loadConfiguration(CONF)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!(supp = mdtxGetSupport(sLabel))) goto error;

  // for all collections
  if (cLabel == NULL) {
    if (conf->collections != NULL) {
      rgRewind(conf->collections);
      while((coll = rgNext(conf->collections)) != NULL) {
	if (!addSupportToCollection(supp, coll)) goto error;
	if (!wasModifiedCollection(coll, SERV)) goto error;
      }
    }
    coll = NULL;
  }
  else {
    // for the provided collection
    if (!(coll = mdtxGetCollection(cLabel))) goto error;
    if (!addSupportToCollection(supp, coll)) goto error;
    if (!wasModifiedCollection(coll, SERV)) goto error;
  }

  conf->fileState[iCONF] = MODIFIED;
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "fails to share \"%s\" support with %s collection%s", 
	    supp?supp->name:"unknown", 
	    cLabel?cLabel:"all", cLabel?"":"s");
  }
  return rc;
}

/*=======================================================================
 * Function   : withdrawSupport
 * Description: remove support's label from collection's support ring(s)
 * Synopsis   : int withdrawSupport(char* sLabel, char* cLabel)
 * Input      : char* sLabel = support to withdraw
 *              char* cLabel = collection to withdraw from ; NULL for all
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxWithdrawSupport(char* sLabel, char* cLabel)
{
  int rc = FALSE;
  Configuration* conf = NULL;
  Support* supp = NULL;
  Collection* coll = NULL;

  if (!loadConfiguration(CONF)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!(supp = mdtxGetSupport(sLabel))) goto error;

  // for all collections
  if (cLabel == NULL) {
    if (conf->collections != NULL) {
      rgRewind(conf->collections);
      while((coll = rgNext(conf->collections)) != NULL) {
	if (!delSupportFromCollection(supp, coll)) goto error;
	if (!wasModifiedCollection(coll, SERV)) goto error;
      }
    }
    coll = NULL;
    coll = NULL;
  }
  else {
    // for the provided collection
    if (!(coll = mdtxGetCollection(cLabel))) goto error;
    if (!delSupportFromCollection(supp, coll)) goto error;
    if (!wasModifiedCollection(coll, SERV)) goto error;
  }

  conf->fileState[iCONF] = MODIFIED;
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, 
	    "fails to withdraw \"%s\" support from %s collection%s", 
	    supp?supp->name:"unknown", 
	    coll?coll->label:"all", coll?"":"s");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
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
  //fprintf(stderr, "  ---\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2010/12/10
 * Description: entry point for conf module
 * Synopsis   : ./utconf
 * Input      : N/A
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll3 = NULL;
  Collection* coll4 = NULL;
  char* supp = "SUPP21_logo.part1";
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
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll3 = addCollection("coll3"))) goto error;

  logEmit(LOG_NOTICE, "%s", "*** List collections: ");
  if (!mdtxListCollection()) goto error;

  logEmit(LOG_NOTICE, "%s", "*** Add a collection:");
  if (!(coll4 = createCollection())) goto error;
  strncpy(coll4->label, "coll4", MAX_SIZE_COLL);
  strncpy(coll4->masterHost, "localhost", MAX_SIZE_HOST);
  if (!mdtxAddCollection(coll4)) goto error;
  coll4 = destroyCollection(coll4);

  logEmit(LOG_NOTICE, "%s", "*** List collections:");
  if (!mdtxListCollection()) goto error;

  logEmit(LOG_NOTICE, "%s", "*** Del collection coll 4:");
  if (!mdtxDelCollection("coll4")) goto error;
  if (!saveConfiguration("topo")) goto error;

  logEmit(LOG_NOTICE, "%s", "*** Share a support:");
  if (!mdtxShareSupport(supp, "coll3")) goto error;
  if (!saveConfiguration("topo")) goto error;

  logEmit(LOG_NOTICE, "%s", "*** Share a support second time:");
  if (!mdtxShareSupport(supp, "coll3")) goto error;
  
  logEmit(LOG_NOTICE, "%s", "*** Share support with all collections:");
  if (!mdtxShareSupport(supp, NULL)) goto error;
  if (!saveConfiguration("topo")) goto error;

  logEmit(LOG_NOTICE, "%s", "*** Withdraw a support:");
  if (!mdtxWithdrawSupport(supp, "coll3")) goto error;
  if (!saveConfiguration("topo")) goto error;

  logEmit(LOG_NOTICE, "%s", "*** Withdraw second time:");
  if (!mdtxWithdrawSupport(supp, "coll3")) goto error;

  logEmit(LOG_NOTICE, "%s", "*** Withdraw support from all collections:");
  if (!mdtxWithdrawSupport(supp, NULL)) goto error;
  if (!saveConfiguration("topo")) goto error;
  /************************************************************************/
  
  rc = TRUE;
 error:
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
