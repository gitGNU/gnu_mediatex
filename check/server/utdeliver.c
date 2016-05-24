/*=======================================================================
 * Project: MediaTeX
 * Module : deliver
 *
 * Unit test for deliver

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
#include "server/mediatex-server.h"
#include "server/utFunc.h"

/*=======================================================================
 * Function   : deliverMails
 * Description: Deliver notifications to users that have subscribe for
 *              a file to download
 * Synopsis   : int deliverMails(Collection* coll)
 * Input      : Collection* coll
 *              RecordTree* records
 * Output     : TRUE on success
 =======================================================================*/
int
deliverMails(Collection* coll)
{
  int rc = FALSE;
  Configuration* conf = 0;
  Archive* archive = 0;
  char* path = 0;
  RGIT* curr = 0;

  logMain(LOG_DEBUG, "delivering %s collection files to users",
	  coll->label);

  if (!(conf = getConfiguration())) goto error;
  if (!loadCollection(coll, CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;

  // for each cache entry
  while ((archive = rgNext_r(coll->cacheTree->archives, &curr))
	!= 0) {

    // look if archive is supplyed
    if (archive->localSupply == 0) continue;

    // test if the file is really there
    path = destroyString(path);
    if (!(path = createString(coll->cacheDir))) goto error3;
    if (!(path = catString(path, "/"))) goto error3;
    if (!(path = catString(path, archive->localSupply->extra)))
      goto error3;

    if (access(path, R_OK) == -1) {
      logMain(LOG_WARNING,
	      "file not find in cache as expected: %s", path);
      continue;
    }

    // send messages
    if (!deliverArchive(coll, archive)) goto error3;
  }

  rc = TRUE;
 error3:
  if (!unLockCache(coll)) goto error;
 error2:
  if (!releaseCollection(coll, CACH)) goto error;
 error:
  if (!rc) {
    logMain(LOG_DEBUG, "deliverMails fails");
  }
  path = destroyString(path);
  return rc;
}

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

  mdtxOptions();
  //fprintf(stderr, "  ---\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2010/12/10
 * Description: entry point for mdtx-env
 * Synopsis   : mdtx-env
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
  env = envUnitTest;
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
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll = mdtxGetCollection("coll3"))) goto error;

  utLog("%s", "Clean the cache:", 0);
  if (!utCleanCaches()) goto error;

  utLog("%s", "add a demand for logo.png:", 0);
  if (!utAddLocalDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075, 
			"nroche@narval.tk")) goto error;
  if (!utCopyFileOnCache(coll, inputRep, "logo.png")) goto error;
  if (!quickScan(coll)) goto error;
  utLog("%s", "Now we have :", coll);

  utLog("%s", "deliver the mail:", 0);
  if (!deliverMails(coll)) goto error;
  utLog("%s", "Finaly we have :", coll);

  if (!utCleanCaches()) goto error;
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
