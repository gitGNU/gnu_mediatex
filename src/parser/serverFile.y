/* prologue: =======================================================*/
%{
  /*=======================================================================
   * Version: $Id: serverFile.y,v 1.2 2014/11/13 16:37:00 nroche Exp $
   * Project: MediaTeX
   * Module : serverFile parser
   *
   * serverFile parser

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
#include "../memory/serverTree.h"
#include "serverFile.h"
  
  void serv_error(const char* message);

// scanner + anything we may want
// (parameter type of reentrant 'yyparse' function)
typedef struct ServBisonSelf {
  yyscan_t    scanner;
  // ... anything we want here
  Collection* collection;
  Server*     currentServer;
} ServBisonSelf;

// name of the belows parameter into bison: must use a common name
// define YYLEX yylex (&yylval, YYLEX_PARAM)
#define YYPARSE_PARAM servBisonSelf
#define YYLEX_PARAM   ((ServBisonSelf*)servBisonSelf)->scanner

// shortcut maccros
#define SERV_LINE_NO ((ServExtra*)serv_get_extra(YYLEX_PARAM))->lineNo
#define SERV_COLL    ((ServBisonSelf*)servBisonSelf)->collection
#define SERV_CURRSRV ((ServBisonSelf*)servBisonSelf)->currentServer
%}

%code provides {
  #include "../memory/serverTree.h"
  int parseServerFile(Collection* coll, const char* path);
}

/* declarations: ===================================================*/
%file-prefix="serverFile"
%defines
%debug
%name-prefix="serv_"
%error-verbose
%verbose
%define api.pure

%start file

   /*%token            srvEOL */
%token            srvMASTER
%token            srvSERVER
%token            srvCOMMENT
%token            srvLABEL
%token            srvHOST
%token            srvNETWORKS
%token            srvGATEWAYS
%token            srvMDTXPORT
%token            srvSSHPORT
%token            srvCOLLKEY
%token            srvUSERKEY
%token            srvHOSTKEY
%token            srvPROVIDE
%token            srvCOMMA
%token            srvCOLON
%token            srvEQUAL
%token            srvENDBLOCK
%token            srvCACHESIZE
%token            srvCACHETTL
%token            srvQUERYTTL
%token            srvSUPPTTL
%token            srvMAXSCORE
%token            srvBADSCORE
%token            srvPOWSUPP
%token            srvPOWIMAGE
%token            srvFACTSUPP
%token            srvMINGEODUP

%token <string>   srvSTRING
%token <number>   srvNUMBER
%token <score>    srvSCORE
%token <hash>     srvHASH
%token <string>   srvERROR
%token <size>     srvSIZE
%token <time>     srvTIME

%%

/* grammar rules: ==================================================*/

file: //empty file 
{
  logParser(LOG_WARNING, "%s", "the server file was empty");
}
    | headers
{
  logParser(LOG_DEBUG, "%s", "file: headers");
}
    | headers stanzas
{
  logParser(LOG_DEBUG, "%s", "file: headers stanzas");
}
;

headers: headers header
{
  logParser(LOG_DEBUG, "%s", "headers: headers header");
}
       | header
{
  logParser(LOG_DEBUG, "%s", "headers: header");
}
;

header: srvMASTER srvHASH
{
  logParser(LOG_DEBUG, "line %-3i master server: %s", SERV_LINE_NO, $2);
  if (!(SERV_COLL->serverTree->master = addServer(SERV_COLL, $2))) YYABORT;
}
      | srvCOLLKEY srvSTRING
{
  strncpy(SERV_COLL->serverTree->aesKey,
	  "01234567890abcdef", MAX_SIZE_AES);
  strncpy(SERV_COLL->serverTree->aesKey, $2, MAX_SIZE_AES);
  $2 = destroyString($2);
}
      | srvSUPPTTL srvNUMBER srvTIME
{
  SERV_COLL->serverTree->scoreParam.suppTTL = $2*$3;
}
      | srvMAXSCORE srvSCORE
{
   SERV_COLL->serverTree->scoreParam.maxScore = $2;
}
      | srvBADSCORE srvSCORE
{
  SERV_COLL->serverTree->scoreParam.badScore = $2;
}
      | srvPOWSUPP srvSCORE
{
  SERV_COLL->serverTree->scoreParam.powSupp = $2;
}
      | srvFACTSUPP srvSCORE
{
  SERV_COLL->serverTree->scoreParam.factSupp = $2;
}
      | srvMINGEODUP srvNUMBER
{
  SERV_COLL->serverTree->scoreParam.factSupp = $2;
}
;

stanzas: stanzas stanza
{
  logParser(LOG_DEBUG, "%s", "stanzas: stanzas stanza");
}
       | stanza
{
  logParser(LOG_DEBUG, "%s", "stanzas: stanza");
}
;

stanza: srvSERVER server lines srvENDBLOCK
{
  logParser(LOG_DEBUG, "%s", "srvSERVER server lines srvENDBLOCK");
  SERV_CURRSRV->user = destroyString(SERV_CURRSRV->user);
  if (isEmptyString(SERV_CURRSRV->label)) {
    if (!(SERV_CURRSRV->user = createString(env.confLabel))) YYABORT;
  }
  else {
    if (!(SERV_CURRSRV->user = createString(SERV_CURRSRV->label))) YYABORT;
  }
  if (!(SERV_CURRSRV->user = catString(SERV_CURRSRV->user, "-"))) YYABORT;
  if (!(SERV_CURRSRV->user = 
	catString(SERV_CURRSRV->user, SERV_COLL->label))) YYABORT;
}
;

server: srvHASH
{
  logParser(LOG_DEBUG, "line %-3i new server: %s", SERV_LINE_NO, $1);
  if (!(SERV_CURRSRV = addServer(SERV_COLL, $1))) YYABORT;
}
;

lines: lines line
     | line

line: srvLABEL srvSTRING
{
  SERV_CURRSRV->label = $2;
}
    | srvCOMMENT srvSTRING
{
  if (SERV_CURRSRV) SERV_CURRSRV->comment = $2;
  $2 = NULL;
}
    | srvHOST srvSTRING
{
  if (SERV_CURRSRV) strncpy(SERV_CURRSRV->host, $2, MAX_SIZE_HOST);
  destroyString($2);
}
    | srvNETWORKS networks
    | srvGATEWAYS gateways
    | srvMDTXPORT srvNUMBER
{
  if (SERV_CURRSRV) SERV_CURRSRV->mdtxPort = (int)$2;
}
    | srvSSHPORT srvNUMBER
{
  if (SERV_CURRSRV) SERV_CURRSRV->sshPort = (int)$2;
}
    | srvUSERKEY srvSTRING
{
  if (SERV_CURRSRV) SERV_CURRSRV->userKey = $2;
  $2 = NULL;
}
    | srvHOSTKEY srvSTRING
{
  if (SERV_CURRSRV) SERV_CURRSRV->hostKey = $2;
  $2 = NULL;
}
    | srvCACHESIZE srvNUMBER srvSIZE
{
  if (SERV_CURRSRV) SERV_CURRSRV->cacheSize = $2*$3;
}
    | srvCACHETTL srvNUMBER srvTIME
{
  if (SERV_CURRSRV) SERV_CURRSRV->cacheTTL = $2*$3;
}
    | srvQUERYTTL srvNUMBER srvTIME
{
  if (SERV_CURRSRV) SERV_CURRSRV->queryTTL = $2*$3;
}
    | srvPROVIDE images
;

images: images srvCOMMA image
      | image
;

image: srvHASH srvCOLON srvNUMBER srvEQUAL srvSCORE
{
  Archive* archive = NULL;
  Image* image = NULL;

  // score is log as 0 if we log it in 4rst position ?!
  // score is log as previous value if we log it in 3rd position ?!
  logParser(LOG_DEBUG, "line %-3i new image %s:%lli", SERV_LINE_NO, $1, $3);
  logParser(LOG_DEBUG, "score = %.2f", $5);

  if (SERV_CURRSRV) {
    if (!(archive = addArchive(SERV_COLL, $1, $3))) YYERROR;
    if (!(image = addImage(SERV_COLL, SERV_CURRSRV, archive))) YYERROR;
    image->score = $5;
  } else {
    logParser(LOG_DEBUG, "%s", "a mon avis ne passe jamais là");
    YYABORT;
  }
}
;

networks: networks srvCOMMA network
        | network
;

gateways: gateways srvCOMMA gateway
        | gateway
;

network: srvSTRING
{
  logParser(LOG_DEBUG, "line %-3i network: %s", SERV_LINE_NO, $1);
  if (!addNetworkToRing(SERV_CURRSRV->networks, $1)) YYABORT;
  destroyString($1);
}
;

gateway: srvSTRING
{
  logParser(LOG_DEBUG, "line %-3i gateway: %s", SERV_LINE_NO, $1);
  if (!addNetworkToRing(SERV_CURRSRV->gateways, $1)) YYABORT;
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
 =======================================================================*/
void serv_error(const char* message)
{
  if(message != NULL) {
    logEmit(LOG_ERR, "%s", message);
  }
}


/*=======================================================================
 * Function   : parseServerList()
 * Description: Parse a file
 * Synopsis   : int parseServerList(const char* path, int debugFlag)
 * Input      : const char* path: the file to parse
 * Output     : TRUE on success
=======================================================================*/
int parseServerFile(Collection* coll, const char* path)
{ 
  int rc = FALSE;
  ServBisonSelf parser;
  ServExtra extra;
  FILE* inputStream = stdin;

  checkCollection(coll);
  logParser(LOG_NOTICE, "parse %s servers", coll->label);

  // initialise scanner
  parser.scanner = NULL;
  if (serv_lex_init(&parser.scanner)) {
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
    logEmit(LOG_DEBUG, "parse server file: %s", path);
  }
  serv_set_in(inputStream, parser.scanner);

  // extra scanner data
  extra.lineNo = 1;
  serv_set_extra (&extra, parser.scanner);

  // debug mode for scanner
  serv_set_debug(env.debugLexer, parser.scanner);
  logEmit(LOG_DEBUG, "serv_set_debug = %i", 
	  serv_get_debug(parser.scanner));

  // extra parser data 
  parser.collection = coll;
  parser.currentServer = NULL;

  // call the parser
  if (serv_parse(&parser)) {
    logEmit(LOG_ERR, "servers file parser error on line %i",
	    ((ServExtra*)serv_get_extra(parser.scanner))->lineNo);
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
    logEmit(LOG_ERR, "%s", "servers file parser error");
  }
  serv_lex_destroy(parser.scanner);
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "confFile.tab.h"
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
 * Description: unitary test for ServerFile module
 * Synopsis   : ./utserverFile.tab
 * Input      : -i inputPath
 * Output     : Should display the same content
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = NULL;
  Collection* coll = NULL;
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
	rc = 2;
      }
      else {
	if ((inputPath = (char*)malloc(sizeof(char) * strlen(optarg) + 1))
	    == NULL) {
	  fprintf(stderr, 
		  "%s: cannot allocate memory for the input stream name\n", 
		  programName);
	  rc = 3;
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
  if (!(conf = getConfiguration())) goto error;
  if (!parseSupports(conf->supportDB)) goto error;
  if (!parseConfiguration(conf->confFile)) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!expandCollection(coll)) goto error;

  if (!parseServerFile(coll, inputPath)) goto error;
  if (!serializeServerTree(coll)) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  freeConfiguration();
  destroyString(inputPath);
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
