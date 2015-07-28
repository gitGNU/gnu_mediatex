/*=======================================================================
 * Version: $Id: keys.c,v 1.5 2015/07/28 11:45:46 nroche Exp $
 * Project: MediaTeX
 * Module : keys
 *
 * ssh keys retrievals

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
#include <openssl/md5.h> // MD5_DIGEST_LENGTH
#include <openssl/evp.h> // EVP_DecodeBlock


/*=======================================================================
 * Function   : readPublicKey
 * Description: open an public key file on read it
 * Synopsis   : char* readPublicKey(char* path) 
 * Input      : char* path: the public key file
 * Output     : the allocated key string or 0 on failure
 =======================================================================*/
char* 
readPublicKey(char* path) 
{
  char* rc = 0;
  FILE* fd = 0;
  char buf[1024];
  char* ptr = buf;
  int n = 0;

  if (path == 0 || *path == (char)0) {
    logMisc(LOG_ERR, "%s", 
	    "please provide a path for the public key to read");
    goto error;
  }

  logMisc(LOG_DEBUG, "readPublicKey: %s", path);

  if ((fd = fopen(path, "r")) == 0) {
    logMisc(LOG_ERR, "fopen fails: %s", strerror(errno));
    goto error;
  }

  *ptr = (char)0; // ending \0
  while ((ptr - buf < 1024) && !feof(fd) && !ferror(fd)) {
    n = fread(ptr, 1, 256, fd);
    ptr += n;
  }

  if (ferror(fd)) {
    logMisc(LOG_ERR, "%s", "fread fails.");
    goto error;
  }

  if (fclose(fd)) {
    logMisc(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error;
  }

  // remove the ending \n
  if (ptr > buf) {
    *(ptr-1) = (char)0;
  }

  if ((rc = malloc (strlen(buf)+1)) == 0) {
    logMisc(LOG_ERR, "malloc fails: %s", strerror(errno));
    goto error;
  }

  strcpy(rc, buf);
 error:
  if (rc == 0) {
    logMisc(LOG_ERR, "%s", "readPublicKey fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : getFingerPrint
 * Description: compute the fingerprint of a key as:
 *              ssh-keygen -lf /etc/ssh/ssh_host_dsa_key.pub
 * Synopsis   : int getFingerPrint(char* key, 
 *                                 char fingerprint[MAX_SIZE_HASH])
 * Input      : char* key: the input key
 *              char fingerprint[MAX_SIZE_HASH]: the fingerprint to store
 * Output     : TRUE on success
 *
 * Note       :
 * 1) http://www.federationhq.de/blog/calculate-ssh-rsa-fingerprint
 * 2) The EVP digest routines are a hight level interface to message
 * digests
 * 3) '=' are use by ssh to pad the base64 encoding, however 
 * the 'key_unbased64_length - nbEquals' trick I found looks very
 * strange (but working)
 =======================================================================*/
int 
getFingerPrint(char* key, char fingerprint[MAX_SIZE_HASH+1])
{
  int rc = FALSE;
  unsigned char *key_unbase64 = (unsigned char *)0;
  int key_unbased64_length = 0;
  int i;
  EVP_MD_CTX mdctx;
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len;
  char* end;
  char* ptr;
  int nbEquals = 0;
  
  logMisc(LOG_DEBUG, "%s", "getFingerPrint");

  // get only the key
  while (*key != ' ') ++key;
  ++key;
  end = key;
  while (*end != ' ') ++end;
  *end = (char)0;
  
  // count how much ending '='
  for (ptr = end-1 ; *ptr == '='; --ptr) ++nbEquals;

  // unbase64 the public key

  // we just use the size of the public key for now
  if ((key_unbase64 = malloc(strlen(key))) == 0) {
    logMisc(LOG_ERR, "malloc fails: %s", strerror(errno));
    goto error;
  }

  /* printf("%s\n", key); */
  /* printf("length= %i\n", key_unbased64_length); */
  /* printf("nbEquals = %i\n", nbEquals); */
    
  // we feed the public key and its length to the function
  key_unbased64_length = 
    EVP_DecodeBlock(key_unbase64, (unsigned char *) key, strlen(key));
  if (key_unbased64_length <= 0) {
    logMisc(LOG_ERR, "EVP_DecodeBlock fails: %i", key_unbased64_length);
    goto error;
  }

  // The next step is, to calculate an MD5 hash from key_unbase64.

  // get us an openssl context
  EVP_MD_CTX_init(&mdctx);

  // initialize our context to use MD5 as the hashing algorithm
  if (EVP_DigestInit_ex(&mdctx, EVP_md5(), 0) != 1) {
    logMisc(LOG_ERR, "%s", "EVP_DigestInit_ex fails");
    goto error;
  }

  // put the unbase64 key into the context
  // -nbEquals is neeeded to work by looks very strange
  if (EVP_DigestUpdate(&mdctx, key_unbase64,
  		       key_unbased64_length - nbEquals) != 1) {
    logMisc(LOG_ERR, "%s", "EVP_DigestUpdate fails");
    goto error;
  }

  // finish the hashing and get us the hash value in md_value
  if (EVP_DigestFinal_ex(&mdctx, md_value, &md_len) != 1) {
    logMisc(LOG_ERR, "%s", "EVP_DigestFinal fails");
    goto error;
  }

  // retrieve the md5sum
  for(i=0; i<MD5_DIGEST_LENGTH; i++)
    sprintf(fingerprint+(i<<1), "%02x", md_value[i]);
  fingerprint[MAX_SIZE_HASH] = (char)0;
 
  EVP_MD_CTX_cleanup(&mdctx);
  rc = TRUE;
 error:
  *end = (char)0;
  if (key_unbase64) free(key_unbase64);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
