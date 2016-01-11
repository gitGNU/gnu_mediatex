/*=======================================================================
 * Version: $Id: utrecordFile.tab.c,v 1.5 2015/10/20 19:41:47 nroche Exp $
 * Project: MediaTeX
 * Module : record parser
 *
 * test for record file parser

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
  parserUsage(programName);
  fprintf(stderr, " [ -i inputPath ]");

  parserOptions();
  fprintf(stderr, "  ---\n"
	  "  -i, --input\t\tinput file to parse\n");
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/12/12
 * Description: unitary test for this parser module
 * Synopsis   : ./utrecordList.tab
 * Input      : stdin
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = (Configuration*)conf;
  Collection* coll = 0;
  RecordTree* rcTree = 0;
  char* inputPath = 0;
  char* outputPath = 0;
  int inputFd = -1;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = PARSER_SHORT_OPTIONS"i:o:";
  struct option longOptions[] = {
    {"input", required_argument, 0, 'i'},
    {"output", required_argument, 0, 'o'},
    PARSER_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;

  // disable stdout by default because we need no ending cariage
  // return for the diff
  env.dryRun = FALSE;

  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {

    case 'i':
      if (!(inputPath = malloc(strlen(optarg)+1))) {
	fprintf(stderr, "cannot allocate the input stream name\n");
	rc = 2;
      }
      strcpy(inputPath, optarg);
      break;

    case 'o':
      if (!(outputPath = malloc(strlen(optarg)+1))) {
	fprintf(stderr, "cannot allocate the input stream name\n");
	rc = 3;
      }
      strcpy(outputPath, optarg);
      break;

      GET_PARSER_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(conf = getConfiguration())) goto error;
  if (!parseConfiguration(conf->confFile)) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!expandCollection(coll)) goto error;
  if (!parseServerFile(coll, coll->serversDB)) goto error;

  if (!isEmptyString(inputPath)) {
    if ((inputFd = open(inputPath, O_RDONLY)) == -1) {
      logMain(LOG_ERR, "cannot open input file: %s", inputPath); 
      goto error;
    }
  }

  if (!(rcTree = parseRecords(inputFd))) goto error;
  
  // serialize the resulting tree :
  if (!outputPath) {
    // do not crypt on stdout
    rcTree->aes.fd = STDOUT_FILENO;
    rcTree->doCypher = FALSE; 
  }
  if (!aesInit(&rcTree->aes, "1234567890abcdef", ENCRYPT)) goto error;
  if (!serializeRecordTree(rcTree, outputPath, 0)) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  freeConfiguration();
  destroyRecordTree(rcTree);
  free(inputPath);
  free(outputPath);
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

 
