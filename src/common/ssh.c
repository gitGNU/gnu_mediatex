/*=======================================================================
 * Version: $Id: ssh.c,v 1.8 2015/09/04 15:30:26 nroche Exp $
 * Project: MediaTeX
 * Module : ssh

 * update ssh user's configuration

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

/*=======================================================================
 * Function   : serializeAuthKeys
 * Description: Serialize the ~/.ssh/authorized_keys file.
 * Synopsis   : static int serializeAuthKeys(Collection* coll, char* path)
 * Input      : Collection* coll = what to serialize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeAuthKeys(Collection* coll)
{ 
  int rc = FALSE;
  char* path = 0;
  FILE* fd = stdout; 
  ServerTree* self = 0;
  Server* server = 0;
  RGIT* curr = 0;
  mode_t mask;

  mask = umask(0177);
  checkCollection(coll);
  if (!(self = coll->serverTree)) goto error;
  logCommon(LOG_DEBUG, "serialize authorized_keys file");

  path = coll->sshAuthKeys;
  logCommon(LOG_INFO, "Serializing the authorized_keys file: %s", 
	  path?path:"stdout");
      
  // output file
  if (!env.dryRun) {
    if ((fd = fopen(path, "w")) == 0) {
      logCommon(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
      goto error;
    }
  }
      
  fprintf(fd, "# This file is managed by MediaTeX software.\n");

  if (self->servers) {
    while ((server = rgNext_r(self->servers, &curr))) {
      fprintf(fd, "%s\n", server->userKey);
    }
  }

  fflush(fd);
  rc = TRUE;  
 error:  
  if (fd != stdout && fclose(fd)) {
    logCommon(LOG_ERR, "fclose fails: %s", strerror(errno));
    rc = FALSE;
  }
  if (!rc) {
    logCommon(LOG_ERR, "fail to serialize authorized_keys file");
  }
  umask(mask);
  return(rc);
}

/*=======================================================================
 * Function   : serializeSshConfig
 * Description: Serialize the ~/.ssh/config file.
 * Synopsis   : int serializeSshConfig(Collection* coll, char* path)
 * Input      : Collection* coll = what to serialize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeSshConfig(Collection* coll)
{ 
  int rc = FALSE;
  char* path = 0;
  FILE* fd = stdout; 
  ServerTree* self = 0;
  Server* server = 0;
  Server* previous = 0;
  RGIT* curr = 0;
  mode_t mask = 0;

  checkCollection(coll);
  if (!(self = coll->serverTree)) goto error;
  logCommon(LOG_DEBUG, "serialize ssh's config file");

  path = coll->sshConfig;
  logCommon(LOG_INFO, "Serializing the ssh's config file: %s", 
	  path?path:"stdout");
      
  // output file
  mask = umask(0177);
  if (!env.dryRun) {
    if ((fd = fopen(path, "w")) == 0) {
      logCommon(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
      goto error;
    }
  }
      
  fprintf(fd, "%s", "# This file is managed by MediaTeX software.\n\n");
  fprintf(fd, "%s", "# Do not ask for password\n"); 
  fprintf(fd, "%s", "BatchMode yes\n");
  fprintf(fd, "%s", "Compression yes\n");
  fprintf(fd, "%s", "#LogLevel DEBUG3\n\n");


  if (!isEmptyRing(self->servers)) {
    if (!rgSort(self->servers, cmpServer)) goto error;
    while ((server = rgNext_r(self->servers, &curr))) {
      if (isEmptyString(server->host) ||
	  (previous && !strcmp(server->host, previous->host))) 
	continue;
      fprintf(fd, "Host %s\n\tPort %i\n\n", server->host, server->sshPort);
      previous = server;
    }
  }
  
  fflush(fd);
  rc = TRUE;  
 error:
  if (fd != stdout && fclose(fd)) {
    logCommon(LOG_ERR, "fclose fails: %s", strerror(errno));
    rc = FALSE;
  }
  if (!rc) {
    logCommon(LOG_ERR, "fail to serialize ssh's config file");
  }
  umask(mask);
  return(rc);
}

/*=======================================================================
 * Function   : serializeKnownHosts
 * Description: Serialize the known_hosts file.
 * Synopsis   : int serializeKnownHosts(Collection* coll, char* path)
 * Input      : Collection* coll = what to serialize
 * Output     : TRUE on success
 * Note       : need to hash the output file by hand:
 *              ssh-keygen -H -f known_hosts
 =======================================================================*/
static int 
serializeKnownHosts(Collection* coll)
{ 
  int rc = FALSE;
  Configuration* conf = 0;
  char* path = 0;
  FILE* fd = stdout; 
  ServerTree* self = 0;
  Server* server = 0;
  RGIT* curr = 0;
  mode_t mask = 0;

  checkCollection(coll);
  if (!(conf = getConfiguration())) goto error;
  if (!(populateConfiguration())) goto error;
  if (!(self = coll->serverTree)) goto error;
  logCommon(LOG_DEBUG, "serialize known_hosts file");

  path = coll->sshKnownHosts;
  logCommon(LOG_INFO, "Serializing the known_host file: %s", 
	  path?path:"stdout");
      
  // output file
  mask = umask(0177);
  if (!env.dryRun) {
    if ((fd = fopen(path, "w")) == 0) {
      logCommon(LOG_ERR, "Cannot open known_host file for write: %s", 
		path);
      goto error;
    }
  }
      
  fprintf(fd, "# This file is managed by MediaTeX software.\n");

  // add twice localhost keys:
  // here using "localhost" name for local connexions
  fprintf(fd, "localhost %s\n", conf->hostKey);
  if (!strcmp(conf->host, "localhost")) {
    logCommon(LOG_WARNING, 
	    "Please avoid using localhost in the configuration");
  }

  // add all keys using the host name provided by servers.txt
  if (self->servers) {
    while ((server = rgNext_r(self->servers, &curr))) {
      if (server->hostKey) {

	// must add port != 22 given by .ssh/config file
	if (server->sshPort == 22) {
	  fprintf(fd, "%s %s\n", server->host, server->hostKey);
	}
	else {
	  fprintf(fd, "[%s]:%i %s\n", server->host, server->sshPort,
		  server->hostKey);
	}

	// warning message as you cannot be reach from the outside
	if (!strcmp(server->host, "localhost")) {
	  logCommon(LOG_WARNING, 
		  "Please avoid having localhost in the servers list");
	}
      }
    }
  }

  if (fd == stdout) {
    fflush(fd);
  }
  else {
    if (fclose(fd)) {
      logCommon(LOG_ERR, "fclose fails: %s", strerror(errno));
      goto error;
    }
  }
  rc = TRUE;  
 error:  
  if (!rc) {
    logCommon(LOG_ERR, "fail to serialize known_host file");
  }
  umask(mask);
  return(rc);
}


/*=======================================================================
 * Function   : sshKeygen
 * Description: call ssh-keygen -H -f .../.ssh/known_hosts
 * Synopsis   : static int sshKeygen(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
static int 
sshKeygen(Collection* coll)
{
  int rc = FALSE;
  char* argv[] = {"/usr/bin/ssh-keygen", "-Hf", 0, 0};
  char *path = 0;

  logCommon(LOG_DEBUG, "do ssh-keygen -H");
  argv[2] = coll->sshKnownHosts;


  if (!(path = createString(coll->sshKnownHosts))) goto error;
  if (!(path = catString(path, ".old"))) goto error;

  if (!env.noRegression && !env.dryRun) {
    // do $ ssh-keygen -H -f $CACHEDIR/home/$LABEL/.ssh/known_hosts
    if (!execScript(argv, 0, 0, TRUE)) goto error;
    
    // do $ rm -f $CACHEDIR/home/$LABEL/.ssh/known_hosts.old
    if (unlink(path) == -1) {
      logCommon(LOG_ERR, "error with unlink %s: %s", path, 
		strerror(errno));
      goto error;
    }
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logCommon(LOG_ERR, "fails to do ssh-keygen -H");
  }
  path = destroyString(path);
  return(rc);
}

/*=======================================================================
 * Function   : upgradeSshConfiguration
 * Description: Upgrade the .ssh repository 
 * Synopsis   : int upgradeSshConfiguration(Collection* coll)
 * Input      : Collection* coll = what to serialize
 * Output     : TRUE on success
 * Note       : cannot test it as we cannot execute a script from unit 
 *              test
 =======================================================================*/
int 
upgradeSshConfiguration(Collection* coll)
{ 
  int rc = FALSE;
  int uid = getuid();

  checkCollection(coll);
  if (!coll->serverTree) goto error;
  logCommon(LOG_DEBUG, "upgrade %s ssh configuration", coll->label);
  if (!rgSort(coll->serverTree->servers, cmpServer)) goto error;

  if (!becomeUser(coll->user, TRUE)) goto error;
  if (!serializeAuthKeys(coll)) goto error;
  if (!serializeSshConfig(coll)) goto error;
  if (!serializeKnownHosts(coll)) goto error;
  if (!sshKeygen(coll)) goto error;

  rc = TRUE;
 error:  
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logCommon(LOG_ERR, "fail to upgrade ssh configuration");
  }
  return(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
