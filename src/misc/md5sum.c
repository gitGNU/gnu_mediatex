/*=======================================================================
 * Version: $Id: md5sum.c,v 1.10 2015/09/21 01:01:51 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
 *
 * md5sum computation

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
#include <openssl/md5.h>
#include <openssl/sha.h>

#if MAX_SIZE_MD5 != (MD5_DIGEST_LENGTH << 1)
#error Bad size used to store md5sums !!
#endif

#if MAX_SIZE_SHA != (SHA_DIGEST_LENGTH << 1)
#error Bad size used to store shasums !!
#endif


/*=======================================================================
 * Function   : manageSIGALRM
 * Description: load the alarm manager for the progression bar
 * Synopsis   : int manageSIGALRM(void (*manager)(int))
 * Input      : void (*manager)(int): manager callback
 * Output     : TRUE on success
 =======================================================================*/
static int 
manageSIGALRM(void (*manager)(int))
{
  int rc = FALSE;
  struct sigaction action;

  initProgBar(&env.progBar.bar);

  action.sa_handler = manager;
  sigemptyset(&(action.sa_mask));
  action.sa_flags = SA_RESTART;
  if (sigaction(SIGALRM, &action, 0)) {
    logMisc(LOG_ERR, "sigaction fails: %s", strerror(errno));
    goto error;
  }

  //if (!enableAlarm()) goto error;
  alarm(1); // call the manager
  rc = TRUE;
 error:
 if (!rc) {
    logMisc(LOG_ERR, "manageSIGALRM fails");
  }
  return TRUE;
}

/*=======================================================================
 * Function   : gestionnaireSIGALRM
 * Description: update the progression bar
 * Synopsis   : void gestionnaireSIGALRM(int i)
 * Input      : N/A (data from env.progBar...)
 * Output     : N/A
 =======================================================================*/
static void 
gestionnaireSIGALRM(int i)
{
  (void) i;

  // generate SIGALARM signal until porbarLabel is free
  if (env.progBar.label) {
    e2fsck_simple_progress(&env.progBar.bar, env.progBar.label, 
			   calc_percent(env.progBar.cur, env.progBar.max));
    alarm(1); // try setitimer if microseconds are wanted
  }
  /* else { */
  /*    disableAlarm(); */
  /* } */
}

/*=======================================================================
 * Function   : startProgBar
 * Description: 
 * Synopsis   : 
 * Input      : char* label
 * Output     : N/A
 =======================================================================*/
int startProgBar(char* label)
{
  // only run progbar for client (but not for cgi or server)
  if (env.noRegression || env.logFacility != 99) 
    return TRUE; 

  env.progBar.label = label;
  return manageSIGALRM(gestionnaireSIGALRM);
}

/*=======================================================================
 * Function   : endProgBar
 * Description: 
 * Synopsis   : 
 * Input      : char* label
 * Output     : N/A
 =======================================================================*/
void stopProgBar()
{
  // only run progbar for client (but not for cgi or server)
  if (env.noRegression || env.logFacility != 99) return; 

  e2fsck_clear_progbar(&env.progBar.bar);
  env.progBar.label = 0;
}

/*=======================================================================
 * Function   : md5sum2string
 * Description: hexa[12] to char[33] convertion
 * Synopsis   : char* md5sum2string(
 *                              unsigned char md5sum[MD5_DIGEST_LENGTH], 
 *                              char rc[MAX_SIZE_MD5 + 1])
 * Input      : unsigned char md5sum[MD5_DIGEST_LENGTH]: 
 *               the md5sum to convert
 * Output     : char rc[MAX_SIZE_MD5 + 1]: the allocated array to return
 =======================================================================*/
char* 
md5sum2string(unsigned char md5sum[MD5_DIGEST_LENGTH], 
	      char rc[MAX_SIZE_MD5 + 1])
{
  int n;

  for(n=0; n<MD5_DIGEST_LENGTH; n++) 
    sprintf(rc+(n<<1), "%02x", md5sum[n]);

  rc[MAX_SIZE_MD5] = (char)0;
  return rc;
}

/*=======================================================================
 * Function   : sha1sum2string
 * Description: hexa[20] to char[40] convertion
 * Synopsis   : char* sha1sum2string(
 *                              unsigned char sha1sum[SHA_DIGEST_LENGTH], 
 *                              char rc[MAX_SIZE_SHA1 + 1])
 * Input      : unsigned char sha1sum[SHA_DIGEST_LENGTH]: 
 *               the sha1sum to convert
 * Output     : char rc[MAX_SIZE_SHA1 + 1]: the input array
 =======================================================================*/
char* 
sha1sum2string(unsigned char sha1sum[SHA_DIGEST_LENGTH], 
	      char rc[MAX_SIZE_SHA + 1])
{
  int n;

  for(n=0; n<SHA_DIGEST_LENGTH; n++) 
    sprintf(rc+(n<<1), "%02x", sha1sum[n]);

  rc[MAX_SIZE_SHA] = (char)0;
  return rc;
}

/*=======================================================================
 * Function   : quickChecksum
 * Description: Compute checksum(s) on first mega byte
 * Synopsis   : static int quickChecksum(int fd, off_t size, ssize_t *sum,
 *                   MD5_CTX *md5Ctx, char quickMd5sum[MAX_SIZE_MD5 + 1],
                     SHA_CTX *shaCtx, char quickShasum[MAX_SIZE_SHA + 1])
 * Input      : int fd: file descriptor to use for computation
 *              off_t size: maximum size of the file
 * Output     : ssize_t *sum: size used for computation
 *              MD5_CTX *md5Ctx: md5sum data at the end (to be re-use next)
 *              char quickMd5sum[MAX_SIZE_MD5 + 1]: the resulting md5sum
 *              SHA_CTX *shaCtx: shasum data at the end (to be re-use next)
 *              char quickShasum[MAX_SIZE_SHA + 1]: the resulting sha1sum
 * Note       : On GNU/Linux there is a bug with last block on CDROM
 *              so we truncate reading using size parameter
 =======================================================================*/
static int 
quickChecksum(int fd, off_t size, ssize_t *sum,
	      MD5_CTX *md5Ctx, char quickMd5sum[MAX_SIZE_MD5 + 1],
	      SHA_CTX *shaCtx, char quickShasum[MAX_SIZE_SHA + 1])
{
  int rc = FALSE;
  char buf[512];
  ssize_t bytes;
  unsigned char md5sum[MD5_DIGEST_LENGTH];
  unsigned char shasum[SHA_DIGEST_LENGTH];
  MD5_CTX tmpMd5;
  SHA_CTX tmpSha;
  int doSha1 = FALSE;

  logMisc(LOG_DEBUG, "quickChecksum");
  doSha1 = (shaCtx && quickShasum);
  env.progBar.max = size;

  if (fd <= 0) {
    logMisc(LOG_ERR, "please provide a file descriptor");
    goto error;
  }

  if (sum == 0) {
  logMisc(LOG_ERR, "allocate provide a ssize_t argument");
    goto error;
  }

  if (quickMd5sum == 0) {
    logMisc(LOG_ERR, "please allocate quickMd5sum argument");
    goto error;
  } 

  MD5_Init(md5Ctx);
  if (doSha1) SHA1_Init(shaCtx);

  if (lseek(fd, 0, SEEK_SET)) {
    logMisc(LOG_ERR, "lseek: %s", strerror(errno));
    goto error;
  }

  *sum = 0;
  bytes=read(fd, buf, 512);
  while ((!size || *sum < size) &&
	*sum < MEGA && bytes > 0) {
    if (*sum + bytes > MEGA) bytes -= ((*sum + bytes) - MEGA);
    *sum += bytes;
    MD5_Update(md5Ctx, buf, bytes);
    if (doSha1) SHA1_Update(shaCtx, buf, bytes);
    env.progBar.cur = *sum;
    bytes=read(fd, buf, 512);
  }
  
  memcpy(&tmpMd5, md5Ctx, sizeof(MD5_CTX));
  MD5_Final(md5sum, &tmpMd5);
  quickMd5sum = md5sum2string(md5sum, quickMd5sum);
  logMisc(LOG_INFO, "%s quick md5sum computed on %llu bytes", 
	  quickMd5sum, (long long unsigned int)*sum);
  
  if (doSha1) {
    memcpy(&tmpSha, shaCtx, sizeof(SHA_CTX));
    SHA1_Final(shasum, &tmpSha);
    quickShasum = sha1sum2string(shasum, quickShasum);
    logMisc(LOG_INFO, "%s quick sha1sum computed on %llu bytes", 
	    quickShasum, (long long unsigned int)*sum);
  }

  rc = (*sum > 0 && *sum <= MEGA);
 error:
  if (!rc) {
    logMisc(LOG_ERR, "quickChecksum fails");
  }
  return(rc);
}

/*=======================================================================
 * Function   : fullChecksum
 * Description: Continue to compute checksum(s) after the first mega byte
 * Synopsis   : static int fullChecksum(int fd, ssize_t *sum, off_t size,
 *                    MD5_CTX *md5Ctx, char fullMd5sum[MAX_SIZE_MD5 + 1],
 *                    SHA_CTX *shaCtx, char fullShasum[MAX_SIZE_SHA + 1])
 * Input      : int fd: file descriptor to use for computation
 *              ssize_t *sum: off_t size: maximum size of the file 
 *              off_t size: size where we continue computation
 * Output     : MD5_CTX *md5Ctx: md5sum at the end of quick computation
 *              char fullMd5sum[MAX_SIZE_MD5 + 1]: the resuling md5sum
 *              SHA_CTX *shaCtx: shasum at the end of quick computation
 *              char fullShasum[MAX_SIZE_SHA + 1]: the resuling shasum
 * Note       : On GNU/Linux there is a bug with last block on CDROM
 *              so we truncate reading using size parameter
 =======================================================================*/
static int 
fullChecksum(int fd, off_t size, ssize_t *sum,
	       MD5_CTX *md5Ctx, char fullMd5sum[MAX_SIZE_MD5 + 1],
	       SHA_CTX *shaCtx, char fullShasum[MAX_SIZE_SHA + 1])
{
  int rc = FALSE;
  char buf[512];
  ssize_t bytes;
  unsigned char md5sum[MD5_DIGEST_LENGTH];
  unsigned char shasum[SHA_DIGEST_LENGTH];
  int doSha1 = FALSE;

  logMisc(LOG_DEBUG, "fullChecksum");
  doSha1 = (shaCtx && fullShasum);
  env.progBar.max = size;

  if (fd <= 0) {
    logMisc(LOG_ERR, "please provide a file descriptor");
    goto error;
  }
  
  if (sum == (ssize_t*)0) {
    logMisc(LOG_ERR, "please allocate a ssize_t argument");
    goto error;
  }

  if (md5Ctx == (MD5_CTX*)0) {
    logMisc(LOG_ERR, "please allocate MD5_CTX argument");
    goto error;
  } 

  if (fullMd5sum == 0) {
    logMisc(LOG_ERR, "please allocate fullMd5sum argument");
    goto error;
  } 
  
  if (lseek(fd, *sum, SEEK_SET) != *sum) {
    logMisc(LOG_ERR, "lseek: %s", strerror(errno));
    goto error;
  }
  
  bytes=read(fd, buf, 512);
  while ((!size || *sum < size) && bytes > 0) {
    *sum += bytes;
    MD5_Update(md5Ctx, buf, bytes);
    if (doSha1) SHA1_Update(shaCtx, buf, bytes);
    env.progBar.cur = *sum;
    bytes=read(fd, buf, 512);
  }
  
  MD5_Final(md5sum, md5Ctx);
  fullMd5sum = md5sum2string(md5sum, fullMd5sum);
  logMisc(LOG_INFO, "%s  full md5sum computed on %llu bytes", 
	  fullMd5sum, (long long unsigned int)*sum);

  if (doSha1) {
    SHA1_Final(shasum, shaCtx);
    fullShasum = sha1sum2string(shasum, fullShasum);
    logMisc(LOG_INFO, "%s full sha1sum computed on %llu bytes", 
	    fullShasum, (long long unsigned int)*sum);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "fullChecksum fails");
  }
  return(rc);
}

/*=======================================================================
 * Function   : doChecksum
 * Description: API for md5sum computations
 * Synopsis   : int doChecksum(CheckData* data)
 * Input      : CheckData* data: see md5sum.h
 * Output     : TRUE on success
 =======================================================================*/
int 
doChecksum(CheckData* data)
{
  int rc = FALSE;
  int fd = -1;
  ssize_t sum = 0;
  MD5_CTX md5Ctx;
  SHA_CTX shaCtx;
  off_t size;
  char quickMd5sum[MAX_SIZE_MD5+1];
  char fullMd5sum[MAX_SIZE_MD5+1];
  char quickShasum[MAX_SIZE_SHA+1];
  char fullShasum[MAX_SIZE_SHA+1];
  char* backupPath = 0;
  int isBlockDev = FALSE;
  unsigned short int bs = 0;
  unsigned long int count = 0;

  logMisc(LOG_DEBUG, "doChecksum");

  // backup the parameter values
  backupPath = data->path;
  size = data->size;
  strncpy(quickMd5sum, data->quickMd5sum, MAX_SIZE_MD5+1); 
  strncpy(fullMd5sum, data->fullMd5sum, MAX_SIZE_MD5+1); 
  strncpy(quickShasum, data->quickShasum, MAX_SIZE_SHA+1); 
  strncpy(fullShasum, data->fullShasum, MAX_SIZE_SHA+1); 

  if (data->path == 0 || *(data->path) == (char)0) {
    logMisc(LOG_ERR, "Please provide a path for checksum computing");
    goto error;
  }

  // when we deal with support devices
  if (data->opp != CHECK_CACHE_ID) {
    // look for an external device real path
    if (!getDevice(backupPath, &data->path)) goto error;
  }
  
  if ((fd = open(data->path, O_RDONLY)) == -1) {
    logMisc(LOG_ERR, "open: %s", strerror(errno));
    goto error;
  }

  // compute size
  if (data->opp != CHECK_CACHE_ID) {
    if (!isBlockDevice(data->path, &isBlockDev)) goto error;
    if (isBlockDev) {
      if (!getIsoSize(fd, &data->size, &count, &bs)) goto error;
    }
    else {
      if ((data->size = lseek(fd, 0, SEEK_END)) == -1) {
	logMisc(LOG_ERR, "lseek: %s", strerror(errno));
	goto error;
      }
    }

    // start progbar
    if (!startProgBar(data->path)) goto error;
  }
  
  data->rc = 0; // CHECK_SUCCESS
  switch (data->opp) {
  case CHECK_CACHE_ID:
    rc = quickChecksum(fd, data->size, &sum, 
		       &md5Ctx, data->quickMd5sum, 0, 0);
    rc&= fullChecksum(fd, data->size, &sum, 
		      &md5Ctx, data->fullMd5sum, 0, 0);
    break;
  case CHECK_SUPP_ADD:
    rc = quickChecksum(fd, data->size, &sum, 
		       &md5Ctx, data->quickMd5sum, 
		       &shaCtx, data->quickShasum);
    rc&= fullChecksum(fd, data->size, &sum, 
		      &md5Ctx, data->fullMd5sum, 
		      &shaCtx, data->fullShasum);
    break;
  case CHECK_SUPP_ID:
  case CHECK_SUPP_CHECK:
    if (size && data->size != size) {
      logMisc(LOG_WARNING, "size doesn't match: %llu vs %llu expected", 
	      (long long unsigned int) data->size, 
	      (long long unsigned int) size);
      data->rc |= CHECK_FALSE_SIZE;
      rc = TRUE;
      goto error;
    }

    rc = quickChecksum(fd, data->size, &sum, 
		       &md5Ctx, data->quickMd5sum, 
		       &shaCtx, data->quickShasum);
    
    if (strncmp(data->quickMd5sum, quickMd5sum, MAX_SIZE_MD5)) {
      logMisc(LOG_INFO, "quick md5sum doesn't match: %s vs %s expected", 
	     data->quickMd5sum, quickMd5sum);
      data->rc |= CHECK_FALSE_QUICK;
      goto error;
    }
    if (strncmp(data->quickShasum, quickShasum, MAX_SIZE_SHA)) {
      logMisc(LOG_INFO, "quick sha1sum doesn't match: %s vs %s expected", 
	     data->quickShasum, quickShasum);
      data->rc |= CHECK_FALSE_QUICK;
      goto error;
    }

    if (data->opp == CHECK_SUPP_ID) break;
    
    rc&= fullChecksum(fd, data->size, &sum, 
		      &md5Ctx, data->quickMd5sum, 
		      &shaCtx, data->quickShasum);

    if (strncmp(data->fullMd5sum, fullMd5sum, MAX_SIZE_MD5)) {
      logMisc(LOG_INFO, "full md5sum doesn't match: %s vs %s expected", 
	      data->fullMd5sum, fullMd5sum);
      data->rc |= CHECK_FALSE_QUICK;
      goto error;
    }
    if (strncmp(data->fullShasum, fullShasum, MAX_SIZE_SHA)) {
      logMisc(LOG_INFO, "full sha1sum doesn't match: %s vs %s expected", 
	      data->fullShasum, fullShasum);
      data->rc |= CHECK_FALSE_QUICK;
      goto error;
    }

    break;
  default:
    logMisc(LOG_ERR, "unknown value for Md5Opp parameter: %i", data->opp);
    goto error;
  }

 error:
  stopProgBar(); // stop progBar
  if (fd != -1 && close(fd) == -1) {
    logMisc(LOG_ERR, "close: %s", strerror(errno));
    rc = FALSE;
  }
  data->rc |= ((~rc) & 1); // |= CHECK_ERROR (if !rc)
  if (data->path != backupPath) {
    free(data->path);
    data->path = backupPath;
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
