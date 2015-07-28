/*=======================================================================
 * Version: $Id: supp.c,v 1.5 2015/07/28 11:45:45 nroche Exp $
 * Project: MediaTeX
 * Module : supp
 *
 * Manage local supports

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
 * Function   : mdtxMount
 * Description: call scp so as to not need setuid bit on daemon
 * Synopsis   : 
 * Input      : 
 *              
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
    logMain(LOG_ERR, "%s", "mdtxMount fails");
  } 
  if (argv[0]) destroyString(argv[0]);
  if (argv[1]) destroyString(argv[1]);
  return rc;
}

/*=======================================================================
 * Function   : mdtxUmount
 * Description: call umount
 * Synopsis   : 
 * Input      : 
 *              
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
    logMain(LOG_ERR, "%s", "mdtxUmount fails");
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

  logMain(LOG_DEBUG, "%s", "mdtxUpdateSupport");

  if (!(conf = getConfiguration())) goto error;
  if (!(supp = mdtxGetSupport(label))) goto error;

  // update support status
  strncpy(supp->status, status, MAX_SIZE_STAT);
  
  conf->fileState[iSUPP] = MODIFIED;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", "mdtxUpdateSupport fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : checkSupport 
 * Description: Do checksums on an available support
 * Synopsis   : int checkSupport(Support *supp, char* path)
 * Input      : Support *supp = the support object
 *              char* path = the device that host the support
 * Output     : TRUE on success
 *
 * Note       : supp->lastCheck: (O will force check)
 *              the input path is free
 =======================================================================*/
static int 
doCheckSupport(Support *supp, char* path)
{
  int rc = FALSE;
  time_t now = 0;
  time_t laps = 0;
  Md5Data data;

  logMain(LOG_DEBUG, "%s", "doCheckSupport");
  memset(&data, 0, sizeof(Md5Data));
  
  if ((data.path = createString(path)) == 0) {
    logMain(LOG_ERR, "%s", "cannot dupplicate path string");
    goto error;
  }
  
  // current date
  if ((now = currentTime()) == -1) goto error;
 
  // by default do not exists: full computation (no check)
  data.opp = MD5_SUPP_ADD;

  // check if support need to be full checked or not
  if (supp->lastCheck > 0) {
    // exists: quick check
    data.opp = MD5_SUPP_ID;
    laps = now - supp->lastCheck;
    if (getConfiguration()->checkTTL/2 >= 0 &&
	laps > getConfiguration()->checkTTL/2) {
      // nearly obsolete: full check
      data.opp = MD5_SUPP_CHECK;
      logMain(LOG_NOTICE, "support no checked since %d days: checking...", 
	      laps/60/60/24);
    }
  }
  
  // copy size and current checksums to compare them
  switch (data.opp) {
  case MD5_SUPP_CHECK:
    strncpy(data.fullMd5sum, supp->fullHash, MAX_SIZE_HASH);
  case MD5_SUPP_ID:
    data.size = supp->size;
    strncpy(data.quickMd5sum, supp->quickHash, MAX_SIZE_HASH);
  default:
    break;
  }

  // checksum computation
  rc = TRUE;
  if (!doMd5sum(&data)) {
      logMain(LOG_DEBUG, 
	      "internal error on md5sum computation for \"%s\" support", 
	      supp->name);
      rc = FALSE;
      goto error;
  }

  if (data.rc == MD5_FALSE_SIZE) {
    logMain(LOG_WARNING, "wrong size on \"%s\" support", supp->name);
    rc = FALSE;
  }
  if (data.rc == MD5_FALSE_QUICK) {
    logMain(LOG_WARNING, "wrong quick hash on \"%s\" support", supp->name);
    rc = FALSE;
  }
  if (data.rc == MD5_FALSE_FULL) {
    logMain(LOG_WARNING, "wrong full hash on \"%s\" support", supp->name);
    rc = FALSE;
  }

  if (!rc) {
    logMain(LOG_WARNING, "please manualy check \"%s\" support", supp->name);
    logMain(LOG_WARNING, "either this is not \"%s\" support at %s", 
	    supp->name, env.noRegression?"xxx":path);
    logMain(LOG_WARNING, "or maybe the \"%s\" support is obsolete", 
	    supp->name);
    goto error;
  }

  // store results
  switch (data.opp) {
  case  MD5_SUPP_ADD:
    supp->size = data.size;
    strncpy(supp->quickHash, data.quickMd5sum, MAX_SIZE_HASH);
    strncpy(supp->fullHash, data.fullMd5sum, MAX_SIZE_HASH);
  case MD5_SUPP_CHECK:
    if (env.noRegression)
      supp->lastCheck = currentTime() + 1*DAY;
    else
      supp->lastCheck = now;
  case MD5_SUPP_ID:
    if (env.noRegression)
      supp->lastSeen = currentTime() + 1*DAY;
    else
      supp->lastSeen = now;
  default:
    break;
  }

 error:
  free(data.path);
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

  logMain(LOG_DEBUG, "%s", "mdtxLsSupport");
  if (!(conf = getConfiguration())) goto error;
  if (!loadConfiguration(CFG | SUPP)) goto error;
  supports = conf->supports;

  rgRewind(supports);
  while((supp = rgNext(supports)) != 0) {
    if (!scoreSupport(supp, &conf->scoreParam)) goto error;
  }
  
  printf("%5s %*s %s\n", 
	 "score", MAX_SIZE_STAT, "state", "label");
  while((supp = rgNext(supports)) != 0) {
    printf("%5.2f %*s %s\n", 
	   supp->score, MAX_SIZE_STAT, supp->status, supp->name);
  }
  printf("\n");

  rc = TRUE;
 error:
 if (!rc) {
   logMain(LOG_ERR, "%s", "mdtxLsSupport fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : addSupport 
 * Description: add a new available support
 * Synopsis   : int addSupport(char* label, char* path)
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

  logMain(LOG_DEBUG, "%s", "mdtxAddSupport");
  if (isEmptyString(path)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if ((now = currentTime()) == -1) goto error;

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

  conf->fileState[iSUPP] = MODIFIED;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "error while adding support %s from %s", 
	    label, env.noRegression?"xxx":path);
    if (supp) delSupport(supp);
  }
  return rc;
}

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : Collection* coll
 *              Archive* iso
 *              char* path
 * Output     : RecordTree* tree
 *              TRUE on success
 =======================================================================*/
static int 
addFinalSupplies(Collection* coll, Support* supp, char* path, char* mnt,
		 RecordTree* tree) 
{
  int rc = FALSE;
  Archive* archive = 0;
  Record* record = 0;
  FromAsso* asso = 0;
  AVLNode *node = 0;
  char* extra = 0;

  logMain(LOG_DEBUG, "addFinalSupplies: %s:%lli",
	  supp->fullHash, (long long int)supp->size);

  if (!loadCollection(coll, EXTR)) goto error;
  if (!getLocalHost(coll)) goto error2;
  if (!(archive = getArchive(coll, supp->fullHash, supp->size))) {
    logMain(LOG_ERR, "%s", "archive is not defined into the extract tree");
    goto error;
  }

  // add the iso archive
  if (!(extra = createString(path))) goto error2;
  if (!(record = addRecord(coll, coll->localhost, archive, SUPPLY, extra)))
    goto error2;
  if (!rgInsert(tree->records, record)) goto error2;

  // add available content once the iso is mounted
  // (server do not mount it so do not use the extraction rules)
  if (!mnt) goto end;
  for(node = archive->toContainer->childs->head; 
      node; node = node->next) {
    asso = node->item;

    if (!(extra = createString(mnt))
	|| !(extra = catString(extra, asso->path)))
      goto error2;
    if (!(record = 
	  addRecord(coll, coll->localhost, 
		    asso->archive, SUPPLY, extra)))
      goto error2;
    if (!rgInsert(tree->records, record)) goto error2;
  }

 end:
  rc = TRUE;
 error2:
  if (!releaseCollection(coll, EXTR)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", "addFinalSupplies fails");
  }
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
notifyHave(Support* supp, char* path, char* mnt) 
{
  int rc = FALSE;
  int socket = 0;
  Configuration* conf = 0;
  Collection* coll = 0;
  RecordTree* tree = 0;
  Record* record = 0;
  RGIT* curr1 = 0;
  RGIT* curr2 = 0;
  char* name = 0;
  int isShared = FALSE;
  char reply[1];

  logMain(LOG_DEBUG, "%s", "notifyHave");
  if (supp == 0) goto error;
  if (isEmptyString(path) || *path != '/') {
    logMain(LOG_ERR, "%s", 
	    "daemon need an absolute path for support extraction");
    goto error;
  }

  conf = getConfiguration();
  if (!loadConfiguration(CFG)) goto error;
  if (!expandConfiguration()) goto error;
  if (!(tree = createRecordTree())) goto error;    
  tree->messageType = HAVE;

  // for each collections
  while((coll = rgNext_r(conf->collections, &curr1)) != 0) {
    tree->collection = coll;

    // find the support if used by this collection
    while((name = rgNext_r(coll->supports, &curr2)) != 0) {
      if (!strncmp(name, supp->name, MAX_SIZE_NAME)) break;
    }
    if (name == 0) continue;
    isShared = TRUE;

    // add final supplies    
    if (!addFinalSupplies(coll, supp, path, mnt, tree)) goto error;

    if (!env.noRegression) {
      // trick to use 127.0.0.1 instead of www IP address
      buildSocketAddressEasy(&coll->localhost->address, 
			     0x7f000001, coll->localhost->mdtxPort);
      
      if ((socket = connectServer(coll->localhost)) == -1) goto error;
      if (!upgradeServer(socket, tree, 0)) goto error;

      // do not use 127.0.0.1 anymore
      coll->localhost->address.sin_family = 0;

      // wait until server shut down the sockect
      tcpRead(socket, reply, 1);
    }
    else {
      fprintf(stderr, "%s", "\n");
      logMain(LOG_INFO, "notify support to %s collection", 
	      tree->collection->label);
    }
    
    // del final supplies
    while((record = rgHead(tree->records)) != 0) {
      if (!delRecord(coll, record)) goto error;
      rgRemove(tree->records);
    }
  }

  if (!isShared) {
    logMain(LOG_DEBUG, "the %s support is not share by any collection",
  	    supp->name);
  }
  rc = TRUE;
 error:
  if (!rc) {
  logMain(LOG_ERR, "fails to launch extraction on the %s support", 
	  supp?supp->name:"unknown");
  }
  if (socket) close(socket);
  tree = destroyRecordTree(tree);
  return rc;
}

/*=======================================================================
 * Function   : isIso 
 * Description: check if a file is an iso
 * Synopsis   : int isIsoFile(char* path)
 * Input      : char* path
 * Output     : TRUE on success
 =======================================================================*/
int isIsoFile(char* path)
{
  int rc = FALSE;
  int fd = -1;
  unsigned long int count = 0;
  unsigned short int bs = 0;
  off_t size = 0;

  logMain(LOG_DEBUG, "%s", "isIsoFile");

  if ((fd = open(path, O_RDONLY)) == -1) {
    logMain(LOG_ERR, "open: %s", strerror(errno));
    goto error;
  }
  
  if (!getIsoSize(fd, &size, &count, &bs)) goto error;
  rc = (size > 0);
 error:
  if (fd != -1 && close(fd) == -1) {
    logMain(LOG_ERR, "close: %s", strerror(errno));
    rc = FALSE;
  }
 if (!rc) {
    logMain(LOG_INFO, "%s", "not an iso");
  }
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
  char* mnt = 0;
  char pid[12];

  logMain(LOG_DEBUG, "%s", "mdtxHaveSupport");
  if (!(conf = getConfiguration())) goto error;
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
 
  // mount the iso if we have one
  if (isIsoFile(path)) {
    sprintf(pid, "/%i/", getpid());
    if (!(mnt = createString(conf->extractDir))
	|| !(mnt = catString(mnt, pid)))
      goto error;
    if (!mdtxMount(path, mnt)) goto error;
  }
  
  // ask the daemon to start extraction on the support
  if (!(absPath = absolutePath(path))) goto error2;
  if (!notifyHave(supp, absPath, mnt)) goto error2;

 end:
  // update support's dates
  conf->fileState[iSUPP] = MODIFIED;
  rc = TRUE;
 error2:
  // unmount the iso
  if (!isEmptyString(mnt) && !mdtxUmount(mnt)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "have query on %s support failed", 
	    supp?supp->name:"unknown");
  }
  absPath = destroyString(absPath);
  mnt = destroyString(mnt);
  return rc;
}

/*=======================================================================
 * Function   : removeSupport 
 * Description: remove a support
 * Synopsis   : int removeSupport(char* label)
 * Input      : char* label = the support to remove
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxDelSupport(char* label)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Support *supp = 0;

  logMain(LOG_DEBUG, "%s", "mdtxDelSupport");

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
