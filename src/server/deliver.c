/*=======================================================================
 * Version: $Id: deliver.c,v 1.6 2015/08/07 17:50:33 nroche Exp $
 * Project: MediaTeX
 * Module : deliver
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

#include "mediatex-config.h"
#include "server/mediatex-server.h"

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
  
  logMain(LOG_DEBUG, "send a mail to %s", address);

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
    logMain(LOG_ERR, "fails to send a mail");
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
  logMain(LOG_DEBUG, "deliverMail %s:%lli", archive->hash, archive->size);
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
    logMain(LOG_DEBUG, "deliverMail fails");
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

  logMain(LOG_DEBUG, "delivering %s collection files to users",
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
      logMain(LOG_WARNING,
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
    logMain(LOG_DEBUG, "deliverMails fails");
  }
  path = destroyString(path);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
