/*=======================================================================
 * Version: $Id: supportTree.c,v 1.7 2015/08/13 21:14:34 nroche Exp $
 * Project: MediaTeX
 * Module : md5sumTree
 *
 * SupportTree producer interface

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
 * Function   : cmpSupport
 * Description: Compare 2 supports
 * Synopsis   : int cmpSupport(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2
 * Output     : like strncmp
 =======================================================================*/
int cmpSupport(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Archive* 
   */
  
  Support* a1 = *((Support**)p1);
  Support* a2 = *((Support**)p2);

  rc = strncmp(a1->name, a2->name, MAX_SIZE_NAME);
  return rc;
}

/*=======================================================================
 * Function   : createSupport
 * Description: Create, by memory allocation a Support
 *              configuration projection.
 * Synopsis   : Support* createSupport(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Support* 
createSupport(void)
{
  Support* rc = 0;

  rc = (Support*)malloc(sizeof(Support));
  if(rc == 0)
    goto error;
   
  memset(rc, 0, sizeof(Support));
  strncpy(rc->status, "ok", MAX_SIZE_STAT);
  if ((rc->collections = createRing()) == 0) goto error;

  return(rc);
 error:
  logMemory(LOG_ERR, "malloc: cannot create Support");
  return destroySupport(rc);
}

/*=======================================================================
 * Function   : destroySupport
 * Description: Destroy a configuration by freeing all the allocate memory.
 * Synopsis   : void destroySupport(Support* self)
 * Input      : Support* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a configuration.
 =======================================================================*/
Support* 
destroySupport(Support* self)
{
  Support* rc = 0;

  if(self) {
    // we do not free the objects (owned by conf), just the rings
    self->collections = destroyOnlyRing(self->collections);
    free(self);
  }

  return(rc);
}

/*=======================================================================
 * Function   : serializeSupport
 * Description: Serialize a support.
 * Synopsis   : int serializeSupport(Support* self, FILE* fd)
 * Input      : FILE* fd      = where to serialize
 *              Support* self = what to serialize
 * Output     :  0 = success
 *              -1 = error
 =======================================================================*/
int 
serializeSupport(Support* self, FILE *fd)
{
  struct tm firstSeen;
  struct tm lastCheck;
  struct tm lastSeen;

  if (fd == 0) fd = stdout;

  if(self == 0) {
    logMemory(LOG_ERR, "cannot serialize empty support");
    goto error;
  }

  if (localtime_r(&self->firstSeen, &firstSeen) == (struct tm*)0 ||
      localtime_r(&self->lastCheck, &lastCheck)  == (struct tm*)0 ||
      localtime_r(&self->lastSeen, &lastSeen) == (struct tm*)0 ) {
    logMemory(LOG_ERR, "localtime_r returns on error");
    goto error;
  }

  fprintf(fd, "\
%04i-%02i-%02i,%02i:%02i:%02i \
%04i-%02i-%02i,%02i:%02i:%02i \
%04i-%02i-%02i,%02i:%02i:%02i \
%*s %*s %*lli %*s %s\n", 
	  firstSeen.tm_year + 1900, firstSeen.tm_mon+1, firstSeen.tm_mday,
	  firstSeen.tm_hour, firstSeen.tm_min, firstSeen.tm_sec,
	  lastCheck.tm_year + 1900, lastCheck.tm_mon+1, lastCheck.tm_mday,
	  lastCheck.tm_hour, lastCheck.tm_min, lastCheck.tm_sec,
	  lastSeen.tm_year + 1900, lastSeen.tm_mon+1, lastSeen.tm_mday,
	  lastSeen.tm_hour, lastSeen.tm_min, lastSeen.tm_sec,
	  MAX_SIZE_HASH, self->quickHash, 
	  MAX_SIZE_HASH, self->fullHash, 
	  MAX_SIZE_SIZE, (long long int)self->size, 
	  MAX_SIZE_STAT, self->status, self->name);

  return TRUE;
 error:
  return FALSE;
}

/*=======================================================================
 * Function   : serializeSupportTree
 * Description: Serialize the supports tree.
 * Synopsis   : int serializeSupportTree()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeSupports()
{ 
  int rc = FALSE;
  Configuration* conf = 0;
  char* path = 0;
  FILE* fd = stdout; 
  Support *supp = 0;
  RGIT* curr = 0;
  int uid = getuid();

  logMemory(LOG_DEBUG, "serialize supports");

  if ((conf = getConfiguration()) == 0) {
    logMemory(LOG_ERR, "cannot load configuration");
    goto error;
  }

  if (!becomeUser(env.confLabel, TRUE)) goto error;

  // output file
  if (env.dryRun == FALSE) path = conf->supportDB;
  logMemory(LOG_INFO, "Serializing the supports list file: %s", 
	  path?path:"stdout");
  if (path && *path != (char)0) {
    if ((fd = fopen(path, "w")) == 0) {
      logMemory(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno));
      fd = stdout;
      goto error;
    }
    if (!lock(fileno(fd), F_WRLCK)) goto error;
  }

  fprintf(fd, "# This file is managed by MediaTeX software.\n");
  fprintf(fd, "# Version: $" "Id" "$\n");
  fprintf(fd, "# Local Supports:\n\n");

  fprintf(fd, "# %17s %19s %19s %*s %*s %*s %*s %s\n", 
	  "firstSeen", "lastCheck", "lastSeen",
	  MAX_SIZE_HASH, "quickHash", MAX_SIZE_HASH, "fullHash",
	  MAX_SIZE_SIZE, "size", MAX_SIZE_STAT, "status", "name");

  if (!isEmptyRing(conf->supports)) {
    if (!rgSort(conf->supports, cmpSupport)) goto error;
    while((supp = rgNext_r(conf->supports, &curr))) {
      if (!serializeSupport(supp, fd)) goto error;
    }
  }

  fflush(fd);
  rc = TRUE;
 error:
  if (fd != stdout) {
    if (!unLock(fileno(fd))) rc = FALSE;
    if (fclose(fd)) {
      logMemory(LOG_ERR, "fclose fails: %s", strerror(errno));
      rc = FALSE;
    }
  }
  if (!logoutUser(uid)) rc = FALSE;
  return rc;
}


/*=======================================================================
 * Function   : getSupport
 * Description: Find an support
 * Synopsis   : Support* getSupport(char* name)
 * Input      : char* name: id
 *              off_t size : id2 of the support
 * Output     : Support* : the Support we have found
 =======================================================================*/
Support* 
getSupport(char* name)
{
  Configuration* conf = 0;
  Support* rc = 0;
  RGIT* curr = 0;

  checkLabel(name);
  if (!(conf = getConfiguration())) goto error;
  logMemory(LOG_DEBUG, "getSupport %s", name);
  
  // look for support
  while((rc = rgNext_r(conf->supports, &curr)))
    if (!strncmp(rc->name, name, MAX_SIZE_NAME)) break;

 error:
  return rc;
}

/*=======================================================================
 * Function   : addSupport
 * Description: Add an support if not already there
 * Synopsis   : Support* addSupport(char* name)
 * Input      : char* name: id
 * Output     : Support* : the Support we have found/added
 =======================================================================*/
Support* 
addSupport(char* name)
{
  Support* rc = 0;
  Support* supp = 0;
  Configuration* conf = 0;

  checkLabel(name);
  logMemory(LOG_DEBUG, "addSupport %s", name);

  // already there
  if ((supp = getSupport(name))) goto end;

  // add new one if not already there
  if (!(conf = getConfiguration())) goto error;
  if ((supp = createSupport()) == 0) goto error;
  strncpy(supp->name, name, MAX_SIZE_NAME);
  supp->name[MAX_SIZE_NAME] = (char)0; // developpement code
  if (!rgInsert(conf->supports, supp)) goto error;

 end:
  rc = supp;
 error:
  if (rc == 0) {
    logMemory(LOG_ERR, "fails to add a support");
    supp = destroySupport(supp);
  }
  return rc;
}

/*=======================================================================
 * Function   : delSupport
 * Description: Del an support
 * Synopsis   : int delSupport(Support* support)
 * Input      : Support* support : the support to del
 * Output     : TRUE on success
 =======================================================================*/
int
delSupport(Support* self)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Collection* coll = 0;
  RGIT* curr = 0;
 
  checkSupport(self);
  if (!(conf = getConfiguration())) goto error;
  logMemory(LOG_DEBUG, "delSupport %s", self->name);

  // delete support from collections rings
  curr = 0;
  while((coll = rgNext_r(conf->collections, &curr)))
    if (!delSupportFromCollection(self, coll)) goto error;
  
  // delete support from configuration ring
  if ((curr = rgHaveItem(conf->supports, self))) {
    rgRemove_r(conf->supports, &curr);
  }

  // free the support
  destroySupport(self);
  rc = TRUE;
 error:
 if (!rc) {
    logMemory(LOG_ERR, "delSupport fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

