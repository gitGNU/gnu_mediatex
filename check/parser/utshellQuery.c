/*=======================================================================
 * Version: $Id: utshellQuery.c,v 1.6 2015/10/20 19:41:47 nroche Exp $
 * Project: Mediatex
 * Module : shell scanner

 * shell scanner test

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
#include "parser/shellQuery.tab.h"
#include "parser/shellQuery.h"

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
  fprintf(stderr, " query");

  parserOptions();
  fprintf(stderr, "  ---\n"
	  "  query\t\t\tTo document here...\n");
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for confFile module.
 * Synopsis   : ./utshellQuery
 * Input      : command line parameters
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  void* buffer = 0;
  yyscan_t scanner;
  YYSTYPE values;
  int tokenVal = 0;
  char token[32];
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = PARSER_SHORT_OPTIONS;
  struct option longOptions[] = {
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

      GET_PARSER_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  // !!! remind to protect \" in cmdline
  if (!getCommandLine(argc, argv, optind)) goto error;

  shell_lex_init(&scanner);
  buffer = shell__scan_string(env.commandLine, scanner);

  shell_set_debug(env.debugLexer, scanner);
  logMain(LOG_DEBUG, "shell_set_debug = %i", shell_get_debug(scanner));
  
  // call scanner 
  do {
    tokenVal = shell_lex(&values, scanner);

    switch (tokenVal) {

    case shellSTRING:
      strcpy(token, "STRING");
      break;
    case shellNUMBER:
      strcpy(token, "NUMBER");
      break;
    case shellEOL:
      strcpy(token, "EOL");
      break;
    case shellCOLL:
      strcpy(token, "COLL");
      break;
    case shellSUPP:
      strcpy(token, "SUPP");
      break;
    case shellKEY:
      strcpy(token, "KEY");
      break;
    case shellUSER:
      strcpy(token, "USER");
      break;
    case shellADMIN:
      strcpy(token, "ADMIN");
      break;
    case shellSERVER:
      strcpy(token, "SERVER");
      break;
    case shellMASTER:
      strcpy(token, "MASTER");
      break;
    case shellALL:
      strcpy(token, "ALL");
      break;
    case shellTO:
      strcpy(token, "TO");
      break;
    case shellFROM:
      strcpy(token, "FROM");
      break;
    case shellAS:
      strcpy(token, "AS");
      break;
    case shellFOR:
      strcpy(token, "FOR");
      break;
    case shellON:
      strcpy(token, "ON");
      break;
    case shellINIT:
      strcpy(token, "INIT");
      break;
    case shellREMOVE:
      strcpy(token, "REMOVE");
      break;
    case shellPURGE:
      strcpy(token, "PURGE");
      break;
    case shellCLEAN:
      strcpy(token, "CLEAN");
      break;
    case shellADD:
      strcpy(token, "ADD");
      break;
    case shellDEL:
      strcpy(token, "DEL");
      break;
    case shellNOTE:
      strcpy(token, "NOTE");
      break;
    case shellLIST:
      strcpy(token, "LIST");
      break;
    case shellUPDATE:
      strcpy(token, "UPDATE");
      break;
    case shellUPGRADE:
      strcpy(token, "UPGRADE");
      break;
    case shellCOMMIT:
      strcpy(token, "COMMIT");
      break;
    case shellMAKE:
      strcpy(token, "MAKE");
      break;
    case shellCHECK:
      strcpy(token, "CHECK");
      break;
    case shellBIND:
      strcpy(token, "BIND");
      break;
    case shellUNBIND:
      strcpy(token, "UNBIND");
      break;
    case shellMOUNT:
      strcpy(token, "MOUNT");
      break;
    case shellUMOUNT:
      strcpy(token, "UMOUNT");
      break;
    case shellUPLOAD:
      strcpy(token, "UPLOAD");
      break;
    case shellGET:
      strcpy(token, "GET");
      break;
    case shellSU:
      strcpy(token, "SU");
      break;
    case shellMOTD:
      strcpy(token, "MOTD");
      break;
    case shellAUDIT:
      strcpy(token, "AUDIT");
      break;
    case shellDELIVER:
      strcpy(token, "DELIVER");
      break;
    case shellSAVE:
      strcpy(token, "SAVE");
      break;
    case shellEXTRACT:
      strcpy(token, "EXTRACT");
      break;
    case shellNOTIFY:
      strcpy(token, "NOTIFY");
      break;
    case shellAROBASE:
      strcpy(token, "AROBASE");
      break;
    case shellCOLON:
      strcpy(token, "COLON");
      break;
    case shellMINUS:
      strcpy(token, "MINUS");
      break;

    case 0:
      strcpy(token, "EOF");
      break;
    default:
      strcpy(token, "?UNKNOWN?");
      logMain(LOG_ERR, "%s ('%s')", token, shell_get_text(scanner));
      goto error;
    }

    logParser(LOG_INFO, "%s ('%s')", token, shell_get_text(scanner));

  } while (tokenVal);

  shell__delete_buffer(buffer, scanner);
  shell_lex_destroy(scanner);
  /************************************************************************/

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
