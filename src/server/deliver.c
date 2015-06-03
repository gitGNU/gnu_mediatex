/*=======================================================================
 * Version: $Id: deliver.c,v 1.3 2015/06/03 14:03:55 nroche Exp $
 * Project: MediaTeX
 *
 * Manage delivering mail

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
#include "../memory/strdsm.h"
#include "../parser/confFile.tab.h"
#include "../parser/recordList.tab.h"
#include "../common/register.h"
#include "../common/connect.h"
#include "../common/openClose.h"
#include "cache.h"
#include "deliver.h"

#include <sys/stat.h> // open
#include <fcntl.h>    //

#ifdef utMAIN
#include "utFunc.h"
#endif

/*=======================================================================
 * Function   : callMail
 * Description: call mail
 * Synopsis   : callMail(Collection* coll, char* mail, char* path)
 * Input      : Collection* coll
 *              char* mail = mail address to use
 *              char* path = path to the file to send as mail content
 * Output     : TRUE on success
* Note       : we need to load bash in order to do the '<' redirection
 =======================================================================*/
int 
callMail(Collection* coll, Record* record, char* address)
{
  int rc = FALSE;
  char *argv[] = {0, 0, 0, 0, 0, 0, 0};
  char available[16];
  char url[256];
  
  logEmit(LOG_DEBUG, "send a mail to %s", address);

  sprintf(available, "%i", (int)(coll->cacheTTL / DAY));
  sprintf(url, "%s?hash=%s&size=%lli", coll->cgiUrl, 
	 record->archive->hash, 
	 (long long int)record->archive->size);

  if (!(argv[0] = createString(getConfiguration()->scriptsDir))
      || !(argv[0] = catString(argv[0], "/deliver.sh"))) 
    goto error;

  argv[1] = coll->label;
  argv[2] = address;
  argv[3] = available;
  argv[4] = record->extra;
  argv[5] = url;
  
  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to send a mail");
  }
  if (argv[0]) destroyString(argv[0]);
  return(rc);
}

/*=======================================================================
 * Function   : deliverMail
 * Description: Deliver notifications to users that have subscribe for
 *              a file to download
 * Synopsis   : int deliverMail(Collection* coll, Record* record)
 * Input      : Collection* coll
 *              RecordTree* records
 * Output     : TRUE on success

 * TODO:      : use a lock to prevent mail to be sent twice (if bad luck)
 =======================================================================*/
int 
deliverMail(Collection* coll, Archive* archive)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Record* record = 0;
  Record* record2 = 0;
  Record* next = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  checkArchive(archive);
  record = archive->localSupply;
  checkRecord(record);
  logEmit(LOG_DEBUG, "deliverMail %s:%lli", archive->hash, archive->size);
  if (!(conf = getConfiguration())) goto error;

  // send same message for all record
  next = rgNext_r(archive->demands, &curr);
  while (next) {
    record2 = next;
    next = rgNext_r(archive->demands, &curr);

    if (getRecordType(record2) == FINALE_DEMAND &&
	!(record2->type & REMOVE)) {
      record2->type |= REMOVE;
      // we should use a lock to prevent mail to be sent twice
      if (!callMail(coll, record, record2->extra)) goto error;
      if (!delCacheEntry(coll, record2)) goto error;
    }
  }

  // update toKeep date
  if (!keepArchive(coll, archive, FINALE_DEMAND)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_DEBUG, "%s", "deliverMail fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : deliverMails
 * Description: Deliver notifications to users that have subscribe for
 *              a file to download
 * Synopsis   : int deliverMails(Collection* coll)
 * Input      : Collection* coll
 *              RecordTree* records
 * Output     : TRUE on success
 =======================================================================*/
int
deliverMails(Collection* coll)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Archive* archive = 0;
  Record* record = 0;
  char* path = 0;
  RGIT* curr = 0;
  RGIT* curr2 = 0;
  int deliver = FALSE;

  logEmit(LOG_DEBUG, "delivering %s collection files to users",
	  coll->label);

  if (!(conf = getConfiguration())) goto error;
  if (!loadCollection(coll, CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;

  // for each cache entry
  while((archive = rgNext_r(coll->cacheTree->archives, &curr))
	!= 0) {

    // look if archive is supplyed
    if (archive->localSupply == 0) continue;

    // test if the file is really there
    path = destroyString(path);
    if (!(path = createString(coll->cacheDir))) goto error3;
    if (!(path = catString(path, "/"))) goto error3;
    if (!(path = catString(path, archive->localSupply->extra))) 
      goto error3;

    if (access(path, R_OK) == -1) {
      logEmit(LOG_WARNING,
	      "file not find in cache as expected: %s", path);
      continue;
    }

    // look if we have final demands on it
    deliver = FALSE;
    curr2 = 0;
    while((record = rgNext_r(archive->demands, &curr2))) {
      if (getRecordType(record) == FINALE_DEMAND) {
	deliver = TRUE;
	break;
      }
    }

    // send messages
    if (deliver && !deliverMail(coll, archive)) goto error3;
  }

  rc = TRUE;
 error3:
  if (!unLockCache(coll)) goto error;
 error2:
  if (!releaseCollection(coll, CACH)) goto error;
 error:
  if (!rc) {
    logEmit(LOG_DEBUG, "%s", "deliverMails fails");
  }
  path = destroyString(path);
  return rc;
}

/************************************************************************/

#ifdef utMAIN
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

  mdtxOptions();
  //fprintf(stderr, "  ---\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2010/12/10
 * Description: entry point for mdtx-env
 * Synopsis   : mdtx-env
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
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
  if (!(coll = mdtxGetCollection("coll3"))) goto error;

  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;

  utLog("%s", "add a demand for logo.png:", 0);
  if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075, 
		"nroche@narval.tk")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.png")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  utLog("%s", "deliver the mail:", 0);
  if (!deliverMails(coll)) goto error;
  utLog("%s", "Finaly we have :", coll);

  if (!utCleanCaches()) goto error;
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
