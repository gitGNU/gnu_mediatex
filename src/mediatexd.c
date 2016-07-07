/*=======================================================================
 * Project: MediaTeX
 * Module : server software
 *
 * Server software's main function

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

/*
  socket return codes:
  
  1--: negative reply
  2--: ok
  3--: bad request
  4--: internal error
*/

#include "mediatex-config.h"
#include "server/mediatex-server.h"

#define OPEN_MAX 4096

extern int taskSocketNumber;
extern int taskSignalNumber;


/*=======================================================================
 * Function   : signalJob
 * Description: Thread callback function for signals
 * Synopsis   : void* signalJob(void* arg)
 * Input      : void* arg = not used
 * Output     : (void*)TRUE on success
 =======================================================================*/
void* 
signalJob(void* arg)
{
  int me = taskSignalNumber;
  Configuration* conf = 0;
  long int rc = FALSE;
  int rc2 = REG_DONE;
  int i = 0;
  int j = 0;
  char mask[REG_SHM_BUFF_SIZE];
  ShmParam param;
  
  struct job {
    int reg;
    int (*function)(Collection*);
    char *name; // only for logs
  };

  static struct job jobs[9] = {
    {REG_SAVEMD5, saveCache, "SAVEMD5"},
    {REG_EXTRACT, extractArchives, "EXTRACT"},
    {REG_NOTIFY, sendRemoteNotify, "NOTIFY"},
    {REG_PURGE, purgeCache, "PURGE"},
    {REG_CLEAN, cleanCache, "CLEAN"},
    {REG_TRIM, trimCache, "TRIM"},
    {REG_SCAN, scanCache, "SCAN"},
    {REG_QUICKSCAN, quickScanCache, "QUICK SCAN"},
    {REG_STATUS, statusCache, "STATUS"}
  };

  (void) arg;
  logMain(LOG_DEBUG, "signalJob: %i", me);
  if (!(conf = getConfiguration())) goto error;
  memset(mask, 0, REG_SHM_BUFF_SIZE);

  if (!shmRead(conf->confFile, REG_SHM_BUFF_SIZE,
	       mdtxShmRead, (void*)&param))
    goto error;
   
  // loop on jobs (as several jobs may be wanted here)
  for (i=0; i<9 ; ++i) {
    if (param.buf[jobs[i].reg] != REG_QUERY) continue;
  
    logMain(LOG_NOTICE, "signalJob %i: %s", me, jobs[i].name);
    mask[jobs[i].reg] = 1;
      
    // "purge" do more than "clean", and so...
    switch (jobs[i].reg) {
    case REG_PURGE:
      mask[REG_CLEAN] = 1;
    case REG_CLEAN:
      mask[REG_TRIM] = 1;
    case REG_TRIM:
    case REG_SCAN:
      mask[REG_QUICKSCAN] = 1;
    case REG_QUICKSCAN:
      mask[REG_STATUS] = 1;
    }
      
    // mark job as pending
    for (j=0; j<REG_SHM_BUFF_SIZE; ++j) {
      if (mask[j]) param.buf[j] = REG_PENDING;
    }
    if (!shmWrite(conf->confFile, REG_SHM_BUFF_SIZE,
		  mdtxShmCopy, (void*)&param)) goto error;
      
    // do the jobs
    if (jobs[i].reg == REG_STATUS) {
      memoryStatus(LOG_NOTICE, __FILE__, __LINE__);
    }
    if (!serverLoop(jobs[i].function)) rc2 = REG_ERROR;
      
    // mark job as done
    for (j=0; j<REG_SHM_BUFF_SIZE; ++j) {
      if (mask[j]) param.buf[j] = rc2;
    }
    if (!shmWrite(conf->confFile, REG_SHM_BUFF_SIZE,
		  mdtxShmCopy, (void*)&param)) goto error;
  }

  rc = TRUE;
 error:
  if (rc) {
    if (rc2 == REG_DONE) {
      logMain(LOG_NOTICE, "signalJob %i: success", me);
    } else {
      logMain(LOG_ERR, "signalJob %i: fails", me);
    }
  } else {
    logMain(LOG_ERR, "signalJob %i: internal fails", me);
  }
  signalJobEnds();
  return (void*)rc;
}


/*=======================================================================
 * Function   : hupManager
 * Description: Reload configuration when receving an HUP signal
 * Synopsis   : void hupManager()
 * Input      : N/A
 * Output     : N/A
 * Note       : not rentrant and not designed to support concurrency
 =======================================================================*/
int
hupManager()
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;

  if (!serverSaveAll()) goto error;
  freeConfiguration();
  if (!loadConfiguration(CFG)) goto error;
  if (!expandConfiguration()) goto error;
  
  // for all collection
  if (!(conf = getConfiguration())) goto error;
  if (conf->collections) {
    while ((coll = rgNext_r(conf->collections, &curr))) {
      if (!loadCache(coll)) goto error;
    }
  }

  // where to call it ? no use here because of freeConfiguration()
  //if (!cleanCacheTree(coll)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "daemon HUP fails: exiting");
  }
  else {
    logMain(LOG_NOTICE, "daemon is HUP");
  }
  return rc;
}


/*=======================================================================
 * Function   : termManager
 * Description: Exit when receiving TERM signal
 * Synopsis   : void termManager()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int
termManager()
{
  int rc = FALSE;

  if (!serverSaveAll()) {
    logMain(LOG_ERR, "Fails to save md5sums while exiting");
    goto error;
  }

  logMain(LOG_NOTICE, "mdtx-cache-daemon exiting");
  rc = TRUE;
 error:
  return rc;
}


/*=======================================================================
 * Function   : socketJob
 * Description: thread callback function
 * Synopsis   : void* socketJob(void* arg)
 * Input      : void* arg = (Connexion*) connection structure
 * Output     : N/A
 =======================================================================*/
void* 
socketJob(void* arg)
{
  int me = taskSocketNumber;
  long int rc = FALSE;
  Connexion* con = (Connexion*)arg;

  static char status[][32] = {
    "301 message parser error",
    "305 unknown message type: %s",
    "400 internal error",
  };

  logMain(LOG_DEBUG, "socketJob %i", me);
  sprintf(con->status, "%s", status[1]);

  // read the socket
  if ((con->message = parseRecords(con->sock)) == 0) {
    sprintf(con->status, "%s", status[0]);
    goto error;
  }

  // common checks on message
  if (!checkMessage(con)) goto error;

  logMain(LOG_INFO, "%s: %s message from %s server (%s)",
	  con->message->collection->label, 
	  strMessageType(con->message->messageType),
	  con->server->fingerPrint, con->host);

  switch (con->message->messageType) {

  case UPLOAD:
    logMain(LOG_NOTICE, "socketJob %i: UPLOAD", me);
    con->status[1] = '1';
    if (!uploadFinaleArchive(con)) goto error;
    break;

  case CGI:
    logMain(LOG_NOTICE, "socketJob %i: CGI", me);
    con->status[1] = '2';
    if (!cgiServer(con)) goto error;
    break;
    
  case HAVE:
    logMain(LOG_NOTICE, "socketJob %i: HAVE", me);
    con->status[1] = '3';
    if (!extractFinaleArchives(con)) goto error;
    break;
    
  case NOTIFY:
    logMain(LOG_NOTICE, "socketJob %i: NOTIFY (from %s)", me, con->host);
    con->status[1] = '4';
    if (!acceptRemoteNotify(con)) goto error ;
    break;
    
  default:
    sprintf(con->status, status[1], 
	    strMessageType(con->message->messageType));
    goto error;
  }
  
  logMain(LOG_NOTICE, "%s", con->status);
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "%s", con->status);
    logMain(LOG_NOTICE, "still alive");
  }

  // send-back status code into the socket
  strcpy(con->status + strlen(con->status), "\n");
  tcpWrite(con->sock, con->status, strlen(con->status));
  con->message = destroyRecordTree(con->message);

  socketJobEnds(con);
  return (void*)rc;
}

/*=======================================================================
 * Function   : cleanTmpDirOnExit
 * Description: call rm -fr ~mdtx/tmp/'*'
 * Synopsis   : cleanTmpDirOnExit
 * Input      : char* source: source path
 *              char* target: destination path
 * Output     : TRUE on success
 * Note       : we need to load bash in order to use the '*'
 =======================================================================*/
int 
cleanTmpDirOnExit()
{
  int rc = FALSE;
  char* argv[] = {"/bin/bash", "-c", 0, 0};
  char *cmd = 0;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "cleanTmpDirOnExit");

  if (!loadConfiguration(CFG)) goto error;
  if (!(conf = getConfiguration())) goto error;

  // for all collection
  if (conf->collections) {
    while ((coll = rgNext_r(conf->collections, &curr))) {
      if (isEmptyString(coll->extractDir)) goto error;

      if (!(cmd = createString("/bin/rm -fr "))) goto error;
      if (!(cmd = catString(cmd, coll->extractDir))) goto error;
      if (!(cmd = catString(cmd, "/*"))) goto error;
      argv[2] = cmd;

      logMain(LOG_INFO, "%s %s %s", argv[0], argv[1], argv[2]);
      if (!env.dryRun && !execScript(argv, 0, 0, FALSE)) goto error;

      cmd = destroyString(cmd);
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "cleanTmpDirOnExit fails");
  }
  cmd = destroyString(cmd);
  return rc;
}

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
  fprintf(stderr,
	  "`" PACKAGE_NAME "' "
	  "is an Electronic Record Management System (ERMS), "
	  "focusing on the archival storage entity define by the `OAIS' "
	  "draft and on the `NF Z 42-013' requirements.\n");

  mdtxUsage(programName);
  fprintf(stderr, " [ -b ]");
  
  mdtxOptions();
  fprintf(stderr, "  -b, --background\trun background as a daemon\n");

  mdtxHelp();
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/11/04
 * Description: MediaTeX daemon
 * Synopsis   : ./mdtxd
 * Input      : sockets, shm and signals
 * Output     : rtfm
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = 0;
  struct passwd pw;
  char *buf1 = 0;
  FILE* fd = 0;
  int pid;
  int i;
  int isThere = FALSE;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS "b";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"background", no_argument, 0, 'b'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.background = FALSE;
  env.allocDiseaseCallBack = serverDiseaseAll;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {

    case 'b':
      env.background = TRUE;
      break;

      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  // if we are root, switch to mdtx user 
  if (getuid() == 0) {
    if (!becomeUser(env.confLabel, FALSE)) {
      goto error;
    }
  }
  
  // assert we are mdtx user 
  if (!getPasswdLine (0, getuid(), &pw, &buf1)) goto error;
  if (strcmp(env.confLabel, pw.pw_name)) {
    logMain(LOG_ERR,  "Please, lanch me as %s user (or root)",
	    env.confLabel);
    goto error;
  }

  // become a daemon
  if (env.background) {
    if (env.logFacility == getLogFacility("file")) {
      logMain(LOG_NOTICE, "switch logs to local2 facility");
    }
    logMain(LOG_INFO, "becoming a daemon...");
    if (chdir("/")) goto error;
    if (fork() != 0) {
      exit(EXIT_SUCCESS);
    }
    if (setsid() == -1) goto error;
    if (fork() != 0) {
      exit(EXIT_SUCCESS);
    }

    // close file descriptors and re-open the log handler
    env.logHandler = logClose(env.logHandler);
    for (i=0; i<OPEN_MAX; ++i) close(i);
    if (env.logFacility == getLogFacility("file")) {
      env.logFacility = getLogFacility("local2");
    }
    if (!setEnv(programName, &env)) goto error;    
    logMain(LOG_INFO, "...I'm a daemon");
  }
    
  // write daemon's pid file
  if (!(conf = getConfiguration())) goto error;
  if ((fd = fopen(conf->pidFile, "w")) == 0) {
    logMain(LOG_ERR, "fails to open %s: %s", 
	    conf->pidFile, strerror(errno));
    goto error;
  }
  pid = getpid();
  logMain(LOG_INFO, "write pid: %i", pid);
  if (fprintf(fd, "%i", pid) < 0) {
    logMain(LOG_ERR, "fails write pid to %s", conf->pidFile);
    goto error2;
  }
  if (fclose(fd)) {
    logMain(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error2;
  }
   
  // start mediatex stuffs
  logMain(LOG_INFO, "start mediatex stuffs");
  if (!hupManager()) goto error2;
  if (!mdtxCall(2, "adm", "bind")) goto error2;
  if (!mainLoop()) goto error3;  
  /************************************************************************/
  
  rc = cleanTmpDirOnExit();
 error3:
  if (!mdtxCall(2, "adm", "unbind")) rc = FALSE;
 error2:
  // delete pid file
  if (!(conf = getConfiguration())) goto error; // HUP may have free conf
  if (!callAccess(conf->pidFile, &isThere)) rc = FALSE;
  if (!isThere) {
    logMain(LOG_WARNING, "no pid file to remove. Expect: %s",
	    conf->pidFile);
  } else {
    if (unlink(conf->pidFile) == -1) {
      logMain(LOG_ERR, "fails to delete pid file: %s:", strerror(errno));
      rc = FALSE;
    }
  }
 error:
  free (buf1);
  freeConfiguration();
  ENDINGS;
  sleep(1);
  rc=!rc;
 optError:
  exit(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
