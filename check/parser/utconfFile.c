/*=======================================================================
 * Version: $Id: utconfFile.c,v 1.4 2015/08/13 21:14:31 nroche Exp $
 * Project: Mediatex
 * Module : conf scanner

 * configuration scanner test

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
#include "parser/confFile.tab.h"
#include "parser/confFile.h"
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
 * Description: Unit test for confFile module.
 * Synopsis   : utconfFile
 * Input      : N/A
 * Output     : N/A
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
  if (conf_lex_init(&scanner)) {
    logMain(LOG_ERR, "error initializing scanner");
    goto error;
  }

  conf_set_debug(env.debugLexer, scanner);
  logMain(LOG_DEBUG, "conf_set_debug = %i", conf_get_debug(scanner));
  
  if (inputPath) {
    if ((inputStream = fopen(inputPath, "r")) == 0) {
      logMain(LOG_ERR, "cannot open input stream: %s", inputPath); 
      goto error;
    }
  }

  conf_set_in(inputStream, scanner);

  // call scanner 
  do {
    tokenVal = conf_lex(&values, scanner);

    switch (tokenVal) {
    case confHOST:
      strcpy(token, "HOST");
      break;
     case confNETWORKS:
      strcpy(token, "NETWORKS");
      break;
    case confGATEWAYS:
      strcpy(token, "GATEWAYS");
      break;
    case confMDTXPORT:
      strcpy(token, "MDTXPORT");
      break;
    case confSSHPORT:
      strcpy(token, "SSHPORT");
      break;
    case confCACHESIZE:
      strcpy(token, "CACHESIZE");
      break;
    case confCACHETTL:
      strcpy(token, "CACHETTL");
      break;
     case confQUERYTTL:
      strcpy(token, "QUERYTTL");
      break;
    case confCHECKTTL:
      strcpy(token, "CHECKTTL");
      break;
    case confSUPPTTL:
      strcpy(token, "SUPPTTL");
      break;
    case confMAXSCORE:
      strcpy(token, "MAXSCORE");
      break;
    case confBADSCORE:
      strcpy(token, "BADSCORE");
      break;
    case confPOWSUPP:
      strcpy(token, "POWSUPP");
      break;
     case confPOWIMAGE:
      strcpy(token, "POWIMAGE");
      break;
    case confFACTSUPP:
      strcpy(token, "FACTSUPP");
      break;
    case confCOLL:
      strcpy(token, "COLL");
      break;
    case confCOMMENT:
      strcpy(token, "COMMENT");
      break;
    case confSHARE:
      strcpy(token, "SHARE");
      break;
    case confLOCALHOST:
      strcpy(token, "LOCALHOST");
      break;
     case confCOMMA:
      strcpy(token, "COMMA");
      break;
    case confDOT:
      strcpy(token, "DOT");
      break;
    case confCOLON:
      strcpy(token, "COLON");
      break;
    case confAROBASE:
      strcpy(token, "AROBASE");
      break;
    case confMINUS:
      strcpy(token, "MINUS");
      break;
    case confENDBLOCK:
      strcpy(token, "ENDBLOCK");
      break;
     case confSTRING:
      strcpy(token, "STRING");
      break;
    case confNUMBER:
      strcpy(token, "NUMBER");
      break;
    case confSCORE:
      strcpy(token, "SCORE");
      break;
    case confSIZE:
      strcpy(token, "SIZE");
      break;
    case confTIME:
      strcpy(token, "TIME");
      break;

    case 0:
      strcpy(token, "EOF");
      break;
    default:
      strcpy(token, "?UNKNOWN?");
      logMain(LOG_ERR, "%s (line %i: '%s')", token,
	      conf_get_lineno(scanner), conf_get_text(scanner));
      goto error;
    }

    logParser(LOG_INFO, "%s (line %i: '%s')", token,
	      conf_get_lineno(scanner), conf_get_text(scanner));

  } while (tokenVal);

  conf_lex_destroy(scanner);
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
