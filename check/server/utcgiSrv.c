/*=======================================================================
 * Version: $Id: utcgiSrv.c,v 1.1 2015/07/01 10:50:09 nroche Exp $
 * Project: MediaTeX
 * Module : cgi-server
 *
 * Unit test for cgi queries

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
#include "server/mediatex-server.h"
#include "server/utFunc.h"
GLOBAL_STRUCT_DEF;

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
  mdtxUsage(programName);
  fprintf(stderr, " [ -d repository ]");

  mdtxOptions();
  fprintf(stderr, "  ---\n"
	  "  -d, --input-rep\trepository with logo files\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2013/01/10
 * Description: Unit test for cache module.
 * Synopsis   : ./utcache
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
  Collection* coll = 0;
  Connexion connexion;
  RecordTree* tree = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"input-rep", required_argument, 0, 'd'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
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

      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll = mdtxGetCollection("coll3"))) goto error;
  connexion.sock = STDOUT_FILENO;
  if (!(tree = ask4logo(coll, 0))) goto error;

  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;
  
  utLog("%s", "1) try to extract the demand", 0);
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "2) retry with it on cache", 0);
  if (!utCopyFileOnCache(coll, inputRep, "logo.png")) goto error;
  if (!quickScan(coll)) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "3) retry with tgz on cache", 0);
  if (!utCleanCaches()) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tgz")) goto error;
  if (!quickScan(coll)) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "4) try with only part1 on cache", 0);
  if (!utCleanCaches()) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!quickScan(coll)) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "5) try with part2 added on cache too", 0);
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.cat")) goto error;
  if (!quickScan(coll)) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "6) register a mail", 0);
  if (!utCleanCaches()) goto error;
  if (!quickScan(coll)) goto error;
  tree = destroyRecordTree(tree);
  if (!(tree = ask4logo(coll, "test@test.com"))) goto error;
  if (!cgiServer(tree, &connexion)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;
  tree = destroyRecordTree(tree);
  /************************************************************************/

  rc = TRUE;
 error:
  freeConfiguration();
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