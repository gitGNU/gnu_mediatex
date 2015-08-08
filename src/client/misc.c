/*=======================================================================
 * Version: $Id: misc.c,v 1.9 2015/08/08 23:31:40 nroche Exp $
 * Project: MediaTeX
 * Module : misc
 *
 * Miscelanous client queries

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
 * Function   : mdtxMake
 * Author     : Nicolas ROCHE
 * modif      : 2013/10/23
 * Description: Serialize collections into LaTex files
 * Synopsis   : int mdtxMake(char* label)
 * Input      : char* label = collection to pre-process
 * Output     : TRUE on success
 =======================================================================*/
int mdtxMake(char* label)
{
  int rc = FALSE;
  Collection* coll = 0;
  int uid = getuid();
  int n = 0;

  checkLabel(label);
  if (!(coll = mdtxGetCollection(label))) goto error;
  if (!loadCollection(coll, SERV|CTLG|EXTR)) goto error;
  if (!computeExtractScore(coll)) goto error2;

  logMain(LOG_INFO, "Make HTML for %s", label);

  // roughtly estimate the number of steps
  env.progBar.cur = 0;
  env.progBar.max = 0;
  n = avl_count(coll->archives);
  env.progBar.max += n + 2*n / MAX_INDEX_PER_PAGE;
  logMain(LOG_INFO, "estimate %i steps for archives", env.progBar.max);
  n = avl_count(coll->catalogTree->documents);
  env.progBar.max += n + 2*n / MAX_INDEX_PER_PAGE;
  n = avl_count(coll->catalogTree->humans);
  env.progBar.max += n + 2*n / MAX_INDEX_PER_PAGE;
  logMain(LOG_INFO, "estimate %i steps for all", env.progBar.max);

  logMain(LOG_INFO, "html");
  if (!becomeUser(env.confLabel, TRUE)) goto error2;
  startProgBar("make");
  if (!serializeHtmlCache(coll)) goto error3;
  if (!serializeHtmlIndex(coll)) goto error3;
  if (!serializeHtmlScore(coll)) goto error3;
  stopProgBar();
  logMain(LOG_INFO, "steps: %lli / %lli", env.progBar.cur, env.progBar.max);
  logMain(LOG_INFO, "ending");

  rc = TRUE;
 error3:
  if (!logoutUser(uid)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, SERV|CTLG|EXTR)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxMake fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxUpgradePlus
 * Author     : Nicolas ROCHE
 * modif      : 2013/10/23
 * Description: Upgrade and make (order as no inpact)
 * Synopsis   : int mdtxUpgradePlus(char* label)
 * Input      : char* label = collection to pre-process
 * Output     : TRUE on success
 =======================================================================*/
int mdtxUpgradePlus(char* label)
{
  int rc = FALSE;

  checkLabel(label);
  if (!mdtxUpgrade(label)) goto error;
  if (!mdtxMake(label)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUpgradePlus fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxUploadPlus
 * Author     : Nicolas ROCHE
 * modif      : 2013/10/23
 * Description: Upload + upgrade
 * Synopsis   : int mdtxUploadPlus(char* label, char* path)
 * Input      : char* label = collection to pre-process
 *              char* path = path to the file to upload
 * Output     : TRUE on success
 =======================================================================*/
int mdtxUploadPlus(char* label, char* path)
{
  int rc = FALSE;

  checkLabel(label);
  if (!mdtxUploadFile(label, path)) goto error;
  if (!mdtxUpgrade(label)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUploadPlus fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : mdtxUploadPlusPlus
 * Author     : Nicolas ROCHE
 * modif      : 2013/10/23
 * Description: Upload + upgrade + make
 * Synopsis   : int mdtxUploadPlusPlus(char* label, char* path)
 * Input      : char* label = collection to pre-process
 *              char* path = path to the file to upload
 * Output     : TRUE on success
 =======================================================================*/
int mdtxUploadPlusPlus(char* label, char* path)
{
  int rc = FALSE;

  checkLabel(label);
  if (!mdtxUploadPlus(label, path)) goto error;
  if (!mdtxMake(label)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUploadPlusPlus fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : cleanCollection
 * Description: del a collection to a configuration
 * Synopsis   : clean the HTML catalog
 * Input      : char* label = the collection to delete
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxClean(char* label)
{ 
  int rc = FALSE;
  Configuration* conf = 0;
  //Collection* coll = 0;
  char* argv[3] = {0, 0, 0};

  logMain(LOG_DEBUG, "del a collection");

  if (!allowedUser(env.confLabel)) goto error;
  if (!(conf = getConfiguration())) goto error;

  argv[0] = createString(conf->scriptsDir);
  argv[0] = catString(argv[0], "/clean.sh");
  argv[1] = label;

  if (!env.noRegression) {
    if (!execScript(argv, "root", 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to clean collection");
  }
  if (argv[0]) free(argv[0]);
  return rc;
}

/*=======================================================================
 * Function   : mdtxInit
 * Description: call init.sh
 * Synopsis   : int mdtxInit()
 * Input      : N/A
 * Output     : TRUE on success
 * Note       : must be call by root
 =======================================================================*/
int 
mdtxInit()
{ 
  int rc = FALSE;
  char *argv[] = {0, 0, 0};

  logMain(LOG_DEBUG, "initializing mdtx software");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir)))
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!(argv[0] = catString(argv[0], "/init.sh"))) goto error;
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "mtdx software initialization fails");
  }
  if (argv[0]) destroyString(argv[0]);
  return rc;
}

/*=======================================================================
 * Function   : mdtxRemove
 * Description: call remove.sh
 * Synopsis   : int mdtxRemove()
 * Input      : N/A
 * Output     : TRUE on success
 * Note       : must be call by root
 =======================================================================*/
int 
mdtxRemove()
{ 
  int rc = FALSE;
  char *argv[] = {0, 0, 0};

  logMain(LOG_DEBUG, "removing mdtx software");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir)))
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!(argv[0] = catString(argv[0], "/remove.sh"))) goto error;
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "mtdx software removal fails");
  }
  if (argv[0]) destroyString(argv[0]);
  return rc;
}

/*=======================================================================
 * Function   : mdtxPurge
 * Description: call purge.sh
 * Synopsis   : int mdtxPurge()
 * Input      : N/A
 * Output     : TRUE on success
 * Note       : must be call by root
 =======================================================================*/
int 
mdtxPurge()
{ 
  int rc = FALSE;
  char *argv[] = {0, 0, 0};

  logMain(LOG_DEBUG, "purging mdtx software");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir)))
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!(argv[0] = catString(argv[0], "/purge.sh"))) goto error;
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "mdtx software purge fails");
  }
  if (argv[0]) destroyString(argv[0]);
  return rc;
}

/*=======================================================================
 * Function   : mdtxAddUser
 * Description: call addUser.sh
 * Synopsis   : int mdtxAddUser(char* user)
 * Input      : char* user: the user to give mdtx permissions
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxAddUser(char* user)
{ 
  int rc = FALSE;  
  Configuration* conf = 0;
  char *argv[] = {0, 0, 0};

  checkLabel(user);
  logMain(LOG_DEBUG, "mdtxAddUser %s:", user);

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/addUser.sh"))) goto error;
  argv[1] = user;
  
  if (!env.noRegression) {
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "mdtxAddUser fails");
  }
  argv[0] = destroyString(argv[0]);
  return(rc);
}

/*=======================================================================
 * Function   : mdtxDelUser
 * Description: call delUser.sh
 * Synopsis   : int mdtxDelUser(char* user)
 * Input      : char* user: the user to give mdtx permissions
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxDelUser(char* user)
{ 
  int rc = FALSE;  
  Configuration* conf = 0;
  char *argv[] = {0, 0, 0};

  checkLabel(user);
  logMain(LOG_DEBUG, "mdtxDelUser %s:", user);

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/delUser.sh"))) goto error;
  argv[1] = user;
  
  if (!env.noRegression) {
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "mdtxDelUser fails");
  }
  argv[0] = destroyString(argv[0]);
  return(rc);
}

/*=======================================================================
 * Function   : mdtxBind
 * Description: call bind.sh
 * Synopsis   : int mdtxBind()
 * Input      : N/A
 * Output     : TRUE on success
 * Note       : must be call by root
 =======================================================================*/
int 
mdtxBind()
{ 
  int rc = FALSE;
  char *argv[] = {0, 0};

  logMain(LOG_DEBUG, "bind mdtx directories");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir))
      || !(argv[0] = catString(argv[0], "/bind.sh"))) 
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, "root", 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "mdtx bind fails");
  }
  if (argv[0]) destroyString(argv[0]);
  return rc;
}

/*=======================================================================
 * Function   : mdtxUnbind
 * Description: call unbind.sh
 * Synopsis   : int mdtxUnind()
 * Input      : N/A
 * Output     : TRUE on success
 * Note       : must be call by root
 =======================================================================*/
int 
mdtxUnbind()
{ 
  int rc = FALSE;
  char *argv[] = {0, 0};

  logMain(LOG_DEBUG, "unbind mdtx directories");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir))
      || !(argv[0] = catString(argv[0], "/unbind.sh"))) 
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, "root", 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "mdtx unbind fails");
  }
  if (argv[0]) destroyString(argv[0]);
  return rc;
}

/*=======================================================================
 * Function   : mdtxSu
 * Description: call bin/bash
 * Synopsis   : int mdtxSu(char* label)
 * Input      : char* label: the collection user to become
 *                                or mdtx user if 0
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxSu(char* label)
{ 
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  char* argv[] = {"/bin/bash", 0, 0};
  char* home = 0;
  char* user = 0;

  logMain(LOG_DEBUG, "mdtxSu %s", label?label:"");

  // set the HOME directory environment variable
  if (label == 0) {
    if (!(conf = getConfiguration())) goto error;
    home = conf->homeDir;
  }
  else {
    if (!(coll = mdtxGetCollection(label))) goto error;
    home = coll->homeDir;
  }
  
  // get the username
  if (coll == 0) {
    user = env.confLabel;
  }
  else {
    user = coll->user;
  }

  if (!env.noRegression && !env.dryRun) {
    if (setenv("HOME", home, 1) == -1) goto error;
    if (!allowedUser(user)) goto error;
    
    // do not close stdout !
    env.debugScript = TRUE;
    
    if (!execScript(argv, user, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "mdtx su has failed");
  }
  return(rc);
}

/*=======================================================================
 * Function   : mdtxScp
 * Description: call scp so as to not need setuid bit on daemon
 * Synopsis   : int mdtxScp(char* label, char* fingerPrint, char* target)
 * Input      : char* label : collection label
 *              char* fingerPrint : source server's fingerprint
 *              char* target : relative path to target in cache
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxScp(char* label, char* fingerPrint, char* target)
{
  int rc = FALSE;
  Collection* coll = 0;
  Server* server = 0;
  char* argv[] = {"/usr/bin/scp", 0, 0, 0};

  checkLabel(label);
  checkLabel(fingerPrint);
  checkLabel(target);
  logMain(LOG_DEBUG, "mdtxScp %s", target);

  if (!(coll = mdtxGetCollection(label))) goto error;
  if (!loadCollection(coll, SERV)) goto error;
  if (!(server = getServer(coll, fingerPrint))) goto error2;

  if (!(argv[1] = createString(server->user))
      || !(argv[1] = catString(argv[1], "@"))
      || !(argv[1] = catString(argv[1], server->host))
      || !(argv[1] = catString(argv[1], ":/var/cache/"))
      || !(argv[1] = catString(argv[1], server->user))
      || !(argv[1] = catString(argv[1], "/"))
      || !(argv[1] = catString(argv[1], target)))
    goto error2;

  if (!(argv[2] = createString(coll->extractDir))
      || !(argv[2] = catString(argv[2], "/")) 
      || !(argv[2] = catString(argv[2], target)))
    goto error2;

  if (!env.dryRun && !execScript(argv, coll->user, 0, FALSE)) 
    goto error2;
  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV)) goto error;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxScp fails");
  } 
  argv[1] = destroyString(argv[1]);
  argv[2] = destroyString(argv[2]);
  return rc;
}

/*=======================================================================
 * Function   : mdtxUploadFile 
 * Description: upload a file and add its extraction metadata
 * Synopsis   : int mdtxUploadFile(char* label, char* path)
 * Input      : char* label = related collection
 *            : char* path = path to the file to upload
 * Output     : TRUE on success
 =======================================================================*/
int
mdtxUploadFile(char* label, char* path)
{
  int rc = FALSE;
  int socket = -1;
  Collection* coll = 0;
  Container* container = 0;
  Archive* archive = 0;
  RecordTree* tree = 0;
  Record* record = 0;
  struct stat statBuffer;
  Md5Data md5; 
  char* extra = 0;
  char* message = 0;
  char reply[256];
  int status = 0;
  int n = 0;
  FromAsso* asso = 0;
  time_t time = 0;
  struct tm date;
  char dateString[32];

  if (isEmptyString(path)) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;
  if (!loadCollection(coll, EXTR)) goto error;

  /* if (!env.noRegression) { */
  /*   // check if daemon is awake */
  /*   if (access(conf->pidFile, R_OK) == -1) { */
  /*     logMain(LOG_INFO, "cannot read daemon's pid file: %s",  */
  /* 	    strerror(errno)); */
  /*     goto end; */
  /*   } */
  /* } */

  if (!(tree = createRecordTree())) goto error2; 
  tree->collection = coll;
  tree->messageType = UPLOAD; 
  if (!getLocalHost(coll)) goto error2;

  // get file attributes (size)
  if (stat(path, &statBuffer)) {
    logMain(LOG_ERR, "status error on %s: %s", path, strerror(errno));
    goto error2;
  }

  // compute hash
  memset(&md5, 0, sizeof(Md5Data));
  md5.path = path;
  md5.size = statBuffer.st_size;
  md5.opp = MD5_CACHE_ID;
  if (!doMd5sum(&md5)) goto error2;

  // archive to upload
  if (!(archive = addArchive(coll, md5.fullMd5sum, statBuffer.st_size)))
    goto error2;

  /* // add the file to the UPLOAD message */
  if (!(extra = absolutePath(path))) goto error2;
  if (!(record = addRecord(coll, coll->localhost, archive, SUPPLY, extra)))
    goto error2;
  extra = 0;
  if (!rgInsert(tree->records, record)) goto error2;
  record = 0;
  
  if (env.noRegression) {
    logMain(LOG_INFO, "upload file to %s collection", label);
  }
  else {
    if ((socket = connectServer(coll->localhost)) == -1) goto error2;
    if (!upgradeServer(socket, tree, 0)) goto error2;
    
    // read reply
    n = tcpRead(socket, reply, 255);
    tcpRead(socket, reply, 1);
    if (sscanf(reply, "%i %s", &status, message) < 1) {
      reply[(n<=0)?0:n-1] = (char)0; // remove ending \n
      logMain(LOG_ERR, "error reading reply: %s", reply);
      goto error2;
    }
    
    logMain(LOG_INFO, "local server tels (%i) %s", status, message);
    if (status != 200) goto error2;
  }

  // add extraction rule
  if (!(time = currentTime())) goto error;
  if (localtime_r(&time, &date) == (struct tm*)0) {
    logMemory(LOG_ERR, "localtime_r returns on error");
    goto error2;
  }
  sprintf(dateString, "%04i-%02i-%02i,%02i:%02i:%02i", 
	  date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	  date.tm_hour, date.tm_min, date.tm_sec);
  if (!(container = coll->extractTree->incoming)) goto error2;
  if (!(asso = addFromAsso(coll, archive, container, dateString))) 
    goto error2;
  if (!wasModifiedCollection(coll, EXTR)) goto error2;

  rc = TRUE;
 error2:
  extra = destroyString(extra);
  if (record) delRecord(coll, record);
  tree = destroyRecordTree(tree);
  if (socket) close(socket);
  if (!releaseCollection(coll, EXTR)) goto error;
 error:
  if (!rc) {
    logMain(LOG_ERR, "upload query failed");
    if (record) delRecord(coll, record);
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
