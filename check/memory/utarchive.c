/*=======================================================================
 * Version: $Id: utarchive.c,v 1.5 2015/09/04 15:30:17 nroche Exp $
 * Project: MediaTeX
 * Module : archive
 *
 * archive producer interface

 MediaTex is an Electronic Archives Management System
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
#include "memory/utFunc.h"
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
  Archive* arch1 = 0;
  Archive* arch2 = 0;
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
  // test types on this architecture:
  off_t offset = 0xFFFFFFFFFFFFFFFFULL; // 2^64
  time_t timer = 0x7FFFFFFF; // 2^32
  logMemory(LOG_DEBUG, "type off_t is store into %u bytes", 
	  (unsigned int)sizeof(off_t));
  logMemory(LOG_DEBUG, "type time_t is store into %u bytes", 
	  (unsigned int) sizeof(time_t));
  logMemory(LOG_DEBUG,  "off_t max value is: %llu", 
	  (unsigned long long int)offset);
  logMemory(LOG_DEBUG, "time_t max value is: %lu", 
	  (unsigned long int)timer);

  if (!createExempleConfiguration(inputRep)) goto error;
  if (!(coll = getCollection("coll1"))) goto error;

  // creating an archive 2 times return the same object
  if (!(arch1 =
  	addArchive(coll, "780968014038afcbae5c6feca2e630de", 81)))
    goto error;
  if (!(arch2 =
  	addArchive(coll, "780968014038afcbae5c6feca2e630de", 81)))
    goto error;
  if (arch1 != arch2) goto error;
  
  // search an archive
  if (getArchive(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 22222))
    goto error;
  if (!(arch1 =
  	addArchive(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 22222)))
    goto error;
  if (getArchive(coll, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 22222) != arch1)
    goto error;

  if (avl_count(coll->archives) != 2) goto error;
  if (!(diseaseArchives(coll))) goto error;
  if (avl_count(coll->archives)) goto error;
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
