
/*=======================================================================
 * Version: $Id: utupload.c,v 1.3 2015/08/11 11:59:33 nroche Exp $
 * Project: MediaTeX
 * Module : conf
 *
 * unit test for conf

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
#include "client/mediatex-client.h"
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
  fprintf(stderr, 
	  "\n\t\t[ -C catalog ] [ -E extract ]\n"
	  "\t\t[ -F file ] [ -T targetPath ]");

  mdtxOptions();
  fprintf(stderr, 
	  "  ---\n"
	  "  -C, --catalog\tcatalog metadata path\n"
	  "  -E, --extract\textract metadata path\n"
	  "  -F, --file\tfile to upload\n"
	  "  -P, --target-path\tpath where to upload in cache\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2010/12/10
 * Description: entry point for conf module
 * Synopsis   : ./utconf
 * Input      : N/A
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* catalog = 0;
  char* extract = 0;
  char* file = 0;
  char* targetPath = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS "C:E:F:T:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"catalog", required_argument, 0, 'C'},
    {"extract", required_argument, 0, 'E'},
    {"file", required_argument, 0, 'F'},
    {"target-path", required_argument, 0, 'T'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'C':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the catalog\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((catalog = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the catalog path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(catalog, optarg, strlen(optarg)+1);
      break;

    case 'E':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the extract\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((extract = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the extract path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(extract, optarg, strlen(optarg)+1);
      break;

    case 'F':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the file\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((file = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the file path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(file, optarg, strlen(optarg)+1);
      break;

    case 'T':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the targetPath\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((targetPath = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the targetPath path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      strncpy(targetPath, optarg, strlen(optarg)+1);
      break;

      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  logMain(LOG_NOTICE, "*** Upload: ");
  if (!mdtxUpload("coll2", catalog)) goto error;
  /************************************************************************/
  
  rc = TRUE;
 error:
  freeConfiguration();
  destroyString(catalog);
  destroyString(extract);
  destroyString(file);
  destroyString(targetPath);
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
