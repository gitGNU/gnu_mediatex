/*=======================================================================
 * Version: $Id: deliver.c,v 1.7 2015/08/09 11:12:35 nroche Exp $
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
 * Function   : deliverArchive
 * Description: adjuste toKeep time and eventually send mails
 * Synopsis   : int deliverArchive()
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int deliverArchive(Collection* coll, Archive* archive)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Record* record = 0;
  time_t date = -1;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "deliverArchive");
  if (!(conf = getConfiguration())) goto error;
  if ((date = currentTime()) == -1) goto error; 

  // find longer to-Keep time to honnor all demands
  while((record = rgNext_r(archive->demands, &curr))) {
    if (record->type & REMOVE) continue;

    switch (getRecordType(record)) {
    case FINALE_DEMAND: // mail + download http
      date += coll->cacheTTL;
      date -= 1*DAY;
    case REMOTE_DEMAND: // scp (time between 2 cron)
      date += 1*DAY;
    case LOCALE_DEMAND: // cgi
      date += archive->size / conf->uploadRate;
      break;
    default:
      goto end;
    }
  }

  // adjust to-keep date
  if (archive->localSupply->date < date) {
    archive->localSupply->date = date;
    if (!computeArchiveStatus(coll, archive)) goto error;
  }

  // deliver mail
  curr = 0;
  while((record = rgNext_r(archive->demands, &curr))) {
    if (record->type & REMOVE) continue;
    if (getRecordType(record) != FINALE_DEMAND) continue;

    // we should use a lock to prevent mail to be sent twice
    record->type |= REMOVE;
    if (!callMail(coll, archive->localSupply, record->extra)) goto error;
    if (!delCacheEntry(coll, record)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "deliverArchive fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
