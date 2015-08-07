
/*=======================================================================
 * Version: $Id: conf.c,v 1.6 2015/08/07 17:50:28 nroche Exp $
 * Project: MediaTeX
 * Module : conf
 *
 * Manage mediatex.conf configuration file

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
#include "client/mediatex-client.h"


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
  Configuration* conf = 0;
  Collection* self = 0;
  char buf[MAX_SIZE_COLL + MAX_SIZE_HOST + 8];
  char* argv[3] = {0, 0, 0};
  char* cvsFile = 0;
  struct stat sb;

  checkCollection(coll);
  checkLabel(coll->label);
  logMain(LOG_DEBUG, "add a new collection (or re-add it)");
  if (!(conf = getConfiguration())) goto error;
  
  // default value to mdtx if not provided
  if (isEmptyString(coll->masterLabel)) {
    if (!(coll->masterLabel = createString(env.confLabel))) goto error;
  }

  // default value to localhost (first) if not provided
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
  
  if (!env.noRegression) {
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  // check if script realy success
  if (!(cvsFile = createString(conf->cvsDir)) 
      || !(cvsFile =  catString(cvsFile, "/"))
      || !(cvsFile =  catString(cvsFile, env.confLabel))
      || !(cvsFile =  catString(cvsFile, "-"))
      || !(cvsFile =  catString(cvsFile, coll->label))
      || !(cvsFile =  catString(cvsFile, "/servers.txt")))
    goto error;
  if (stat(cvsFile, &sb) == -1) {
    logMain(LOG_INFO, "stat: %s", strerror(errno));
    logMain(LOG_DEBUG, "(stat was looking for %s)", cvsFile);
    logMain(LOG_NOTICE,
	    "Please send your collection key to %s server admin",
	    coll->masterHost);
    goto end;
  }

  // register collection into configuration file
  if (!loadConfiguration(CFG)) goto error;
  if (!(self = addCollection(coll->label))) goto error;
  destroyString(self->masterLabel);
  self->masterLabel = coll->masterLabel;
  coll->masterLabel = 0; // consume it
  strncpy(self->masterHost, coll->masterHost, MAX_SIZE_HOST);
  self->masterPort = coll->masterPort;
  if (!expandCollection(self)) goto error;
  if (!populateCollection(self)) goto error;
  conf->fileState[iCFG] = MODIFIED; // upgrade conf

  // register collection into servers.txt file
  if (!(self->serverTree->master = getLocalHost(self))) goto error;
  if (!loadCollection(self, SERV)) goto error; // upgrade keys
  if (!releaseCollection(self, SERV)) goto error;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to add new collection");
    if (self) delCollection(self);
  }
  argv[0] = destroyString(argv[0]);
  cvsFile = destroyString(cvsFile);
  // coll is free by parser
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
  Configuration* conf = 0;
  Collection* coll = 0;
  char* argv[3] = {0, 0, 0};

  logMain(LOG_DEBUG, "del a collection");

  //if (!allowedUser(env.confLabel)) goto error;
  if (!(conf = getConfiguration())) goto error;

  // call script without removing ourself from severs.txt
  // so as to eventally repare collection (del/add)
  argv[0] = createString(conf->scriptsDir);
  argv[0] = catString(argv[0], "/free.sh");
  argv[1] = label;

  if (!env.noRegression) {
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  // upgrade configuration file
  // note: do not expand collection
  if (!loadConfiguration(CFG)) goto error;
  if (!(coll = getCollection(label))) {
    logMain(LOG_WARNING, "there was no collection named '%s'", label);
    goto end;
  }
  if (!delCollection(coll)) goto error;
  conf->fileState[iCFG] = MODIFIED;
  
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to del collection");
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
  Configuration* conf = 0;
  Collection* collection = 0;
  int nb = 0;

  // search into the configuration
  if (!loadConfiguration(CFG)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (conf->collections != 0) {

    if (conf->collections != 0) {
      if (!rgSort(conf->collections, cmpCollection)) {
	logMain(LOG_ERR, "fails to sort collections ring");
	goto error;
      }
    }
    
    while((collection = rgNext(conf->collections)) != 0) {
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
 *              char* cLabel = collection to share with, 0 for all
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxShareSupport(char* sLabel, char* cLabel)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support* supp = 0;
  Collection* coll = 0;

  checkLabel(sLabel);
  if (!loadConfiguration(CFG)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!(supp = mdtxGetSupport(sLabel))) goto error;

  // for all collections
  if (cLabel == 0) {
    if (conf->collections != 0) {
      rgRewind(conf->collections);
      while((coll = rgNext(conf->collections)) != 0) {
	if (!addSupportToCollection(supp, coll)) goto error;
	if (!wasModifiedCollection(coll, SERV)) goto error;
      }
    }
    coll = 0;
  }
  else {
    // for the provided collection
    if (!(coll = mdtxGetCollection(cLabel))) goto error;
    if (!addSupportToCollection(supp, coll)) goto error;
    if (!wasModifiedCollection(coll, SERV)) goto error;
  }

  conf->fileState[iCFG] = MODIFIED;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to share \"%s\" support with %s collection%s", 
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
 *              char* cLabel = collection to withdraw from ; 0 for all
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxWithdrawSupport(char* sLabel, char* cLabel)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support* supp = 0;
  Collection* coll = 0;

  if (!loadConfiguration(CFG)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!(supp = mdtxGetSupport(sLabel))) goto error;

  // for all collections
  if (cLabel == 0) {
    if (conf->collections != 0) {
      rgRewind(conf->collections);
      while((coll = rgNext(conf->collections)) != 0) {
	if (!delSupportFromCollection(supp, coll)) goto error;
	if (!wasModifiedCollection(coll, SERV)) goto error;
      }
    }
    coll = 0;
    coll = 0;
  }
  else {
    // for the provided collection
    if (!(coll = mdtxGetCollection(cLabel))) goto error;
    if (!delSupportFromCollection(supp, coll)) goto error;
    if (!wasModifiedCollection(coll, SERV)) goto error;
  }

  conf->fileState[iCFG] = MODIFIED;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, 
	    "fails to withdraw \"%s\" support from %s collection%s", 
	    supp?supp->name:"unknown", 
	    coll?coll->label:"all", coll?"":"s");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
