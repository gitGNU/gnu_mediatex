 /*=======================================================================
 * Version: $Id: misc.c,v 1.3 2015/06/03 14:03:29 nroche Exp $
 * Project: MediaTeX
 * Module : wrapper/make
 *
 * Front-end for the html sub modules

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

#include "../mediatex.h"
#include "../misc/log.h"
#include "../misc/command.h"
#include "../misc/device.h"
#include "../misc/progbar.h"
#include "../misc/md5sum.h"
#include "../misc/tcp.h"
#include "../common/connect.h"
#include "../common/openClose.h"
#include "../common/extractScore.h"
#include "commonHtml.h"
#include "catalogHtml.h"
#include "extractHtml.h"
#include "misc.h"

/*=======================================================================
 * Function   : mdtxMake
 * Author     : Nicolas ROCHE
 * modif      : 2013/10/23
 * Description: Serialize collections into LaTex files
 * Synopsis   : int mdtxPp(char* label)
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

  logEmit(LOG_INFO, "Make HTML for %s", label);

  // roughtly estimate the number of steps
  env.progBar.cur = 0;
  env.progBar.max = 0;
  n = avl_count(coll->archives);
  env.progBar.max += n + 2*n / MAX_INDEX_PER_PAGE;
  logEmit(LOG_INFO, "estimate %i steps for archives", env.progBar.max);
  n = avl_count(coll->catalogTree->documents);
  env.progBar.max += n + 2*n / MAX_INDEX_PER_PAGE;
  n = avl_count(coll->catalogTree->humans);
  env.progBar.max += n + 2*n / MAX_INDEX_PER_PAGE;
  logEmit(LOG_INFO, "estimate %i steps for all", env.progBar.max);

  logEmit(LOG_INFO, "%s", "html");
  if (!becomeUser(env.confLabel, TRUE)) goto error2;
  startProgBar("make");
  if (!serializeHtmlCache(coll)) goto error3;
  if (!serializeHtmlIndex(coll)) goto error3;
  if (!serializeHtmlScore(coll)) goto error3;
  stopProgBar();
  logEmit(LOG_INFO, "steps: %lli / %lli", env.progBar.cur, env.progBar.max);
  logEmit(LOG_INFO, "%s", "ending");

  rc = TRUE;
 error3:
  if (!logoutUser(uid)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, SERV|CTLG|EXTR)) rc = FALSE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "mdtxMake fails");
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

  logEmit(LOG_DEBUG, "%s", "del a collection");

  if (!allowedUser(env.confLabel)) goto error;
  if (!(conf = getConfiguration())) goto error;

  argv[0] = createString(conf->scriptsDir);
  argv[0] = catString(argv[0], "/clean.sh");
  argv[1] = label;
#ifndef utMAIN
  if (!execScript(argv, "root", 0, FALSE)) goto error;
#endif

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to clean collection");
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

  logEmit(LOG_DEBUG, "%s", "initializing mdtx software");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir)))
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!(argv[0] = catString(argv[0], "/init.sh"))) goto error;
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "mtdx software initialization fails");
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

  logEmit(LOG_DEBUG, "%s", "removing mdtx software");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir)))
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!(argv[0] = catString(argv[0], "/remove.sh"))) goto error;
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "mtdx software removal fails");
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

  logEmit(LOG_DEBUG, "%s", "purging mdtx software");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir)))
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!(argv[0] = catString(argv[0], "/purge.sh"))) goto error;
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "mdtx software purge fails");
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
  logEmit(LOG_DEBUG, "mdtxAddUser %s:", user);

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/addUser.sh"))) goto error;
  argv[1] = user;
  
#ifndef utMAIN
  if (!execScript(argv, 0, 0, FALSE)) goto error;
#endif

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "mdtxAddUser fails");
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
  logEmit(LOG_DEBUG, "mdtxDelUser %s:", user);

  if (!(conf = getConfiguration())) goto error;
  if (!(argv[0] = createString(conf->scriptsDir))) goto error;
  if (!(argv[0] = catString(argv[0], "/delUser.sh"))) goto error;
  argv[1] = user;
  
#ifndef utMAIN
  if (!execScript(argv, 0, 0, FALSE)) goto error;
#endif

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "mdtxDelUser fails");
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

  logEmit(LOG_DEBUG, "%s", "bind mdtx directories");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir))
      || !(argv[0] = catString(argv[0], "/bind.sh"))) 
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, "root", 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "mdtx bind fails");
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

  logEmit(LOG_DEBUG, "%s", "unbind mdtx directories");

  if (!(argv[0] = createString(getConfiguration()->scriptsDir))
      || !(argv[0] = catString(argv[0], "/unbind.sh"))) 
    goto error;

  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, "root", 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "mdtx unbind fails");
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

  logEmit(LOG_DEBUG, "mdtxSu %s", label?label:"");

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
    logEmit(LOG_ERR, "%s", "mdtx su has failed");
  }
  return(rc);
}

/*=======================================================================
 * Function   : mdtxScp
 * Description: call scp so as to not need setuid bit on daemon
 * Synopsis   : 
 * Input      : 
 *              
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
  logEmit(LOG_DEBUG, "mdtxScp %s", target);

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
    logEmit(LOG_ERR, "%s", "mdtxScp fails");
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
#ifndef utMAIN
  char* message = 0;
  char reply[256];
  int status = 0;
  int n = 0;
#endif

  if (isEmptyString(path)) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;
  if (!loadCollection(coll, EXTR)) goto error;

/* #ifndef utMAIN */
/*   // check if daemon is awake */
/*   if (access(conf->pidFile, R_OK) == -1) { */
/*     logEmit(LOG_INFO, "cannot read daemon's pid file: %s",  */
/* 	    strerror(errno)); */
/*     goto end; */
/*   } */
/* #endif */

  if (!(tree = createRecordTree())) goto error2; 
  tree->collection = coll;
  tree->messageType = UPLOAD; 
  if (!getLocalHost(coll)) goto error2;

  // get file attributes (size)
  if (stat(path, &statBuffer)) {
    logEmit(LOG_ERR, "status error on %s: %s", path, strerror(errno));
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
  
#ifdef utMAIN
  logEmit(LOG_INFO, "upload file to %s collection", label);
#else
  if ((socket = connectServer(coll->localhost)) == -1) goto error2;
  if (!upgradeServer(socket, tree, 0)) goto error2;

  // read reply
  n = tcpRead(socket, reply, 255);
  tcpRead(socket, reply, 1);

  // erase the \n send by server
  if (n<=0) n = 1;
  reply[n-1] = (char)0;

  if (sscanf(reply, "%i %s", &status, message) < 1) {
    logEmit(LOG_ERR, "error reading reply: %s", reply);
    goto error2;
  }
    
  logEmit(LOG_INFO, "local server tels (%i) %s", status, message);
  if (status != 200) goto error2;
#endif

  // add extraction rule
  if (!(container = addContainer(coll, REC, archive))) goto error;
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
    logEmit(LOG_ERR, "%s", "upload query failed");
    if (record) delRecord(coll, record);
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
  fprintf(stderr, " [ -d repository ]");

  mdtxOptions();
  fprintf(stderr, "  ---\n"
	  "  -d, --input-rep\trepository with logo files\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2010/12/10
 * Description: entry point for make module
 * Synopsis   : ./utmake
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[255] = ".";
  char* path = 0;
  Collection* coll = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"input-rep", required_argument, 0, 'd'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  //env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'd':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input repository\n",
		programName);
	rc = EINVAL;
	break;
      }
      strncpy(inputRep, optarg, strlen(optarg)+1);
      break; 

      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;
  
  /************************************************************************/
  // build the HTML catalog
  if (!mdtxMake("coll1")) goto error;

  // upload a file
  if (!(path = createString(inputRep))) goto error;
  if (!(path = catString(path, "/../../examples/logo.png"))) goto error;
  if (!mdtxUploadFile("coll1", path)) goto error;
  if (!(coll = addCollection("coll1"))) goto error;
  if (!saveCollection(coll, EXTR)) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  path = destroyString(path);
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
