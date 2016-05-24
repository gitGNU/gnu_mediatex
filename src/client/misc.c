/*=======================================================================
 * Project: MediaTeX
 * Module : misc
 *
 * Miscelanous client queries

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
 *              char* catalog: catalog metadata file to upload
 *              char* extract: extract metadata file to upload
 *              char* file: data to upload
 *              char* targetPath: where to copy data into the cache
 * Output     : TRUE on success
 =======================================================================*/
int mdtxUploadPlus(char* label, char* catalog, char* extract, 
		   char* file, char* targetPath)
{
  int rc = FALSE;

  checkLabel(label);
  if (!mdtxUpload(label, catalog, extract, file, targetPath)) 
    goto error;

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
 *              char* catalog: catalog metadata file to upload
 *              char* extract: extract metadata file to upload
 *              char* file: data to upload
 *              char* targetPath: where to copy data into the cache
 * Output     : TRUE on success
 =======================================================================*/
int mdtxUploadPlusPlus(char* label, char* catalog, char* extract, 
		   char* file, char* targetPath)
{
  int rc = FALSE;

  checkLabel(label);
  if (!mdtxUploadPlus(label, catalog, extract, file, targetPath))
    goto error;

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
  LogSeverity* tmpSev =  env.logHandler->severity[LOG_SCRIPT];
  
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
    
    // do not close stdout! else we loose id and groups.
    env.logHandler->severity[LOG_SCRIPT] = LogSeverities+7;
    
    if (!execScript(argv, user, 0, FALSE)) goto error;
  }

  env.logHandler->severity[LOG_SCRIPT] = tmpSev;
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
 * Input      : char* label: collection label
 *              char* fingerPrint: source server's fingerprint
 *              char* source: relative path to remote source
 *              char* target: relative path to local destination
 * Output     : TRUE on success
 * Note       : scp will fails it jail/etc/passwd do not provides
 *              collection user.
 =======================================================================*/
int 
mdtxScp(char* label, char* fingerPrint, char* source, char* target)
{
  int rc = FALSE;
  Collection* coll = 0;
  Server* server = 0;
  char* argv[] = {"/usr/bin/scp", 0, 0, 0};

  checkLabel(label);
  checkLabel(fingerPrint);
  checkLabel(source);
  checkLabel(target);
  logMain(LOG_DEBUG, "mdtxScp %s from %s on %s",
	  source, fingerPrint, target);

  if (!(coll = mdtxGetCollection(label))) goto error;
  if (!loadCollection(coll, SERV)) goto error;
  if (!(server = getServer(coll, fingerPrint))) goto error2;

  if (!(argv[1] = createString(server->user))
      || !(argv[1] = catString(argv[1], "@"))
      || !(argv[1] = catString(argv[1], server->host))
      || !(argv[1] = catString(argv[1], ":/var/cache/"))
      || !(argv[1] = catString(argv[1], server->user))
      || !(argv[1] = catString(argv[1], "/"))
      || !(argv[1] = catString(argv[1], source)))
    goto error2;

  argv[2] = target;

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
  return rc;
}

/*=======================================================================
 * Function   : mdtxAudit
 * Description: simulate extraction on all archive of a collection
 * Synopsis   : int mdtxAudit(char* label, char* mail)
 * Input      : char* label: collection label
 *              char* mail: mail to use to return the audit
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxAudit(char* label, char* mail)
{ 
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RecordTree* tree = 0;
  Archive* archive = 0;
  Record* record = 0;
  AVLNode *node = 0;
  FILE* fd = stdout; 
  char path[MAX_SIZE_STRING];
  char extra[MAX_SIZE_STRING];
  char* ptrMail = 0;
  char* tmp = 0;
  time_t now = 0;
  struct tm date;
  int socket = -1;
  int n = 0;
  char reply[255] = "100 nobody";
  int status = 0;
  int uid = getuid();

  logMain(LOG_DEBUG, "audit %s collection for %s", label, mail);
  checkLabel(label);
  checkLabel(mail);

  if (!(conf = getConfiguration())) goto error;
  /* if (!env.noRegression) { */
  /*   // check if daemon is awake */
  /*   if (access(conf->pidFile, R_OK) == -1) { */
  /*     logMain(LOG_INFO, "cannot read daemon's pid file: %s",  */
  /* 	      strerror(errno)); */
  /*     goto end; */
  /*   } */
  /* } */

  if (!becomeUser(env.confLabel, TRUE)) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error2;
  if (!(tree = createRecordTree())) goto error2;
  if (!getLocalHost(coll)) goto error2;
  tree->collection = coll;
  tree->messageType = CGI;

  // open audit file
  if ((now = currentTime()) == -1) goto error2;
  if (localtime_r(&now, &date) == (struct tm*)0) {
    logMemory(LOG_ERR, "localtime_r returns on error");
    goto error2;
  }
 
  sprintf(path, "%s/%s%04i%02i%02i-%02i%02i%02i_%s.txt",
	  coll->extractDir, CONF_AUDIT,
	  date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	  date.tm_hour, date.tm_min, date.tm_sec,
	  coll->userFingerPrint);
  strcpy(extra, path + strlen(coll->extractDir) + 1);
  ptrMail = extra + strlen(extra) - 3;
  *(ptrMail - 1) = ':';

  if (!env.dryRun) {
    if ((fd = fopen(path, "w")) == 0) {
      logMemory(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
      fd = stdout;
      goto error2;
    }
    if (!lock(fileno(fd), F_WRLCK)) goto error2;
  }

  logMemory(LOG_INFO, "Serializing audit into: %s", 
	    env.dryRun?"stdout":path);

  if (!loadCollection(coll, EXTR)) goto error2;

  fprintf(fd, "Audit on %s collection\n", coll->label);
  fprintf(fd, " requested on %04i-%02i-%02i %02i:%02i:%02i\n",
	  date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	  date.tm_hour, date.tm_min, date.tm_sec); 
  fprintf(fd, " requested by %s\n", mail);
  fprintf(fd, " %i archives to check\n", avl_count(coll->archives));
  fprintf(fd, " %i archives checked:\n", 0);
  fprintf(fd, "  %i archives ok\n", 0);
  fprintf(fd, "  %i archives ko\n", 0);

  fprintf(fd, "\nArchive to check:\n");
  if (avl_count(coll->archives)) {
    for(node = coll->archives->head; node; node = node->next) {
      archive = (Archive*)node->item;
      fprintf(fd, " %s:%lli\n", archive->hash,
	      (long long int)archive->size);
      
      // add a final demand
      strcpy(ptrMail, mail);
      if (!(tmp = createString(extra))) goto error3; 
      if (!(record = newRecord(coll->localhost, archive, DEMAND, tmp))) 
	goto error3;
      tmp = 0;
      if (!rgInsert(tree->records, record)) goto error3;
      record = 0;
    }
  }
  fflush(fd);

  // connect server and write query
  if (env.noRegression) goto end;
  if ((socket = connectServer(coll->localhost)) == -1) goto error3;
  if (!upgradeServer(socket, tree, 0)) goto error3;
  
  // read reply
  if (env.dryRun) goto end;
  n = tcpRead(socket, reply, 255);
  // erase the \n send by server
  if (n<=0) n = 1;
  reply[n-1] = (char)0; 
  if (sscanf(reply, "%i", &status) < 1) {
    logMain(LOG_ERR, "error reading server reply: %s", reply);
    goto error;
  }
  
  logMain(LOG_INFO, "receive: %s", reply);
  logMain(LOG_NOTICE, "please check motd and performe extraction", reply);
 end:
  rc = (env.noRegression || env.dryRun || status == 221);
 error3:
  if (!releaseCollection(coll, EXTR)) rc = FALSE;
 error2:
  if (!logoutUser(uid)) rc = FALSE;
 error:
  if (fd != stdout) {
    if (!unLock(fileno(fd))) rc = FALSE;
    if (fclose(fd)) {
      logMemory(LOG_ERR, "fclose fails: %s", strerror(errno));
      rc = FALSE;
    }
  }
  if (!rc) {
    logMain(LOG_ERR, "mdtx audit fails");
  }
  destroyString(tmp);
  destroyRecord(record);
  destroyRecordTree(tree);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
