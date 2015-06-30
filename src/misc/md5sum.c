/*=======================================================================
 * Version: $Id: md5sum.c,v 1.4 2015/06/30 17:37:33 nroche Exp $
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

#if MAX_SIZE_HASH != (MD5_DIGEST_LENGTH << 1)
#error Bad size used to store md5sums !!
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
  if (sigaction(SIGALRM, &action, 0) != 0) {
    logEmit(LOG_ERR, "%s", "sigaction fails: %s", strerror(errno));
    goto error;
  }

  //if (!enableAlarm()) goto error;
  alarm(1); // call the manager
  rc = TRUE;
 error:
 if (!rc) {
    logEmit(LOG_ERR, "%s", "manageSIGALRM fails");
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
  if (env.noRegression || strncmp(env.logFacility, "file", 4)) 
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
  if (env.noRegression || strncmp(env.logFacility, "file", 4)) return; 

  e2fsck_clear_progbar(&env.progBar.bar);
  env.progBar.label = 0;
}

/*=======================================================================
 * Function   : md5sum2string
 * Description: hexa[12] to char[33] convertion
 * Synopsis   : char* md5sum2string(
 *                              unsigned char md5sum[MD5_DIGEST_LENGTH], 
 *                              char rc[MAX_SIZE_HASH + 1])
 * Input      : unsigned char md5sum[MD5_DIGEST_LENGTH]: 
 *               the md5sum to convert
 * Output     : char rc[MAX_SIZE_HASH + 1]: the allocated array to return
 =======================================================================*/
char* 
md5sum2string(unsigned char md5sum[MD5_DIGEST_LENGTH], 
	      char rc[MAX_SIZE_HASH + 1])
{
  int n;

  for(n=0; n<MD5_DIGEST_LENGTH; n++) 
    sprintf(rc+(n<<1), "%02x", md5sum[n]);

  rc[MAX_SIZE_HASH] = (char)0;
  return rc;
}

/*=======================================================================
 * Function   : computeQuickMd5
 * Description: Compute md5sum on first mega byte
 * Synopsis   : static int computeQuickMd5(int fd, ssize_t *sum, 
 *                                  MD5_CTX *c, off_t size, 
 *                                  char quickMd5sum[MAX_SIZE_HASH+1])
 * Input      : int fd: file descriptor to use for computation
 *              off_t size: maximum size of the file (for progbar)
 * Output     : ssize_t *sum: size used for computation
 *              MD5_CTX *c: md5sum data at the end (to be re-use next)
 *              char quickMd5sum[MAX_SIZE_HASH + 1]: the resulting md5sum
 * Note       : On GNU/Linux there is a bug with last block on CDROM
 *              so we truncate reading using size parameter
 =======================================================================*/
static int 
computeQuickMd5(int fd, ssize_t *sum, MD5_CTX *c, 
		off_t size, char quickMd5sum[MAX_SIZE_HASH + 1])
{
  int rc = FALSE;
  char buf[512];
  ssize_t bytes;
  unsigned char md5sum[MD5_DIGEST_LENGTH];
  MD5_CTX copy;

  logEmit(LOG_DEBUG, "%s", "computeQuickMd5");
  env.progBar.max = size;

  if (fd <= 0) {
    logEmit(LOG_ERR, "%s", "quickMd5: please provide a file descriptor");
    goto error;
  }

  if (sum == 0) {
  logEmit(LOG_ERR, "%s", "quickMd5: allocate provide a ssize_t argument");
    goto error;
  }

  if (quickMd5sum == 0) {
    logEmit(LOG_ERR, "%s", "quickMd5 please allocate quickMd5sum argument");
    goto error;
  } 

  if (lseek(fd, 0, SEEK_SET) != 0) {
    logEmit(LOG_ERR, "lseek: %s", strerror(errno));
    goto error;
  }

  *sum = 0;
  MD5_Init(c);
  bytes=read(fd, buf, 512);

  while((!size || *sum < size) &&
	*sum < MEGA && bytes > 0) {
    if (*sum + bytes > MEGA) bytes -= ((*sum + bytes) - MEGA);
    *sum += bytes;
    MD5_Update(c, buf, bytes);
    env.progBar.cur = *sum;
    bytes=read(fd, buf, 512);
  }
  
  memcpy(&copy, c, sizeof(MD5_CTX));
  MD5_Final(md5sum, &copy);
  quickMd5sum = md5sum2string(md5sum, quickMd5sum);

  logEmit(LOG_INFO, "%s quick md5sum computed on %llu bytes", 
	  quickMd5sum, (long long unsigned int)*sum);

  rc = (*sum > 0 && *sum <= MEGA);
 error:
  return(rc);
}

/*=======================================================================
 * Function   : computeFullMd5
 * Description: Continue to compute md5sum after the quickIndex bytes
 * Synopsis   : static int computeFullMd5(int fd, ssize_t *sum, MD5_CTX *c, 
 *                          off_t size, char fullMd5sum[MAX_SIZE_HASH+1])

 * Input      : int fd: file descriptor to use for computation
 *              ssize_t *sum: off_t size: maximum size of the file 
 *                 (for progbar)
 *              MD5_CTX *c: md5sum data at the end of quick computation
 *              off_t size: size where we continue computation
 * Output     : char fullMd5sum[MAX_SIZE_HASH + 1]): the resuling md5sum
 * Note       : On GNU/Linux there is a bug with last block on CDROM
 *              so we truncate reading using size parameter
 =======================================================================*/
static int 
computeFullMd5(int fd, ssize_t *sum, MD5_CTX *c, 
	       off_t size, char fullMd5sum[MAX_SIZE_HASH + 1])
{
  int rc = FALSE;
  char buf[512];
  ssize_t bytes;
  unsigned char md5sum[MD5_DIGEST_LENGTH];

  logEmit(LOG_DEBUG, "%s", "computeFullMd5");
  env.progBar.max = size;

  if (fd <= 0) {
    logEmit(LOG_ERR, "%s", "fullMd5: please provide a file descriptor");
    goto error;
  }
  
  if (sum == (ssize_t*)0) {
    logEmit(LOG_ERR, "%s", "fullMd5: please allocate a ssize_t argument");
    goto error;
  }

  if (c == (MD5_CTX*)0) {
    logEmit(LOG_ERR, "%s", "fullMd5 please allocate MD5_CTX argument");
    goto error;
  } 

  if (fullMd5sum == 0) {
    logEmit(LOG_ERR, "%s", "fullMd5 please allocate fullMd5sum argument");
    goto error;
  } 
  
  if (lseek(fd, *sum, SEEK_SET) != *sum) {
    logEmit(LOG_ERR, "lseek: %s", strerror(errno));
    goto error;
  }
  
  bytes=read(fd, buf, 512);
  while((!size || *sum < size) && bytes > 0) {
    *sum += bytes;
    MD5_Update(c, buf, bytes);
    env.progBar.cur = *sum;
    bytes=read(fd, buf, 512);
  }
  
  MD5_Final(md5sum, c);
  fullMd5sum = md5sum2string(md5sum, fullMd5sum);
  logEmit(LOG_INFO, "%s  full md5sum computed on %llu bytes", 
	  fullMd5sum, (long long unsigned int)*sum);

  rc = TRUE;
 error:
  return(rc);
}

/*=======================================================================
 * Function   : doMd5sum
 * Description: API for md5sum computations
 * Synopsis   : int doMd5sum(Md5Data* data)
 * Input      : Md5Data* data: see md5sum.h
 * Output     : TRUE on success
 =======================================================================*/
int 
doMd5sum(Md5Data* data)
{
  int rc = FALSE;
  int fd = -1;
  ssize_t sum = 0;
  MD5_CTX c;
  off_t size;
  char quickMd5sum[MAX_SIZE_HASH+1];
  char fullMd5sum[MAX_SIZE_HASH+1];
  char* backupPath = 0;
  int isBlockDev = FALSE;
  unsigned short int bs = 0;
  unsigned long int count = 0;

  logEmit(LOG_DEBUG, "%s", "doMd5sum");

  // backup the parameter values
  backupPath = data->path;
  size = data->size;
  strncpy(quickMd5sum, data->quickMd5sum, MAX_SIZE_HASH+1); 
  strncpy(fullMd5sum, data->fullMd5sum, MAX_SIZE_HASH+1); 

  if (data->path == 0 || *(data->path) == (char)0) {
    logEmit(LOG_ERR, "%s", "Please provide a path for md5sum computing");
    goto error;
  }

  // when we deal with support devices
  if (data->opp != MD5_CACHE_ID) {
    // look for an external device real path
    if (!getDevice(backupPath, &data->path)) goto error;
  }
  
  if ((fd = open(data->path, O_RDONLY)) == -1) {
    logEmit(LOG_ERR, "open: %s", strerror(errno));
    goto error;
  }

  // compute size
  if (data->opp != MD5_CACHE_ID) {
    if (!isBlockDevice(data->path, &isBlockDev)) goto error;
    if (isBlockDev) {
      if (!getIsoSize(fd, &data->size, &count, &bs)) goto error;
    }
    else {
      if ((data->size = lseek(fd, 0, SEEK_END)) == -1) {
	logEmit(LOG_ERR, "lseek: %s", strerror(errno));
	goto error;
      }
    }

    // start progbar
    if (!startProgBar(data->path)) goto error;
  }
  
  data->rc = 0; // MD5_SUCCESS
  switch (data->opp) {
  case MD5_CACHE_ID:
    rc = computeQuickMd5(fd, &sum, &c, data->size, data->quickMd5sum);
    rc&= computeFullMd5(fd, &sum, &c, data->size, data->fullMd5sum);
    break;
  case MD5_SUPP_ADD:
    MD5_Init(&c);
    rc = computeQuickMd5(fd, &sum, &c, data->size, data->quickMd5sum);
    rc&= computeFullMd5(fd, &sum, &c, data->size, data->fullMd5sum);
    break;
  case MD5_SUPP_ID:
  case MD5_SUPP_CHECK:
    if (size != 0 && data->size != size) {
      logEmit(LOG_WARNING, "size doesn't match: %llu vs %llu expected", 
	      (long long unsigned int) data->size, 
	      (long long unsigned int) size);
      data->rc |= MD5_FALSE_SIZE;
      rc = TRUE;
      goto error;
    }

    rc = computeQuickMd5(fd, &sum, &c, data->size, data->quickMd5sum);
    if (strncmp(data->quickMd5sum, quickMd5sum, MAX_SIZE_HASH)) {
      logEmit(LOG_INFO, "quick md5sum doesn't match: %s vs %s expected", 
	     data->quickMd5sum, quickMd5sum);
      data->rc |= MD5_FALSE_QUICK;
      goto error;
    }

    if (data->opp == MD5_SUPP_ID) break;
    
    rc = computeFullMd5(fd, &sum, &c, data->size, data->fullMd5sum);
    if (strncmp(data->fullMd5sum, fullMd5sum, MAX_SIZE_HASH)) {
      logEmit(LOG_INFO, "full md5sum doesn't match: %s vs %s expected", 
	      data->fullMd5sum, fullMd5sum);
      data->rc |= MD5_FALSE_QUICK;
      goto error;
    }
    break;
  default:
    logEmit(LOG_ERR, "unknown value for Md5Opp parameter: %i", data->opp);
    goto error;
  }

 error:
  stopProgBar(); // stop progBar
  if (fd != -1 && close(fd) == -1) {
    logEmit(LOG_ERR, "close: %s", strerror(errno));
    rc = FALSE;
  }
  data->rc |= ((~rc) & 1); // |= MD5_ERROR (if !rc)
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
