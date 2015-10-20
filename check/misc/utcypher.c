/*=======================================================================
 * Version: $Id: utcypher.c,v 1.8 2015/10/20 19:41:45 nroche Exp $
 * Project: MediaTeX
 * Module : cypher
 *
 * test for aes encryption

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

#include "mediatex.h"

extern int doCypher(AESData* data);


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
  miscOptions();
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for md5sum module
 * Synopsis   : ./utcommand -i scriptPath
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char key[MAX_SIZE_AES+1] = "1000000000000000";
  AESData data;
  int pipefd[2];
  char buf[512];
  int result = 0;
  int i = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /***********************************************************************/
  logMain(LOG_NOTICE, "*** low level functions");

  memcpy(data.mBlock, "top secret messg\0", 17);
  logMain(LOG_NOTICE, "message: %s", data.mBlock);

  if (!aesInit(&data, key, ENCRYPT)) goto error;
  if (!doCypher(&data)) goto error;
  logMain(LOG_NOTICE, "encrypt:  %s", data.cBlock);

  if (!aesInit(&data, key, DECRYPT)) goto error;
  if (!doCypher(&data)) goto error;
  logMain(LOG_NOTICE, "decrypt: %s", data.mBlock);

  // =================
  logMain(LOG_NOTICE, "***  encrypt API");

  if (!aesInit(&data, key, ENCRYPT)) goto error;
  //data.fd = STDOUT_FILENO;
  data.fd = open("/dev/null", O_WRONLY);

  aesPrint(&data, "%s", "# Collection's archives:\n");
  aesPrint(&data, "%s", "Headers\n"); 
  aesPrint(&data, "\tCollection\t%-*s\n", MAX_SIZE_COLL, "unit-test");
  aesPrint(&data, "\tDoCypher\t%s\n", "TRUE");
  aesFlush(&data);
  aesPrint(&data, "%s", "\nBody          \n");
  data.doCypher = TRUE;
  aesPrint(&data, "# %20s %15s %*s %*s %s\n",
	   "date", "host", 
	   MAX_SIZE_MD5, "hash", 
	   MAX_SIZE_SIZE, "size", 
	   "path");
  aesFlush(&data);

  // =====================
  logMain(LOG_NOTICE, "***  decrypt API");

  if (pipe(pipefd)) {
    logMain(LOG_NOTICE, "pipe fails: %s", strerror(errno));
    goto error;
  }

  if (!aesInit(&data, key, ENCRYPT)) goto error;
  data.fd = pipefd[1];

  aesPrint(&data, "%s", "# Collection's archives:\n");
  aesPrint(&data, "%s", "Headers\n"); 
  aesPrint(&data, "\tCollection\t%-*s\n", MAX_SIZE_COLL, "unit-test");
  aesPrint(&data, "\tDoCypher\t%s\n", "TRUE");
  aesFlush(&data);
  aesPrint(&data, "%s", "\nBody          \n");
  data.doCypher = TRUE;
  aesPrint(&data, "# %20s %15s %*s %*s %s\n",
	   "date", "host", 
	   MAX_SIZE_MD5, "hash", 
	   MAX_SIZE_SIZE, "size", 
	   "path");
  aesFlush(&data);

  close (pipefd[1]);
  if (!aesInit(&data, key, DECRYPT)) goto error;
  data.fd = pipefd[0];
 
  // because we know here we have 7*16 byte of header here
  for (i=0; i<7; ++i) {
    if (!aesInput(&data, buf, &result, 1024)) goto error;
    buf[result] = (char)0;
    printf("%s", buf);
  };
  data.doCypher = TRUE;
  do {
    if (!aesInput(&data, buf, &result, 1024)) goto error;
    buf[result] = (char)0;
    printf("%s", buf);
  } while (result > 0);
  
  close (pipefd[0]);
  /***********************************************************************/
  
  rc = TRUE;
 error:
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
