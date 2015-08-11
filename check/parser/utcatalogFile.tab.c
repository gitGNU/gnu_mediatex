/*=======================================================================
 * Version: $Id: utcatalogFile.tab.c,v 1.2 2015/08/11 18:14:23 nroche Exp $
 * Project: MediaTeX
 * Module : catalog parser
 *
 * test dor catalog file parser

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
 * modif      : 2012/05/01
 * Description: unitary test for this parser module
 * Synopsis   : ./utcatalogFile.tab
 * Input      : -i inputPath
 * Output     : Should display the same content
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = (Configuration*)conf;
  Collection* coll = 0;
  CvsFile fd = {0, 0, 0, FALSE, 0, cvsCutOpen, cvsCutPrint};
  char* inputPath = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = PARSER_SHORT_OPTIONS"i:";
  struct option longOptions[] = {
     {"input", required_argument, 0, 'i'},
    PARSER_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {

    case 'i':
      if (!(inputPath = malloc(strlen(optarg)+1))) {
	fprintf(stderr, "cannot allocate the input stream name\n");
	rc = 2;
      }
      strcpy(inputPath, optarg);
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

  if (!parseCatalogFile(coll, inputPath)) goto error;
  if (!serializeCatalogTree(coll, &fd)) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  freeConfiguration();
  free(inputPath);
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

/* Local Variables: */
/* mode: c *//* mode: font-lock */
/* mode: auto-fill */
/* End: */
