/*=======================================================================
 * Project: Mediatex
 * Module : catalog scanner

 * catalog scanner test

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
#include "parser/catalogFile.tab.h"
#include "parser/catalogFile.h"

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
 * Description: Catalog lexer unit test
 * Synopsis   : ./utcatalogFile
 * Input      : stdin
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputPath = 0;
  FILE* inputStream = stdin;
  yyscan_t scanner;
  YYSTYPE values;
  int tokenVal = 0;
  char token[32];
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
  env = envUnitTest;
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
		
      GET_PARSER_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (cata_lex_init(&scanner)) {
    logMain(LOG_ERR, "error initializing scanner");
    goto error;
  }

  cata_set_debug(env.debugLexer, scanner);
  logMain(LOG_DEBUG, "cata_set_debug = %i", cata_get_debug(scanner));
  
  if (inputPath) {
    if ((inputStream = fopen(inputPath, "r")) == 0) {
      logMain(LOG_ERR, "cannot open input stream: %s", inputPath); 
      goto error;
    }
  }

  cata_set_in(inputStream, scanner);

  // call scanner 
  do {
    tokenVal = cata_lex(&values, scanner);

    switch (tokenVal) {

    case cataTOP:
      strcpy(token, "TOP");
      break;
    case cataCATEGORY:
      strcpy(token, "CATEGORY");
      break;
    case cataDOCUMENT:
      strcpy(token, "DOCUMENT");
      break;
    case cataHUMAN:
      strcpy(token, "HUMAN");
      break;
    case cataWITH:
      strcpy(token, "WITH");
      break;
    case cataARCHIVE:
      strcpy(token, "ARCHIVE");
      break;
    case cataEQUAL:
      strcpy(token, "EQUAL");
      break;
    case cataCOLON:
      strcpy(token, "COLON");
      break;
    case cataCOMMA:
      strcpy(token, "COMMA");
      break;
    case cataHASH:
      strcpy(token, "HASH");
      break;
    case cataNUMBER:
      strcpy(token, "NUMBER");
      break;
    case cataSTRING:
      strcpy(token, "STRING");
      break;

    case 0:
      strcpy(token, "EOF");
      break;
    default:
      strcpy(token, "?UNKNOWN?");
      logMain(LOG_ERR, "%s (line %i: '%s')", token,
	      cata_get_lineno(scanner), cata_get_text(scanner));
      goto error;
    }

    logParser(LOG_INFO, "%s (line %i: '%s')", token,
	      cata_get_lineno(scanner), cata_get_text(scanner));

  } while (tokenVal);

  cata_lex_destroy(scanner);
  fclose(inputStream);
  /************************************************************************/

  free(inputPath);
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
