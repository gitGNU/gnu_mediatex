/*=======================================================================
 * Project: MediaTeX
 * Module : supp
 *
 * Unit test for supp

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
#include "client/mediatex-client.h"

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
 * modif      : 2010/12/10
 * Description: entry point for supp module
 * Synopsis   : ./utsupp
 * Input      : N/A
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[255] = "../../examples";
  char* supp = "SUPP21_logo.part1";
  char path[1024];
  Configuration* conf = 0;
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
  if (!(conf = getConfiguration())) goto error;
  
  logMain(LOG_NOTICE, "*** Start like this:");
  conf->checkTTL = 946080000;
  if (!mdtxLsSupport()) goto error;
  conf->fileState[iSUPP] = MODIFIED;
  if (!saveConfiguration("topo")) goto error;
  
  logMain(LOG_NOTICE, "*** Add a support:");
  sprintf(path, "%s/logo.tgz", inputRep);
  if (!mdtxAddSupport("me", path)) goto error;
  if (!saveConfiguration("topo")) goto error;

  logMain(LOG_NOTICE, "*** Add a file:");
  sprintf(path, "%s/logo.tgz", inputRep);
  if (!mdtxAddFile(path)) goto error;
  if (!saveConfiguration("topo")) goto error;

  logMain(LOG_NOTICE, "*** List supports:");
  if (!mdtxLsSupport()) goto error;

  logMain(LOG_NOTICE, "*** Add support a second time:");
  if (mdtxAddSupport("me", path)) goto error;
  if (!saveConfiguration("topo")) goto error;

  logMain(LOG_NOTICE, "*** Add file a second time:");
  sprintf(path, "%s/logo.tgz", inputRep);
  if (mdtxAddFile(path)) goto error;

  logMain(LOG_NOTICE, "*** Add support with name begining with '/':");
  if (mdtxAddSupport("/me", path)) goto error;

  logMain(LOG_NOTICE, "*** Have a bad support:");
  sprintf(path, "%s/logo.png", inputRep);
  if (mdtxHaveSupport(supp, path)) goto error;

  logMain(LOG_NOTICE, "*** Have a file:");
  if (mdtxHaveSupport("/me", path)) goto error;
  
  logMain(LOG_NOTICE, "*** Have a support:");
  sprintf(path, "%s/logoP1.iso", inputRep);
  if (!mdtxHaveSupport(supp, path)) goto error;
  
  logMain(LOG_NOTICE, "*** Update a support:");
  if (!mdtxUpdateSupport(supp, "too much caracteres => troncated")) 
    goto error;
  if (!saveConfiguration("topo")) goto error;

  logMain(LOG_NOTICE, "*** Remove a support:");
  if (!mdtxDelSupport(supp)) goto error;
  if (!saveConfiguration("topo")) goto error;
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
