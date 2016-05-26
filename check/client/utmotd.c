/*=======================================================================
 * Project: MediaTeX
 * Module : motd
 *
 * Unit test for motd

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
 * modif      : 2012/11/05
 * Description: entry point for motd module
 * Synopsis   : ./utmotd
 * Input      : N/A
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = 0;
  Server* server = 0;
  Archive* archive = 0;
  char* string = 0;
  Record* record = 0;
  char inputRep[255] = "../misc";
  char path[1024];
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
  if (!(coll = mdtxGetCollection("coll1"))) goto error;
  
  // add a demand for logo
  if (!loadCollection(coll, CACH)) goto error;
  if (!(string = createString("!wanted"))) goto error;
  if (!(archive = addArchive(coll,
			     "b281449c229bcc4a3556cdcc0d3ebcec", 815)))
    goto error;
  if (!(server = getLocalHost(coll))) goto error;
  if (!(record = addRecord(coll, server, archive, DEMAND, string)))
    goto error;
  archive = 0;
  string = 0;
  if (!addCacheEntry(coll, record)) goto error;
  record = 0;

  // add a local supplies (to test alphabetic order)
  if (!(string = createString("zzz"))) goto error;
  if (!(archive = addArchive(coll,
			     "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", 42)))
    goto error;
  if (!(record = addRecord(coll, server, archive, SUPPLY, string)))
    goto error;
  archive = 0;
  string = 0;
  if (!addCacheEntry(coll, record)) goto error;
  record = 0;
  if (!(string = createString("aaa"))) goto error;
  if (!(archive = addArchive(coll,
			     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 42)))
    goto error;
  if (!(record = addRecord(coll, server, archive, SUPPLY, string)))
    goto error;
  archive = 0;
  string = 0;
  if (!addCacheEntry(coll, record)) goto error;
  record = 0;
 
  // test 1
  logMain(LOG_INFO, "** Test 1");
  if (!updateMotd()) goto error;

  // test 2: test alphabetic order on supports
  logMain(LOG_INFO, "** Test 2");
  sprintf(path, "%s/logoP1.iso", inputRep);
  if (!mdtxAddSupport("AAA", path)) goto error;
  if (!mdtxAddSupport("ZZZ", path)) goto error;
  if (!mdtxShareSupport("AAA", "coll1")) goto error; // set wrong order
  if (!mdtxShareSupport("ZZZ", "coll1")) goto error;
  if (!scoreLocalImages(coll)) goto error; // <- this perturb first test!
  if (!updateMotd()) goto error; 
  
  if (!releaseCollection(coll, CACH)) goto error;
  /************************************************************************/
  
  freeConfiguration();
  rc = TRUE;
 error:
  destroyString(string);
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
