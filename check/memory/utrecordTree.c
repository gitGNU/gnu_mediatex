/*=======================================================================
 * Project: MediaTeX
 * Module : recordTree
 *
 * recordTree producer interface
 *
 * Note: records physically belongs to
 * - coll->cacheTree->recordTree for local records
 * - server->recordTree for remote records

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

#include "mediatex.h"
#include "memory/utFunc.h"

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually argv[0].
 * Output     : N/A
 =======================================================================*/
static void 
usage(char* programName)
{
  memoryUsage(programName);
  fprintf(stderr, " [ -d repository ]");

  memoryOptions();
  fprintf(stderr, "  ---\n"
	  "  -d, --input-rep\tsrcdir directory for make distcheck\n");
  return;
}

/*=======================================================================
 * Function   : main
 * Author     : Nicolas Roche
 * modif      : 2012/02/11
 * Description: Unit test for md5sumTree module.
 * Synopsis   : utmd5sumTree
 * Input      : N/A
 * Output     : /tmp/mdtx/var/local/cache/mdtx/md5sums.txt
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
  Collection* coll = 0;
  RecordTree* tree = 0;
  char key[MAX_SIZE_AES+1] = "1234567890abcdef";
  AESData* aes = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MEMORY_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MEMORY_LONG_OPTIONS,
    {"input-rep", required_argument, 0, 'd'},
    {0, 0, 0, 0}
  };
       
  // import mdtx environment
  env = envUnitTest;
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'd':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input repository\n",
		programName);
	rc = EINVAL;
	break;
      }
      strncpy(inputRep, optarg, strlen(optarg)+1);
      break; 

      GET_MEMORY_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!createExempleConfiguration(inputRep)) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!(tree = createExempleRecordTree(coll))) goto error;
  
  aes = &tree->aes;
  if (!aesInit(aes, key, ENCRYPT)) goto error;
  if (!aesInit(aes, key, ENCRYPT)) goto error;

  // serialize uncrypted file
  if (!serializeRecordTree(tree, coll->md5sumsDB, 0)) goto error;

  // serialize crypted file
  strncpy(coll->md5sumsDB + strlen(coll->md5sumsDB) -3, "aes", 3);
  tree->doCypher = TRUE;
  if (!serializeRecordTree(tree, coll->md5sumsDB, 0)) goto error;
  
  aes->doCypher = FALSE;
  aes->fd = STDOUT_FILENO;
  
  if (!(diseaseRecordTree(tree))) goto error;
  if (tree->records->nbItems > 0) goto error;
  tree = destroyRecordTree(tree);
  /************************************************************************/

  freeConfiguration();
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
