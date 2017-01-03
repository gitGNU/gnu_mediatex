/*=======================================================================
 * Project: mediaTeX
 * Module : configuration
 *
 * /etc configuration producer interface

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche
 
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
#include "utFunc.h"

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
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for confTree module.
 * Synopsis   : ./utconfTree -i
 * Input      : N/A
 * Output     : create the mediatex.conf file
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
  Configuration* conf = 0;
  Collection* coll = 0;
  char* string = 0;
  RGIT* curr = 0;
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
  // test building and serializing
  if (!createExempleConfiguration(inputRep)) goto error;  
  if (!serializeConfiguration(getConfiguration())) goto error;

  // test accessing the tree
  if (getCollection("coll0")) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!(coll = getCollection("coll2"))) goto error;
  if (!(coll = getCollection("coll3"))) goto error;
  if (!addCollection("coll4")) goto error;
  freeConfiguration();

  // Create 2 other configurations for later unit-tests based on
  // network topologie (utNotify, utExtract: todo). 
  // So we have:
  // - mdtx1 on "www" network (default)
  // - mdtx2 is the gateway for private1 network
  // - mdtx3 on "private1" network
  //
  // mdtx1.conf is the configuration file for server1,
  // mdtx2.conf for server2 and mdtx3.conf for server3.

  // Each server share together 3 collections.
  // Here, the collection's server keys are the sames for each server
  // on every collection. This should never append, but remains
  // possible. So, we can use the same servers.txt file everywhere.
  // So we alway have :
  // mdtx1's collection key: 746d6ceeb76e05cfa2dea92a1c5753cd
  // mdtx2's collection key: 6b18ed0194b0fbadd08e0a13cccda00e
  // mdtx3's collection key: bedac32422739d7eced624ba20f5912e

  // Note that the host key is the same for each server:
  // f3e8a2cfcf01e5779608583de63cdc42.
  // This is the case for instance during post-install test (tests.sh)
  // that runs 3 servers on the same host (using -c option).  

  env.confLabel="mdtx2";
  if (!createExempleConfiguration(inputRep)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!(string = addNetwork("www"))) goto error;
  if (!rgInsert(conf->networks, string)) goto error;
  if (!(string = addNetwork("private1"))) goto error;
  if (!rgInsert(conf->networks, string)) goto error;
  if (!rgInsert(conf->gateways, string)) goto error;

  // overwrite gateway on collection settings
  if (!(coll = getCollection("coll3"))) goto error;
  if (!(string = addNetwork("private2"))) goto error;
  if ((curr = rgHaveItem(coll->gateways, string))) {
    rgRemove_r(coll->gateways, &curr);
  }
  if (!(string = addNetwork("none"))) goto error;
  if (!rgInsert(coll->gateways, string)) goto error;

  if (!serializeConfiguration(getConfiguration())) goto error;
  freeConfiguration();

  env.confLabel="mdtx3";
  if (!createExempleConfiguration(inputRep)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!(string = addNetwork("private1"))) goto error;
  if (!rgInsert(conf->networks, string)) goto error;
  if (!serializeConfiguration(getConfiguration())) goto error;
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

