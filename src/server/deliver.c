/*=======================================================================
 * Version: $Id: deliver.c,v 1.10 2015/09/21 01:01:52 nroche Exp $
 * Project: MediaTeX
 * Module : deliver
 *
 * Manage delivering mail

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
#include "server/mediatex-server.h"

/*=======================================================================
 * Function   : callMail
 * Description: call mail
 * Synopsis   : callMail(Collection* coll, Record* record, char* address)
 * Input      : Collection* coll
 *              char* Record* record = archive we deliver
 *              char* address = mail address to use
 * Output     : TRUE on success
 * Note       : we need to load bash in order to do the '<' redirection
 =======================================================================*/
int 
callMail(Collection* coll, Record* record, char* address)
{
  int rc = FALSE;
  char *argv[] = {0, 0, 0, 0, 0, 0, 0};
  char available[16];
  char url[512];
  
  logMain(LOG_DEBUG, "send a mail to %s", address);

  if (sprintf(available, "%i", (int)(coll->cacheTTL / DAY)) <= 0)
    goto error;
  
  if (sprintf(url, "%s/cgi/get.cgi?hash=%s&size=%lli",
	      coll->localhost->url,
	      record->archive->hash, 
	      (long long int)record->archive->size)<= 0) goto error;
      
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
 * Function   : auditArchive
 * Description: check archive on notice it
 * Synopsis   : int auditArchive(Collection* coll, Record* record)
 * Input      : Collection* coll
 *              Record* record
 * Output     : TRUE on success
 =======================================================================*/
int auditArchive(Collection* coll, Record* demand)
{
  int rc = FALSE;
  Record* supply = 0;
  struct stat statBuffer;
  CheckData md5; 
  char* path = 0;
  char *argv[] = {0, 0, 0, 0, 0, 0, 0};
  char size[MAX_SIZE_SIZE+1];
  char status[2];
  
  logMain(LOG_DEBUG, "auditArchive");
  checkCollection(coll);
  checkRecord(demand);
  supply = demand->archive->localSupply;
  checkRecord(supply);

  // get file attributes (size)
  if (!(path = getAbsoluteRecordPath(coll, supply))) goto error;
  if (stat(path, &statBuffer)) {
    logMain(LOG_ERR, "status error on %s: %s", supply->extra, 
	    strerror(errno));
    goto error;
  }

  // check hash (result is given by md5.rc)
  memset(&md5, 0, sizeof(CheckData));
  md5.path = path;
  md5.size = statBuffer.st_size;
  md5.opp = CHECK_CACHE_ID;
  if (!doChecksum(&md5)) goto error;

  // call script to update report
  if (!(argv[0] = createString(getConfiguration()->scriptsDir))
      || !(argv[0] = catString(argv[0], "/audit.sh"))) 
    goto error;

  argv[1] = coll->label;
  argv[2] = demand->extra;
  argv[3] = demand->archive->hash;
  sprintf(size, "%lli", (long long int)demand->archive->size);
  argv[4] = size;
  sprintf(status, "%i", md5.rc);
  argv[5] = status;
  
  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, 0, 0, FALSE)) goto error;
  }

  demand->type |= REMOVE;
  if (!delCacheEntry(coll, demand)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "auditArchive fails");
  }
  destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : deliverArchive
 * Description: adjuste toKeep time and eventually send mails
 * Synopsis   : int deliverArchive()
 * Input      : Collection* coll
 *              Archive* archive
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
  while ((record = rgNext_r(archive->demands, &curr))) {
    if (record->type & REMOVE) continue;

    switch (getRecordType(record)) {
    case FINAL_DEMAND: // mail + download http
      
      // workaround for audit
      if (!strncmp(record->extra, CONF_AUDIT, strlen(CONF_AUDIT))) {
	if (!auditArchive(coll, record)) goto error;
	continue;
      }

      date += coll->cacheTTL;
      date -= 1*DAY;
    case REMOTE_DEMAND: // scp (time between 2 cron)
      date += 1*DAY;
    case LOCAL_DEMAND: // cgi
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
  while ((record = rgNext_r(archive->demands, &curr))) {
    if (record->type & REMOVE) continue;
    if (getRecordType(record) != FINAL_DEMAND) continue;
    if (!strncmp(record->extra, CONF_AUDIT, strlen(CONF_AUDIT))) continue;

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
