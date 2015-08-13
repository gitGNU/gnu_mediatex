/*=======================================================================
 * Version: $Id: utserverFile.c,v 1.4 2015/08/13 21:14:31 nroche Exp $
 * Project: Mediatex
 * Module : server scanner

 * server scanner test

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
#include "parser/serverFile.tab.h"
#include "parser/serverFile.h"
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
 * modif      : 2010/11/09
 * Description: module's unitary test
 * Synopsis   : ./utServerFile
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
  if (serv_lex_init(&scanner)) {
    logMain(LOG_ERR, "error initializing scanner");
    goto error;
  }
  
  serv_set_debug(env.debugLexer, scanner);
  logMain(LOG_DEBUG, "serv_set_debug = %i", serv_get_debug(scanner));
  
  if (inputPath) {
    if ((inputStream = fopen(inputPath, "r")) == 0) {
      logMain(LOG_ERR, "cannot open input stream: %s", inputPath); 
      goto error;
    }
  }
  
  serv_set_in(inputStream, scanner);
  
  // call scanner 
  do {
    tokenVal = serv_lex(&values, scanner);

    switch (tokenVal) {
    case servMASTER:
      strcpy(token, "MASTER");
      break;
    case servSERVER:
      strcpy(token, "SERVER");
      break;
    case servCOMMENT:
      strcpy(token, "COMMENT");
      break;
    case servLABEL:
      strcpy(token, "LABEL");
      break;
    case servHOST:
      strcpy(token, "HOST");
      break;
    case servNETWORKS:
      strcpy(token, "NETWORKS");
      break;
    case servGATEWAYS:
      strcpy(token, "GATEWAYS");
      break;
    case servMDTXPORT:
      strcpy(token, "MDTXPORT");
      break;
    case servSSHPORT:
      strcpy(token, "SSHPORT");
      break;
    case servCOLLKEY:
      strcpy(token, "COLLKEY");
      break;
    case servUSERKEY:
      strcpy(token, "USERKEY");
      break;
    case servHOSTKEY:
      strcpy(token, "HOSTKEY");
      break;
    case servPROVIDE:
      strcpy(token, "PROVIDE");
      break;
    case servCOMMA:
      strcpy(token, "COMMA");
      break;
    case servCOLON:
      strcpy(token, "COLON");
      break;
    case servEQUAL:
      strcpy(token, "EQUAL");
      break;
    case servENDBLOCK:
      strcpy(token, "ENDBLOCK");
      break;
    case servCACHESIZE:
      strcpy(token, "CACHESIZE");
      break;
    case servCACHETTL:
      strcpy(token, "CACHETTL");
      break;
    case servQUERYTTL:
      strcpy(token, "QUERYTTL");
      break;
    case servSUPPTTL:
      strcpy(token, "SUPPTTL");
      break;
    case servMAXSCORE:
      strcpy(token, "MAXSCORE");
      break;
    case servBADSCORE:
      strcpy(token, "BADSCORE");
      break;
    case servPOWSUPP:
      strcpy(token, "POWSUPP");
      break;
    case servFACTSUPP:
      strcpy(token, "FACTSUPP");
      break;
    case servMINGEODUP:
      strcpy(token, "MINGEODUP");
      break;
    case servSTRING:
      strcpy(token, "STRING");
      break;
    case servNUMBER:
      strcpy(token, "NUMBER");
      break;
    case servSCORE:
      strcpy(token, "SCORE");
      break;
    case servHASH:
      strcpy(token, "HASH");
      break;
    case servSIZE:
      strcpy(token, "SIZE");
      break;
    case servTIME:
      strcpy(token, "TIME");
      break;

    case 0:
      strcpy(token, "EOF");
      break;
    default:
      strcpy(token, "?UNKNOWN?");
      logMain(LOG_ERR, "%s (line %i: '%s')", token,
	      serv_get_lineno(scanner), serv_get_text(scanner));
      goto error;
    }

    logParser(LOG_INFO, "%s (line %i: '%s')", token,
	      serv_get_lineno(scanner), serv_get_text(scanner));
  
  } while (tokenVal);

  serv_lex_destroy(scanner);
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
