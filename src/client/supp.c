/*=======================================================================
 * Project: MediaTeX
 * Module : supp
 *
 * Manage local supports

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
#include "client/mediatex-client.h"

/*=======================================================================
 * Function   : mdtxMount
 * Description: Mount an iso support
 * Synopsis   : int mdtxMount(char* iso, char* target)
 * Input      : char* iso
 *              char* target
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxMount(char* iso, char* target)
{
  int rc = FALSE;
  char *argv[] = {0, 0, 0, 0, 0};
  int isBlockDev = FALSE;

  checkLabel(iso);
  checkLabel(target);
  logMain(LOG_DEBUG, "mdtx mount %s", iso);
  
  if (!(argv[0] = createString(getConfiguration()->scriptsDir))
      || !(argv[0] = catString(argv[0], "/mount.sh"))) 
    goto error;

  argv[2] = target;
  if (!(getDevice(iso, &argv[1]))) goto error;
  
  // check if we have a block device or a normal file
  if (!isBlockDevice(argv[1], &isBlockDev)) goto error;
  if (!isBlockDev) argv[3] = ",loop";

  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, "root", 0, FALSE)) goto error;
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxMount fails");
  } 
  if (argv[0]) destroyString(argv[0]);
  if (argv[1]) destroyString(argv[1]);
  return rc;
}

/*=======================================================================
 * Function   : mdtxUmount
 * Description: call umount
 * Synopsis   : int mdtxUmount(char* target)
 * Input      : char* target
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxUmount(char* target)
{
  int rc = FALSE;
  char *argv[] = {0, 0, 0};

  checkLabel(target);
  logMain(LOG_DEBUG, "mdtx umount %s", target);

  if (!(argv[0] = createString(getConfiguration()->scriptsDir))
    || !(argv[0] = catString(argv[0], "/umount.sh"))) 
    goto error;

  argv[1] = target;

  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, "root", 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUmount fails");
  } 
  if (argv[0]) destroyString(argv[0]);
  return rc;
}

/*=======================================================================
 * Function   : updateSupport 
 * Description: update the status of a registered support
 * Synopsis   : int updateSupport(char* label, char* status)
 * Input      : char* label = support to update
 *            : char* status = new status for this support
 * Output     : N/A
 =======================================================================*/
int
mdtxUpdateSupport(char* label, char* status)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support *supp = 0;

  logMain(LOG_DEBUG, "mdtxUpdateSupport");

  if (!(conf = getConfiguration())) goto error;
  if (!(supp = mdtxGetSupport(label))) goto error;

  // update support status
  strncpy(supp->status, status, MAX_SIZE_STAT);
  
  conf->fileState[iSUPP] = MODIFIED;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUpdateSupport fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : lsSupport 
 * Description: list all availables supports
 * Synopsis   : int lsSupport()
 * Input      : N/A
 * Output     : stdout
 * Note       : supports validity is updated and indeed the database too
 =======================================================================*/
int 
mdtxLsSupport()
{
  int rc = FALSE;
  Configuration* conf = 0;
  RG* supports = 0;
  Support *supp = 0;

  logMain(LOG_DEBUG, "mdtxLsSupport");
  if (!(conf = getConfiguration())) goto error;
  if (!loadConfiguration(CFG|SUPP)) goto error;
  supports = conf->supports;

  rgRewind(supports);
  while ((supp = rgNext(supports))) {
    if (!scoreSupport(supp, &conf->scoreParam)) goto error;
  }

  printf("%5s %*s %s\n", 
	 "score", MAX_SIZE_STAT, "state", "label");  

  while ((supp = rgNext(supports))) {     
    printf("%5.2f %*s %s\n", 
	   supp->score, MAX_SIZE_STAT, supp->status, supp->name);
  }
  printf("\n");

  rc = TRUE;
 error:
 if (!rc) {
   logMain(LOG_ERR, "mdtxLsSupport fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : mdtxAddSupport 
 * Description: add a new available support
 * Synopsis   : int mdtxAddSupport(char* label, char* path)
 * Input      : char* label = support's label
 *              char* path = path to the device that host the support
 * Output     : N/A
 =======================================================================*/
int 
mdtxAddSupport(char* label, char* path)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support *supp = 0;
  time_t now = 0;

  logMain(LOG_DEBUG, "mdtxAddSupport");
  if (isEmptyString(path)) goto error;
  if (isEmptyString(label)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if ((now = currentTime()) == -1) goto error;
  
  // name begining with '/' are reserved for support files
  if (*label == '/') {
    logMain(LOG_ERR, "support's name cannot begin with '/'");
    goto error;
  }

  // look for this name in the support Ring
  if (!loadConfiguration(SUPP)) goto error;
  if ((supp = getSupport(label))) {
    logMain(LOG_ERR, "a support labeled \"%s\" already exist", label);
    goto error;
  }

  // create and complete support object
  if ((supp = addSupport(label)) == 0) goto error;
  supp->firstSeen = now;
  if (!doCheckSupport(supp, path)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "error while adding support %s from %s", 
	    label, path);
    if (supp) delSupport(supp);
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxAddFile
 * Description: add a file in place of a new available support
 * Synopsis   : int mdtxAddFile(char* path)
 * Input      : char* path = path of the file to add
 * Output     : N/A
 * Note       : support's label will be the absolute path
 =======================================================================*/
int 
mdtxAddFile(char* path)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support *supp = 0;
  time_t now = 0;
  char* absolutePath = 0;

  logMain(LOG_DEBUG, "mdtxAddFile %s", path);
  if (isEmptyString(path)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if ((now = currentTime()) == -1) goto error;

  // get the absolute path of the file
  if (!(absolutePath = getAbsolutePath(path))) goto error;

  // look for this name in the support Ring
  if (!loadConfiguration(SUPP)) goto error;
  if ((supp = getSupport(absolutePath))) {
    logMain(LOG_ERR, "a support labeled \"%s\" already exist", 
	    absolutePath);
    goto error;
  }

  // create and complete support object
  if ((supp = addSupport(absolutePath)) == 0) goto error;
  supp->firstSeen = now;
  if (!doCheckSupport(supp, absolutePath)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxAddFile fails");
    if (supp) delSupport(supp);
  }
  destroyString(absolutePath);
  return rc;
}


/*=======================================================================
 * Function   : addFinalSupplies
 * Description: fill recordTree with content to tell to the server
 * Synopsis   : static int addFinalSupplies(Collection* coll, 
 *                                Support* supp, char* path,
 *                                RecordTree* tree) 
 * Input      : Collection* coll
 *              Support* supp
 *              char* path: path to the support
 * Output     : RecordTree* tree
 *              TRUE on success
 =======================================================================*/
static int 
addFinalSupplies(Collection* coll, Support* supp, char* path,
		 RecordTree* tree) 
{
  int rc = FALSE;
  Archive* archive = 0;
  Record* record = 0;
  char* device = 0;
  char buf[MAX_SIZE_STRING] = "";
  char* extra = 0;

  logMain(LOG_DEBUG, "addFinalSupplies: %s:%lli",
	  supp->fullMd5sum, (long long int)supp->size);

  // tels "/PATH:{CONF_SUPPD}/NAME" 
  if (snprintf(buf, MAX_SIZE_STRING, "%s%s%s", 
	       path, CONF_SUPPD, supp->name) >= MAX_SIZE_STRING) {
    logMain(LOG_ERR, "buffer too few to copy path and support name");
    goto error;
  }

  if (!getLocalHost(coll)) goto error;
  if (!(extra = createString(buf))) goto error;

  if (!(archive = addArchive(coll, supp->fullMd5sum, supp->size))) 
    goto error;

  if (!(record = addRecord(coll, coll->localhost, archive, SUPPLY, extra)))
    goto error;
  extra = 0;
  if (!avl_insert(tree->records, record)) {
    logMain(LOG_ERR, "cannot add record to tree (already there?)");
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "addFinalSupplies fails");
  }
  destroyString(extra);
  destroyString(device);
  return rc;
}

/*=======================================================================
 * Function   : notifyHave
 * Description: Tell the local server we have a support for him
 * Synopsis   : static int notifyHave(Support* supp, char* path)
 * Input      : Support* supp = the support we provide
 *            : char* path = path to the device that host the support
 * Output     : N/A
 =======================================================================*/
static int 
notifyHave(Support* supp, char* path) 
{
  int rc = FALSE;
  int socket = 0;
  Configuration* conf = 0;
  Collection* coll = 0;
  RecordTree* tree = 0;
  RGIT* curr1 = 0;
  RGIT* curr2 = 0;
  char* name = 0;
  int isShared = FALSE;

  // warning tcpRead seems to read more than 1 and erase the stack
  char reply[576];

  logMain(LOG_DEBUG, "notifyHave");
  if (supp == 0) goto error;
  if (isEmptyString(path) || *path != '/') {
    logMain(LOG_ERR, 
	    "daemon need an absolute path for support extraction");
    goto error;
  }

  conf = getConfiguration();
  if (!loadConfiguration(CFG)) goto error;
  if (!expandConfiguration()) goto error;
  if (!(tree = createRecordTree())) goto error;    
  tree->messageType = HAVE;

  // for each collections
  while ((coll = rgNext_r(conf->collections, &curr1))) {
    tree->collection = coll;

    // find the support if used by this collection
    while ((name = rgNext_r(coll->supports, &curr2))) {
      if (!strncmp(name, supp->name, MAX_SIZE_STRING)) break;
    }
    if (name == 0) continue;
    isShared = TRUE;

    // add final supplies    
    if (!addFinalSupplies(coll, supp, path, tree)) goto error;

    // trick to use 127.0.0.1 instead of www IP address
    buildSocketAddressEasy(&coll->localhost->address, 
			   0x7f000001, coll->localhost->mdtxPort);
    
    if ((socket = connectServer(coll->localhost)) == -1) goto error;
    if (!upgradeServer(socket, tree, 0)) goto error;
    
    // do not use 127.0.0.1 anymore
    coll->localhost->address.sin_family = 0;
    
    // wait until server shut down the sockect
    if (!env.dryRun) tcpRead(socket, reply, 1);

    if (!diseaseRecordTree(tree)) goto error;
  }
  
  if (!isShared) {
    logMain(LOG_NOTICE, "the %s support is not share by any collection",
  	    supp->name);
  }
  rc = TRUE;
 error:
  if (!rc) {
  logMain(LOG_ERR, "fails to launch extraction on the %s support", 
	  supp?supp->name:"unknown");
  }
  if (!env.dryRun && socket != -1) close(socket);
  tree = destroyRecordTree(tree);
  return rc;
}

/*=======================================================================
 * Function   : mdtxHaveSupport 
 * Description: check an already registered support
 * Synopsis   : int mdtxHaveSupport(char* label, char* path)
 * Input      : char* label = support provided
 *            : char* path = path to the device that host the support
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxHaveSupport(char* label, char* path)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support *supp = 0;
  char* absPath = 0;

  logMain(LOG_DEBUG, "mdtxHaveSupport");
  if (isEmptyString(path)) goto error;
  if (isEmptyString(label)) goto error;  
  if (!(conf = getConfiguration())) goto error;
  if (*label == '/') {
    logMain(LOG_ERR, "have function do not handle support file");
    goto error;
  }
  if (!(supp = mdtxGetSupport(label))) goto error;
  if (supp == 0) goto error;

  if (isEmptyString(path)) goto error;
  if (!doCheckSupport(supp, path)) goto error;
  
  // maybe we should updgrade daemon's md5sumsDB 
  // before to launch extraction ?

  if (!env.noRegression) {
    // check if daemon is awake
    if (access(conf->pidFile, R_OK) == -1) {
      logMain(LOG_INFO, "cannot read daemon's pid file: %s", 
	      strerror(errno));
      goto end;
    }
  }
  
  // ask the daemon to start extraction on the support
  if (!(absPath = getAbsolutePath(path))) goto error;
  if (!notifyHave(supp, absPath)) goto error;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxHaveSupport fails");
  }
  absPath = destroyString(absPath);
  return rc;
}

/*=======================================================================
 * Function   : mdtxDelSupport 
 * Description: remove a support
 * Synopsis   : int mdtxDelSupport(char* label)
 * Input      : char* label = the support to remove
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxDelSupport(char* label)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support *supp = 0;

  logMain(LOG_DEBUG, "mdtxDelSupport");

  // withdraw support
  if (!mdtxWithdrawSupport(label, 0)) goto error;

  // remove support
  if (!(conf = getConfiguration())) goto error;
  if (!(supp = mdtxGetSupport(label))) goto error;
  if (!delSupport(supp)) goto error;
  
  conf->fileState[iSUPP] = MODIFIED;
  rc = TRUE;
error:
  if (!rc) {
    logMain(LOG_ERR, "fails to remove the %s support", supp->name);
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
