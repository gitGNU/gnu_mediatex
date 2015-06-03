/*=======================================================================
 * Version: $Id: keys.c,v 1.3 2015/06/03 14:03:45 nroche Exp $
 * Project: MediaTeX
 * Module : keys
 *
 * uuid and keys retrievals

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

#include "alloc.h"
#include "keys.h"

#include <openssl/evp.h>
#include <openssl/md5.h>


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
    logEmit(LOG_ERR, "%s", 
	    "please provide a path for the public key to read");
    goto error;
  }

  logEmit(LOG_DEBUG, "readPublicKey: %s", path);

  if ((fd = fopen(path, "r")) == 0) {
    logEmit(LOG_ERR, "fopen fails: %s", strerror(errno));
    goto error;
  }

  *ptr = (char)0; // ending \0
  while ((ptr - buf < 1024) && !feof(fd) && !ferror(fd)) {
    n = fread(ptr, 1, 256, fd);
    ptr += n;
  }

  if (ferror(fd)) {
    logEmit(LOG_ERR, "%s", "fread fails.");
    goto error;
  }

  if (fclose(fd)) {
    logEmit(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error;
  }

  // remove the ending \n
  if (ptr > buf) {
    *(ptr-1) = (char)0;
  }

  if ((rc = malloc (strlen(buf)+1)) == 0) {
    logEmit(LOG_ERR, "malloc fails: %s", strerror(errno));
    goto error;
  }

  strcpy(rc, buf);
 error:
  if (rc == 0) {
    logEmit(LOG_ERR, "%s", "readPublicKey fails");
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
  
  logEmit(LOG_DEBUG, "%s", "getFingerPrint");

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
    logEmit(LOG_ERR, "malloc fails: %s", strerror(errno));
    goto error;
  }

  /* printf("%s\n", key); */
  /* printf("length= %i\n", key_unbased64_length); */
  /* printf("nbEquals = %i\n", nbEquals); */
    
  // we feed the public key and its length to the function
  key_unbased64_length = 
    EVP_DecodeBlock(key_unbase64, (unsigned char *) key, strlen(key));
  if (key_unbased64_length <= 0) {
    logEmit(LOG_ERR, "EVP_DecodeBlock fails: %i", key_unbased64_length);
    goto error;
  }

  // The next step is, to calculate an MD5 hash from key_unbase64.

  // get us an openssl context
  EVP_MD_CTX_init(&mdctx);

  // initialize our context to use MD5 as the hashing algorithm
  if (EVP_DigestInit_ex(&mdctx, EVP_md5(), 0) != 1) {
    logEmit(LOG_ERR, "%s", "EVP_DigestInit_ex fails");
    goto error;
  }

  // put the unbase64 key into the context
  // -nbEquals is neeeded to work by looks very strange
  if (EVP_DigestUpdate(&mdctx, key_unbase64,
  		       key_unbased64_length - nbEquals) != 1) {
    logEmit(LOG_ERR, "%s", "EVP_DigestUpdate fails");
    goto error;
  }

  // finish the hashing and get us the hash value in md_value
  if (EVP_DigestFinal_ex(&mdctx, md_value, &md_len) != 1) {
    logEmit(LOG_ERR, "%s", "EVP_DigestFinal fails");
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

/************************************************************************/

#ifdef utMAIN
#include "command.h"
GLOBAL_STRUCT_DEF;

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static 
void usage(char* programName)
{
  miscUsage(programName);
  fprintf(stderr, "\n\t\t-i path");

  miscOptions();
  fprintf(stderr, "  ---\n"
  	  "  -i, --input-key\tprint the key content\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for md5sum module
 * Synopsis   : ./utkeys -d ici -u toto -g toto -p 777
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputFile = 0;
  char* key = 0;
  char fingerprint[MAX_SIZE_HASH+1];
  int i;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"i:";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"input-key", required_argument, 0, 'i'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {					
					
    case 'i':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the input device\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((inputFile = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the input device path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(inputFile, optarg, strlen(optarg)+1);
      break;

      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (inputFile == 0) {
    usage(programName);
    goto error;	
  }
  
  // load the dsa public key
  if ((key = readPublicKey(inputFile)) == 0) goto error;
  if (!getFingerPrint(key, fingerprint)) goto error;

  // print the fingerprint
  printf("%c%c", fingerprint[0], fingerprint[1]);
  for (i=2; i<MAX_SIZE_HASH; i+=2)
    printf(":%c%c", fingerprint[i], fingerprint[i+1]);
  printf("\n");
  /************************************************************************/

  rc = TRUE;
 error:
  ENDINGS;
  rc=!rc;
 optError:
  if (inputFile) free(inputFile);
  if (key) free(key);
  exit(rc);
}

#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
