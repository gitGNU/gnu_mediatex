/* prologue: =======================================================*/
%{
  /*=======================================================================
   * Version: $Id: confFile.y,v 1.1 2014/10/13 19:39:48 nroche Exp $
   * Project: Mediatex
   * Module : conf parser
   *
   * configuration parser.

   MediaTex is an Electronic Records Management System
   Copyright (C) 2014  Nicolas Roche
   
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

#include "../../config.h"
#include "../mediatex.h"
#include "../misc/log.h"
#include "../misc/locks.h"
#include "../memory/confTree.h"
#include "confFile.h"
  
void conf_error(const char* message);

// scanner + anything we may want
// (parameter type of reentrant 'yyparse' function)
typedef struct ConfBisonSelf {
  yyscan_t       scanner;
  // ... anything we want here
  Collection*    coll;
} ConfBisonSelf;

// name of the belows parameter into bison: must use a common name
// define YYLEX yylex (&yylval, YYLEX_PARAM)
#define YYPARSE_PARAM confBisonSelf
#define YYLEX_PARAM   ((ConfBisonSelf*)confBisonSelf)->scanner

// shortcut maccros
#define CONF_LINE_NO ((ConfExtra*)conf_get_extra(YYLEX_PARAM))->lineNo
#define CONF_COLL    ((ConfBisonSelf*)confBisonSelf)->coll
%}

%{
  // data initialisation
%}

%code provides {
  #include "../memory/confTree.h"
  int parseConfiguration(const char* path);
}

/* declarations: ===================================================*/
%file-prefix="confFile"
%defines
%debug
%name-prefix="conf_"
%error-verbose
%verbose
%define api.pure

%start stanzas

%token          confHOST
%token          confNETWORKS
%token          confGATEWAYS
%token          confMDTXPORT
%token          confSSHPORT
%token          confCACHESIZE
%token          confCACHETTL
%token          confQUERYTTL
%token          confCHECKTTL
%token          confSUPPTTL
%token          confMAXSCORE
%token          confBADSCORE
%token          confPOWSUPP
%token          confPOWIMAGE
%token          confFACTSUPP
%token          confCOLL
%token          confCOMMENT
%token          confSHARE
%token          confLOCALHOST
%token          confCOMMA
%token          confDOT
%token          confCOLON
%token          confAROBASE
%token          confMINUS
%token          confENDBLOCK

%token <string> confERROR
%token <string> confSTRING
%token <number> confNUMBER
%token <score>  confSCORE
%token <size>   confSIZE
%token <time>   confTIME
  
%%

   /* grammar rules: ==================================================*/
   
stanzas: stanzas confLine
       | stanzas collectionStanza
       | confLine
       | collectionStanza
       | confERROR {YYABORT;}
;
 
collectionStanza: newCollection collectionLines confENDBLOCK
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, 
	    "collectionStanza: newCollection collectionLines confENDBLOCK");
  CONF_COLL = NULL;
}
;

newCollection: confCOLL confSTRING confMINUS confSTRING confAROBASE confSTRING confCOLON confNUMBER
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, 
	    "newCollection: confCOLL confSTRING confAROBASE confSTRING"
	    " confCOLON confNUMBER");
  if (!(CONF_COLL = addCollection($4))) YYERROR;
  CONF_COLL->masterLabel = $2;
  strncpy(CONF_COLL->masterHost, $6, MAX_SIZE_HOST);
  CONF_COLL->masterPort = $8;
  destroyString($4);
  destroyString($6);
}
;

collectionLines: collectionLines collectionLine
               | collectionLine
;

collectionLine: confSHARE supports
              | confLOCALHOST confSTRING
{
  logParser(LOG_DEBUG, "line %-3i localhost = %s", CONF_LINE_NO, $2);
  strncpy(CONF_COLL->userFingerPrint, $2, MAX_SIZE_HASH+1);
  destroyString($2);
}
              | confNETWORKS cnetworks
              | confGATEWAYS cgateways
              | confCACHESIZE confNUMBER confSIZE
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "cache size");
  CONF_COLL->cacheSize = $2*$3;
}
              | confCACHETTL confNUMBER confTIME
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "cache TTL");
  CONF_COLL->cacheTTL = $2*$3;
}
              | confQUERYTTL confNUMBER confTIME
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "query TTL");
  CONF_COLL->queryTTL = $2*$3;
}
;

supports: supports confCOMMA support
        | support
;

support: confSTRING
{
  Support* supp = NULL;
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "support");
  if (!(supp = addSupport($1))) YYERROR;
  if (!addSupportToCollection(supp, CONF_COLL)) YYERROR;
  $1 = destroyString($1);
}
;

confLine: confHOST confSTRING
{
  logParser(LOG_DEBUG, "line %-3i host = %s", CONF_LINE_NO, $2);
  strncpy(getConfiguration()->host, $2, MAX_SIZE_HOST);
  destroyString($2);
}
       | confNETWORKS Cnetworks
{
  logParser(LOG_DEBUG, "line %-3i networks", CONF_LINE_NO);
}
       | confGATEWAYS Cgateways
{
  logParser(LOG_DEBUG, "line %-3i gateways", CONF_LINE_NO);
}
       | confCOMMENT confSTRING
{
  logParser(LOG_DEBUG, "line %-3i comment = %s", CONF_LINE_NO, $2);
  destroyString(getConfiguration()->comment);
  getConfiguration()->comment = $2;
}
       | confMDTXPORT confNUMBER
{
  logParser(LOG_DEBUG, "line %-3i mdtxPort = %i", CONF_LINE_NO, $2);
  getConfiguration()->mdtxPort = $2;
}
       | confSSHPORT confNUMBER
{
  logParser(LOG_DEBUG, "line %-3i sshPort = %i", CONF_LINE_NO, $2);
  getConfiguration()->sshPort = $2;
}
       | confCACHESIZE confNUMBER confSIZE
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "cache size");
  getConfiguration()->cacheSize = $2*$3;
}
       | confCACHETTL confNUMBER confTIME
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "cache TTL");
  getConfiguration()->cacheTTL = $2*$3;
}
       | confQUERYTTL confNUMBER confTIME
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "query TTL");
  getConfiguration()->queryTTL = $2*$3;
}
       | confCHECKTTL confNUMBER confTIME
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "check TTL");
  getConfiguration()->checkTTL = $2*$3;
}
       | confSUPPTTL confNUMBER confTIME
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "supp TTL");
  getConfiguration()->scoreParam.suppTTL = $2*$3;
}
       | confMAXSCORE confSCORE
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "score: max");
  getConfiguration()->scoreParam.maxScore = $2;
}
       | confBADSCORE confSCORE
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "score: bad");
  getConfiguration()->scoreParam.badScore = $2;
}
       | confPOWSUPP confSCORE
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "score: pow supp");
  getConfiguration()->scoreParam.powSupp = $2;
}
       | confFACTSUPP confSCORE
{
  logParser(LOG_DEBUG, "line %-3i %s", CONF_LINE_NO, "score: fact supp");
  getConfiguration()->scoreParam.factSupp = $2;
}
;


cnetworks: cnetworks confCOMMA cnetwork
         | cnetwork
;

cgateways: cgateways confCOMMA cgateway
         | cgateway
;

Cnetworks: Cnetworks confCOMMA Cnetwork
         | Cnetwork
;

Cgateways: Cgateways confCOMMA Cgateway
         | Cgateway
;

cnetwork: confSTRING
{
  logParser(LOG_DEBUG, "line %-3i network: %s", CONF_LINE_NO, $1);
  if (!addNetworkToRing(CONF_COLL->networks, $1)) YYABORT;
  destroyString($1);
}
;

cgateway: confSTRING
{
  logParser(LOG_DEBUG, "line %-3i gateway: %s", CONF_LINE_NO, $1);
  if (!addNetworkToRing(CONF_COLL->gateways, $1)) YYABORT;
  destroyString($1);
}
;

Cnetwork: confSTRING
{
  logParser(LOG_DEBUG, "line %-3i network: %s", CONF_LINE_NO, $1);
  if (!addNetworkToRing(getConfiguration()->networks, $1)) YYABORT;
  destroyString($1);
}
;

Cgateway: confSTRING
{
  logParser(LOG_DEBUG, "line %-3i gateway: %s", CONF_LINE_NO, $1);
  if (!addNetworkToRing(getConfiguration()->gateways, $1)) YYABORT;
  destroyString($1);
}
;

%%

/* epilogue: =======================================================*/


/*=======================================================================
 * Function   : parsererror
 * Description: Emit an error message. Called by the parser on error
 *              state.
 * Synopsis   : void parsererror(char* message)
 * Input      : char* message = the error message.
 * Output     : N/A
 * Note       : Sorry, find no way to get the lineNo using reentrant
 *              parser.
 =======================================================================*/
void 
conf_error(const char* message)
{
  if(message != NULL) {
    logEmit(LOG_ERR, "parser error: %s", message);
  }
}

/*=======================================================================
 * Function   : parseConfiguration
 * Description: Parse a file
 * Synopsis   : int parseConfiguration(const char* path, int debugFlag)
 * Input      : const char* path: the file to parse
 * Output     : TRUE on success
=======================================================================*/
int 
parseConfiguration(const char* path)
{ 
  int rc = FALSE;
  ConfBisonSelf parser;
  ConfExtra extra;
  FILE* inputStream = stdin;

  logParser(LOG_NOTICE, "%s", "parse configuration");

  // initialise scanner
  parser.scanner = NULL;
  if (conf_lex_init(&parser.scanner)) {
    logEmit(LOG_ERR, "%s", "error initializing scanner");
    goto error;
  }

  // scan input file if defined (else stdin)
  inputStream = stdin;
  if (path != NULL) {
    if ((inputStream = fopen(path, "r")) == NULL) {
      logEmit(LOG_ERR, "cannot open input stream: %s", path); 
      goto error;
    }
    if (!lock(fileno(inputStream), F_RDLCK)) goto error;
    logEmit(LOG_DEBUG, "parse conf file: %s", path);
  }
  conf_set_in(inputStream, parser.scanner);

  // extra scanner data
  extra.lineNo = 1;
  conf_set_extra (&extra, parser.scanner);

  // debug mode for scanner
  conf_set_debug(env.debugLexer, parser.scanner);
  logEmit(LOG_DEBUG, "conf_set_debug = %i", 
	  conf_get_debug(parser.scanner));

  // extra parser data
  parser.coll = NULL;

  // call the parser
  if (conf_parse(&parser)) {
    logEmit(LOG_ERR, "configuration file parser error on line %i",
	    ((ConfExtra*)conf_get_extra(parser.scanner))->lineNo);
    logEmit(LOG_ERR, "please edit %s", path);
    goto error;
  }

  if (inputStream != stdin) {
    if (!unLock(fileno(inputStream))) goto error;
    fclose(inputStream);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "conf parser error");
  }
  conf_lex_destroy(parser.scanner);
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "supportFile.tab.h"
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
 * Synopsis   : ./utconfFile.tab
 * Input      : -i inputPath
 * Output     : Should display the same content
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = (Configuration*)conf;
  char* inputPath = NULL;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = PARSER_SHORT_OPTIONS"i:";
  struct option longOptions[] = {
    {"input", required_argument, NULL, 'i'},
    PARSER_LONG_OPTIONS,
    {0, 0, 0, 0}
  };
       
  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
    case 'i':
      if(isEmptyString(optarg)) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input stream\n",
		programName);
	rc = EINVAL;
      }
      else {
	if ((inputPath = (char*)malloc(sizeof(char) * strlen(optarg) + 1))
	    == NULL) {
	  fprintf(stderr, 
		  "%s: cannot allocate memory for the input stream name\n", 
		  programName);
	  rc = ENOMEM;
	}
	else {
	  strcpy(inputPath, optarg);
	}
      }
      break;
		
      GET_PARSER_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!parseConfiguration(inputPath)) goto error;
  if (!(conf = getConfiguration())) goto error;
  if (!expandConfiguration()) goto error;
  if (!populateConfiguration()) goto error;
  if (!serializeConfiguration(conf)) goto error;
  /************************************************************************/
  
  freeConfiguration();
  destroyString(inputPath);
  rc = TRUE;
 error:
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}
 
#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

