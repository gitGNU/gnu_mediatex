/*=======================================================================
 * Version: $Id: utextract.c,v 1.5 2015/09/04 15:30:23 nroche Exp $
 * Project: MediaTeX
 * Module : extract
 *
 * Unit test for extract

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
	  "  -d, --input-rep\tsrcdir directory for make distcheck\n");
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

      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  env.dryRun = FALSE;
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll = mdtxGetCollection("coll3"))) goto error;

  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;
  
  utLog("%s", "add a demand for logo.png:", 0);
  if (!utAddFinalDemand(coll)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "try to extract the demand (1)", 0);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);
  
  utLog("%s", "Put containers files:", 0);
  if (!utCopyFileOnCache(coll, inputRep, "logoP1.cat")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logoP2.cat")) goto error;
  
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan gives :", coll);
  
  utLog("%s", "try to extract the demand (2)", 0);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  /********************* other containers ********************/

  utLog("%s", "Try tar-bzip2 container:", 0);
  if (!utCleanCaches()) goto error;
  if (!utAddFinalDemand(coll)) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tar.bz2")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before tar-bzip:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  /* utLog("%s", "Try afio container:", 0); */
  /* if (!utCleanCaches()) goto error; */
  /* if (!utAddFinalDemand(coll)) goto error; */
  /* if (!utCopyFileOnCache(coll, inputRep, "logo.afio")) goto error; */
  /* if (!quickScan(coll)) goto error; */
  /* utLog("%s", "Scan before afio:", coll); */
  /* if (!extractArchives(coll)) goto error; */
  /* utLog("%s", "Now we have :", coll); */

  utLog("%s", "Try zip container:", 0);
  if (!utCleanCaches()) goto error;
  if (!utAddFinalDemand(coll)) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.zip")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before unzip:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  /* utLog("%s", "Try rar container:", 0); */
  /* if (!utCleanCaches()) goto error; */
  /* if (!utAddFinalDemand(coll)) goto error; */
  /* if (!utCopyFileOnCache(coll,inputRep,"logo.part1.rar")) goto error; */
  /* if (!utCopyFileOnCache(coll,inputRep,"logo.part2.rar")) goto error; */
  /* if (!quickScan(coll)) goto error; */
  /* utLog("%s", "Scan before unrar:", coll); */
  /* if (!extractArchives(coll)) goto error; */
  /* utLog("%s", "Now we have :", coll); */

  utLog("%s", "Try tar container:", 0);
  if (!utCleanCaches()) goto error;
    if (!utAddFinalDemand(coll)) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.tar")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before tar:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  utLog("%s", "Try cpio+gz container:", 0);
  if (!utCleanCaches()) goto error;
  if (!utAddFinalDemand(coll)) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.cpio.gz")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before tar:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  utLog("%s", "Try cpio+bz2 container:", 0);
  if (!utCleanCaches()) goto error;
  if (!utAddFinalDemand(coll)) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.cpio.bz2")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Scan before tar:", coll);
  if (!extractArchives(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;
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
