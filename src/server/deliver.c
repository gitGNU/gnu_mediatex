/*=======================================================================
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
  CheckData md5; 
  char* path = 0;
  struct stat statBuffer;
  char size[MAX_SIZE_SIZE+1];
  char status[2]="0";
  char *argv[] = {0, 0, 0, 0, 0, 0, 0};
  
  logMain(LOG_DEBUG, "auditArchive");
  checkCollection(coll);
  checkRecord(demand);
  supply = demand->archive->localSupply;
  checkRecord(supply);

  logMain(LOG_INFO, "auditArchive %s", supply->extra);
  if (!(path = getAbsoluteRecordPath(coll, supply))) goto error;

  // get file size
  if (stat(path, &statBuffer)) {
    logMain(LOG_ERR, "stat fails on %s: %s", path, strerror(errno));
    goto error;
  }

  // compute checksum
  memset(&md5, 0, sizeof(CheckData));
  md5.path = path;
  md5.opp = CHECK_CACHE_ID;
  if (!doChecksum(&md5)) goto error;

  // double check supply (should not comes here on wrong checksum)
  argv[5] = "0"; // success
  if (statBuffer.st_size != demand->archive->size) {
    logMain(LOG_WARNING, "bad size auditing %s: %lli vs %lli expected",
	    supply->extra, (long long int)statBuffer.st_size,
	    (long long int)demand->archive->size);
    status[0] = '1'; // failure
  }
  if (strncmp(md5.fullMd5sum, demand->archive->hash, MAX_SIZE_MD5)) {
    logMain(LOG_WARNING, "bad md5sum auditing %s: %s vs %s expected",
	    supply->extra, md5.fullMd5sum, demand->archive->hash);
    status[0] = '1'; // failure
  }
  
  // call script to update report
  if (!(argv[0] = createString(getConfiguration()->scriptsDir))
      || !(argv[0] = catString(argv[0], "/audit.sh"))) 
    goto error;

  argv[1] = coll->label;
  argv[2] = demand->extra;
  argv[3] = demand->archive->hash;
  sprintf(size, "%lli", (long long int)demand->archive->size);
  argv[4] = size;
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
  const int finalDemand =  4; // time user read mail
  const int remoteDemand = 2; // scp (time between 2 cron)
  const int localDemand =  1; // download initiated by cgi
  int rc = FALSE;
  Configuration* conf = 0;
  Record* record = 0;
  time_t date = -1;
  RGIT* curr = 0;
  int todo = 0;
  
  logMain(LOG_DEBUG, "deliverArchive");
  if (!(conf = getConfiguration())) goto error;
  if ((date = currentTime()) == -1) goto error; 

  // find longer to-Keep time to honnor all demands
  while ((record = rgNext_r(archive->demands, &curr))) {
    if (record->type & REMOVE) continue;

    switch (getRecordType(record)) {
    case FINAL_DEMAND:
      if (!strncmp(record->extra, CONF_AUDIT, strlen(CONF_AUDIT))) {
	if (!auditArchive(coll, record)) goto error;
      }
      else {
	todo |= finalDemand;
      }
      break;
    case REMOTE_DEMAND:
      todo |= remoteDemand;
      break;
    case LOCAL_DEMAND:
      todo |= localDemand;
      break;
    default:
      logMain(LOG_WARNING, "a demand record was expected");
    }
  }

  // mail + download http
  if (todo >= finalDemand) {
    date += coll->cacheTTL;
    date -= 1*DAY;
  }

  // time between 2 cron
  if (todo >= remoteDemand) {
      date += 1*DAY;
  }

  // download time
  if (todo >= localDemand) {
    date += archive->size / conf->uploadRate + 5;
  }

  // adjust to-keep date
  if (todo >= localDemand) {
    if (archive->localSupply->date < date)
      archive->localSupply->date = date;
    if (archive->backupDate < date)
      archive->backupDate = date;
    if (!computeArchiveStatus(coll, archive)) goto error;
  }

  // deliver mails (for all final-demands but not on audit)
  curr = 0;
  while ((record = rgNext_r(archive->demands, &curr))) {
    if (record->type & REMOVE) continue;
    if (getRecordType(record) != FINAL_DEMAND) continue;
    if (!strncmp(record->extra, CONF_AUDIT, strlen(CONF_AUDIT))) continue;

    // we should use a lock to prevent mail to be sent by other threads
    record->type |= REMOVE;
    if (!callMail(coll, archive->localSupply, record->extra)) goto error;
    if (!delCacheEntry(coll, record)) goto error;
  }

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
