/*=======================================================================
 * Version: $Id: cypher.c,v 1.7 2015/08/13 21:14:35 nroche Exp $
 * Project: MediaTeX
 * Module : cypher
 *
 * aes encryption

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

/*=======================================================================
 * Function   : aesInit
 * Description: load the AES key
 * Synopsis   : int aesInit(AESData* data, char key[MAX_SIZE_AES+1], 
 *                          MDTX_AES_WAY order)
 * Input      : AESData* data: AES context
 *              MDTX_AES_WAY way: data must be initialized for 
 *                                ENCRYPT or DECRYPT dedicated way
 * Output     : AESData* data: AES context
 *              TRUE on success
 =======================================================================*/
int 
aesInit(AESData* data, char key[MAX_SIZE_AES+1], MDTX_AES_WAY way)
{
  int rc = FALSE;

  if (key == 0 || key[MAX_SIZE_AES] != (char)0) goto error;
  logMisc(LOG_DEBUG, "loadAesKey: '%s' for %s", key,
	  way==ENCRYPT?"writing":(way==DECRYPT?"reading":"??"));
  if (data == 0) {
    logMisc(LOG_ERR, "please provide a AESData structure");
    goto error;
  }

  switch (way) {
  case ENCRYPT:
    /* 
     * key contains the actual 128-bit AES key. aeskey is a data structure 
     * holding a transformed version of the key, for efficiency. 
     */
    if (AES_set_encrypt_key((const unsigned char *) key, 
			    128, &data->aesKey)) {
      logMisc(LOG_ERR, "AES_set_encrypt_key fails");
      goto error;
    }
    break;

  case DECRYPT:
    /* 
     * We need to set AESkey appropriately before inverting AES. 
     * Note that the underlying key Key is the same; just the data structure
     * AESkey is changing (for reasons of efficiency).
     */
    if (AES_set_decrypt_key((const unsigned char *) key, 
			    128, &data->aesKey)) {
      logMisc(LOG_ERR, "AES_set_decrypt_key fails");
      goto error;
    }
    break;

  default:
    logMisc(LOG_ERR, "crypt way should be ENCRYPT or DECRYPT");
    goto error;
  }
  
  /* int i; */
  /* printf("key\t "); */
  /* for (i=0; i<MAX_SIZE_AES; ++i) printf("%x ", data->aesKey.rd_key[i]); */
  /* printf("\n"); */

  data->way = way;
  data->doCypher = FALSE;
  data->mBlock[MAX_SIZE_AES] = (char)0;
  data->cBlock[MAX_SIZE_AES] = (char)0;
  data->index = 0;
  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "loadAesKey fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : doCypher
 * Description: cypher an AES buffer (16 bytes)
 * Synopsis   : int doCypher(AESData* data)
 * Input      : AESData* data: AES context
 * Output     : AESData* data: AES context
 *              TRUE on success
 =======================================================================*/
int 
doCypher(AESData* data) {
  int rc = FALSE;
  //int i;

  logMisc(LOG_DEBUG, "doCypher");

  if (data == 0) {
    logMisc(LOG_ERR, "please provide a AESData structure");
    goto error;
  }

  /* printf("\nkey\t "); */
  /* for (i=0; i<MAX_SIZE_AES; ++i) printf("%x ", data->aesKey.rd_key[i]); */
  /* printf("\n"); */
  
  switch (data->way) {
  case ENCRYPT:
    /* printf("encrypt\t'%s'\n", data->mBlock); */
    /* printf("ie\t "); */
    /* for (i=0; i<MAX_SIZE_AES; ++i) printf("%2x ", data->mBlock[i]); */
    /* printf("\n"); */

    AES_encrypt((const unsigned char *) data->mBlock, data->cBlock, 
		(const AES_KEY *) &data->aesKey);
    
    /* printf("as\t "); */
    /* for (i=0; i<MAX_SIZE_AES; ++i) printf("%2x ", data->cBlock[i]); */
    /* printf("\n"); */
    break;

  case DECRYPT:
    /* printf("decrypt\t "); */
    /* for (i=0; i<MAX_SIZE_AES; ++i) printf("%2x ", data->cBlock[i]); */
    /* printf("\n"); */

    AES_decrypt((const unsigned char *) data->cBlock, data->mBlock, 
		(const AES_KEY *) &data->aesKey);

    /* printf("as\t'%s'\n", data->mBlock); */
    break;
    
  default:
    logMisc(LOG_ERR, "crypt order should be ENCRYPT or DECRYPT");
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "doCypher fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : fdWrite
 * Description: write into a file descriptor
 * Synopsis   : int fdWrite(int fd, char* buffer, size_t bufferSize)
 * Input      : fd:         file descriptor
 *              buffer:     string to write
 *              bufferSize: number of char to write
 * Output     : TRUE on success
 =======================================================================*/
int 
fdWrite(int fd, void* buffer, size_t bufferSize)
{
  int rc = FALSE;
  void *next = buffer;
  size_t remaining = bufferSize;
  ssize_t writen = -1;

  while (remaining > 0){
    while ((writen = write(fd, next, remaining)) == -1) {
      if (errno == EINTR) {
	logMisc(LOG_WARNING, "writting file interrupted by kernel: %s",
		strerror(errno));
	continue;
      }
      if (errno != EAGAIN) {
	logMisc(LOG_ERR, "writting file error: %s", strerror(errno));
	goto error;
      }
      logMisc(LOG_WARNING, "writting file keep EAGAIN error: %s", 
	      strerror(errno));
    }
    remaining -= writen;
    next += writen;

    //logMisc(LOG_INFO, "written %d ; remain %d", writen, remaining); */
  }

  rc = (remaining == 0);
 error:
  return rc;
}

/*=======================================================================
 * Function   : fdRead
 * Description: read into a file
 * Synopsis   : size_t fdRead(int sd, char* buffer, size_t bufferSize)
 * Input      : sd:          socket descriptor
 *              buffer:      string to write
 *              bufferSize:  number of char to write
 * Output     : number of char written
 =======================================================================*/
size_t
fdRead(int fd, void* buffer, size_t bufferSize)
{
  size_t rc = FALSE;
  char *next = buffer;
  size_t remaining = bufferSize;
  ssize_t nread = -1;

  while (remaining > 0 && (nread = read(fd, next, remaining))) {
    if (nread == -1) {
      if (errno != EINTR) {
	logMisc(LOG_ERR, "reading file error: %s", strerror(errno));
	goto error;
      }
      logMisc(LOG_WARNING, "reading file interrupted by kernel: %s",
	      strerror(errno));
    }
    else {
      rc += nread;
      next += nread;
      remaining -= nread;

      //logMisc(LOG_INFO, "read %d ; remain %d", rc, remaining); */
    }
  }

 error:
  return rc;
}

/*=======================================================================
 * Function   : aesFlush
 * Description: pad with space the pending content and flush it
 * Synopsis   : int aesFlush(AESData* data)
 * Input      : AESData* data: AES context
 * Output     : AESData* data: AES context
 *              TRUE on success
 =======================================================================*/
int 
aesFlush(AESData* data) 
{
  int rc = FALSE;

  logMisc(LOG_DEBUG, "aesFlush"); 

  if (data == 0) {
    logMisc(LOG_ERR, "please provide a AESData structure");
    goto error;
  }

  if (data->way != ENCRYPT) {
    logMisc(LOG_ERR, "AESData isn't initialized for writing");
    goto error;
  }

  if (data->index == 0) goto nothingTodo;

  // pad the crypto buffer with spaces
  while (data->index < MAX_SIZE_AES) {
    data->mBlock[data->index++] = ' ';
  }

  if (data->doCypher) {
    if (!doCypher(data)) goto error;
    if (!fdWrite(data->fd, data->cBlock, MAX_SIZE_AES)) goto error;
  }
  else {
    if (!fdWrite(data->fd, data->mBlock, MAX_SIZE_AES)) goto error;
  }
  
 nothingTodo:
  data->index = 0;
  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "aesFlush fails");
  }

  return rc;
}

/*=======================================================================
 * Function   : aesPrint
 * Description: encrypt into the data->fd file descriptor
 * Synopsis   : int aesPrint(AESData* data, const char* format, ...) {
 * Input      : AESData* data: AES context
 *              const char* format, ...: like printf
 * Output     : AESData* data: AES context
 *              TRUE on success
 * Note       :
 Warning: vsnprintf may write characteres into severals bytes,
 and theses byte may be split when we cut into MAX_SIZE_AES arrays
 =======================================================================*/
int 
aesPrint(AESData* data, const char* format, ...) 
{
  int rc = FALSE;
  va_list args;
  static int maxSize = 512;
  char buffer[maxSize];
  int index = 0;
  int len = 0;
  int max = 0;

  logMisc(LOG_DEBUG, "aesPrint"); 

  if (data == 0) {
    logMisc(LOG_ERR, "please provide a AESData structure");
    goto error;
  }

  if (data->way != ENCRYPT) {
    logMisc(LOG_ERR, "AESData isn't initialized for writing");
    goto error;
  }

  // print into the buffer
  va_start(args, format);
  len = vsnprintf(buffer, maxSize, format, args);
  if (len < 0 || len > maxSize) {
    logMisc(LOG_ERR, "vnsprintf fails: size=%i/%i", len, maxSize);
    goto error;
  }
  va_end(args);

  // concatenate on a non empty message buffer
  if (data->index > 0) {
    max = MAX_SIZE_AES - data->index;
    if (len < max) max = len;
    memcpy(data->mBlock + data->index, buffer, max);
    data->index += max;
    if (data->index < MAX_SIZE_AES) goto end;
    if (!aesFlush (data)) goto error;
    index += max;
  }

  // message buffer is now fully available
  while (index < len && index + MAX_SIZE_AES <= len) {
    memcpy(data->mBlock, buffer + index, MAX_SIZE_AES);
    data->index = MAX_SIZE_AES;
    if (!aesFlush (data)) goto error;
    index += MAX_SIZE_AES;
  }

  // copy what remains into the begining of the message buffer
  if (len - index > 0) {
    data->index = len - index;
    memcpy(data->mBlock, buffer + index, data->index);
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "aesPrint fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : aesInput
 * Description: callback function for scanner (YYINPUT)
 * Synopsis   : int aesInput(AESData* data, char* buf, int *result, 
 *                           int maxsize) 
 * Input      : AESData* data: AES context
 *              int maxsize: maximum size in buf (not used here)
 * Output     : char* buf: the buffer to return
 *              int *result: number of char we put on the buffer
 *              TRUE on success
 =======================================================================*/
int 
aesInput(AESData* data, char* buf, int *result, int maxsize) 
{
  int rc = FALSE;
  int len = 0;

  (void) maxsize;
  *result = 0;

  logMisc(LOG_DEBUG, "aesInput"); 

  if (data == 0) {
    logMisc(LOG_ERR, "please provide a AESData structure");
    goto error;
  }

  if (data->way != DECRYPT) {
    logMisc(LOG_ERR, "AESData isn't initialized for reading");
    goto error;
  }

  // reading from the input
  len = fdRead(data->fd, data->cBlock, MAX_SIZE_AES);
  if (len == 0) goto empty;
  if (len != MAX_SIZE_AES) {
    logMisc(LOG_WARNING, "crypted input was'nt padded: remains %i bytes",
	    MAX_SIZE_AES - len);

    // we pad with space if not cyphered (ie: telnet tests)
    if (data->doCypher) goto error;
    while (len<MAX_SIZE_AES) data->cBlock[len++] = ' ';
  }

  if (data->doCypher) {
    if (!doCypher(data)) goto error;
    memcpy(buf, data->mBlock, MAX_SIZE_AES);
  }
  else {
    memcpy(buf, data->cBlock, MAX_SIZE_AES);
  }
  
  *result = MAX_SIZE_AES;
  /* debug
  fflush(stderr);
  fprintf(stderr, "\n->%s<-\n", data->cBlock);
  fprintf(stderr, "\n->%s<-\n", data->mBlock);
  fflush(stderr);
  */
 empty:
  rc = TRUE;
 error:
  if (!rc) {
    logMisc(LOG_ERR, "aesInput fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
