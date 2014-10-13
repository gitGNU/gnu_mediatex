/*=======================================================================
 * Version: $Id: supportTree.c,v 1.1 2014/10/13 19:39:12 nroche Exp $
 * Project: MediaTeX
 * Module : md5sumTree
 *
 * SupportTree producer interface

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
#include "supportTree.h"
#include "confTree.h"

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
  Support* rc = NULL;

  rc = (Support*)malloc(sizeof(Support));
  if(rc == NULL)
    goto error;
   
  memset(rc, 0, sizeof(Support));
  strncpy(rc->status, "ok", MAX_SIZE_STAT);
  if ((rc->collections = createRing()) == NULL) goto error;

  return(rc);
 error:
  logEmit(LOG_ERR, "%s", "malloc: cannot create Support");
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
  Support* rc = NULL;

  if(self != NULL) {
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

  if (fd == NULL) fd = stdout;

  if(self == NULL) {
    logEmit(LOG_ERR, "%s", "cannot serialize empty support");
    goto error;
  }

  if (localtime_r(&self->firstSeen, &firstSeen) == (struct tm*)0 ||
      localtime_r(&self->lastCheck, &lastCheck)  == (struct tm*)0 ||
      localtime_r(&self->lastSeen, &lastSeen) == (struct tm*)0 ) {
    logEmit(LOG_ERR, "%s", "localtime_r returns on error");
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
  Configuration* conf = NULL;
  char* path = NULL;
  FILE* fd = stdout; 
  Support *supp = NULL;
  RGIT* curr = NULL;
  int uid = getuid();

  logEmit(LOG_DEBUG, "%s", "serialize supports");

  if ((conf = getConfiguration()) == NULL) {
    logEmit(LOG_ERR, "%s", "cannot load configuration");
    goto error;
  }

  if (!becomeUser(env.confLabel, TRUE)) goto error;

  // output file
  if (env.dryRun == FALSE) path = conf->supportDB;
  logEmit(LOG_INFO, "Serializing the supports list file: %s", 
	  path?path:"stdout");
  if (path != NULL && *path != (char)0) {
    if ((fd = fopen(path, "w")) == NULL) {
      logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno));
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
      logEmit(LOG_ERR, "fclose fails: %s", strerror(errno));
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
  Configuration* conf = NULL;
  Support* rc = NULL;
  RGIT* curr = NULL;

  checkLabel(name);
  if (!(conf = getConfiguration())) goto error;
  logEmit(LOG_DEBUG, "getSupport %s", name);
  
  // look for support
  while((rc = rgNext_r(conf->supports, &curr)) != NULL)
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
  Support* rc = NULL;
  Support* supp = NULL;
  Configuration* conf = NULL;

  checkLabel(name);
  logEmit(LOG_DEBUG, "addSupport %s", name);

  // already there
  if ((supp = getSupport(name))) goto end;

  // add new one if not already there
  if (!(conf = getConfiguration())) goto error;
  if ((supp = createSupport()) == NULL) goto error;
  strncpy(supp->name, name, MAX_SIZE_NAME);
  supp->name[MAX_SIZE_NAME] = (char)0; // developpement code
  if (!rgInsert(conf->supports, supp)) goto error;

 end:
  rc = supp;
 error:
  if (rc == NULL) {
    logEmit(LOG_ERR, "%s", "fails to add a support");
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
  Configuration* conf = NULL;
  Collection* coll = NULL;
  RGIT* curr = NULL;
 
  checkSupport(self);
  if (!(conf = getConfiguration())) goto error;
  logEmit(LOG_DEBUG, "delSupport %s", self->name);

  // delete support from collections rings
  curr = NULL;
  while((coll = rgNext_r(conf->collections, &curr)) != NULL)
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
    logEmit(LOG_ERR, "%s", "delSupport fails");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "utFunc.h"
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
  memoryUsage(programName);

  memoryOptions();
  //fprintf(stderr, "\t\t---\n");
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for confTree module.
 * Synopsis   : utconfTree
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Support* supp = NULL;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MEMORY_SHORT_OPTIONS;
  struct option longOptions[] = {
    MEMORY_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MEMORY_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  // test types on this architecture:
  off_t offset = 0xFFFFFFFFFFFFFFFFULL; // 2^64
  time_t timer = 0x7FFFFFFF; // 2^32
  logEmit(LOG_NOTICE, "type off_t is store into %u bytes", 
	  (unsigned int)sizeof(off_t));
  logEmit(LOG_NOTICE, "type time_t is store into %u bytes", 
	  (unsigned int) sizeof(time_t));
  logEmit(LOG_NOTICE,  "off_t max value is: %llu", 
	  (unsigned long long int)offset);
  logEmit(LOG_NOTICE, "time_t max value is: %lu", 
	  (unsigned long int)timer);

  // test memory tree
  if (!createExempleSupportTree()) goto error;
  if (!(supp = getSupport("SUPP11_logo.png"))) goto error;
  if (!serializeSupports()) goto error;
  /************************************************************************/

  freeConfiguration();
  rc = TRUE;
 error:
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

