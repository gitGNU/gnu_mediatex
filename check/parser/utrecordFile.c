/*=======================================================================
 * Version: $Id: utrecordFile.c,v 1.5 2015/10/20 19:41:47 nroche Exp $
 * Project: Mediatex
 * Module : record scanner

 * record scanner test

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
#include "parser/recordFile.tab.h"
#include "parser/recordFile.h"

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
 * Description: Unit test for recordList module.
 * Synopsis   : ./utrecordList
 * Input      : stdin
 * Output     : stdout

 * Note       : cannot scan cryted input as key is loaded by parser
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputPath = 0;
  int inputFd = STDIN_FILENO;
  AESData aes;
  RecordExtra extra;
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
  // init scanner
  if (record_lex_init (&scanner)) {
    logMain(LOG_ERR, "error initializing scanner");
    goto error;
  }
  record_set_debug(env.debugLexer, scanner);
  logMain(LOG_DEBUG, "record_set_debug = %i", 
	  record_get_debug(scanner));
  
  if (inputPath) {
    if ((inputFd = open(inputPath, O_RDONLY)) == 0) {
      logMain(LOG_ERR, "cannot open input file: %s", inputPath); 
      goto error;
    }
  }
  
  //in GDB: print *(RecordExtra*)yyg->yyextra_r
  extra.aesData = &aes;
  extra.aesData->fd = inputFd;
  extra.aesData->way = DECRYPT;
  aesInit(extra.aesData, "1234567890abcdef", DECRYPT);
  record_set_extra (&extra, scanner); 

  // call scanner 
  do {
    tokenVal = record_lex(&values, scanner);

    switch (tokenVal) {

    case recordHEADER:
      strcpy(token, "HEADER");
      break;
    case recordBODY:
      strcpy(token, "BODY");
      break;
    case recordCOLL:
      strcpy(token, "COLL");
      break;
    case recordDOCYPHER:
      strcpy(token, "DOCYPHER");
      break;
    case recordSERVER:
      strcpy(token, "SERVER");
      break;
    case recordMSGTYPE:
      strcpy(token, "MSGTYPE");
      break;
    case recordSTRING:
      strcpy(token, "STRING");
      break;
    case recordTYPE:
      strcpy(token, "TYPE");
      break;
    case recordDATE:
      strcpy(token, "DATE");
      break;
    case recordHASH:
      strcpy(token, "HASH");
      break;
    case recordSIZE:
      strcpy(token, "SIZE");
      break;
    case recordPATH:
      strcpy(token, "PATH");
      break;
    case recordMSGVAL:
      strcpy(token, "MSGVAL");
      break;
    case recordERROR:
      strcpy(token, "!ERROR!");
      logMain(LOG_ERR, "%s (line %i: '%s')", token,
	      record_get_lineno(scanner), record_get_text(scanner));
      goto error;

    case 0:
      strcpy(token, "EOF");
      break;
    default:
      strcpy(token, "?UNKNOWN?");
      logMain(LOG_ERR, "%s (line %i: '%s')", token,
	      record_get_lineno(scanner), record_get_text(scanner));
      goto error;
    }

    logParser(LOG_INFO, "%s (line %i: '%s')", token,
	      record_get_lineno(scanner), record_get_text(scanner));

  } while (tokenVal);

  record_lex_destroy(scanner);
  //close(inputFd); ?
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

