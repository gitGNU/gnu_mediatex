/*=======================================================================
 * Version: $Id: serverFile.y,v 1.10 2015/09/03 13:02:33 nroche Exp $
 * Project: MediaTeX
 * Module : server parser
 *
 * server file parser

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

/* prologue: ===========================================================*/

%code provides {
/*=======================================================================
 * Version: this file is generated by BISON using serverFile.y
 * Project: MediaTeX
 * Module : server parser
 *
 * server file parser

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

#define YYSTYPE SERV_STYPE
}

%{  
#include "mediatex-config.h"
  /* yyscan_t is pre-defined by mediatex-config.h and will be defined
     by Flex header bellow */
%}

%union {
  off_t  number;
  off_t  size;
  time_t time;
  float  score;
  char   string[(MAX_SIZE_STRING+1)*2]; // keys needs more place
  char   hash[MAX_SIZE_HASH+1];
}

%{
  // upper YYSTYPE union is required by Flex headers
#include "parser/serverFile.h"

#define LINENO serv_get_lineno(yyscanner)
  
void serv_error(yyscan_t yyscanner, Collection* coll, Server* server,
		const char* message);
%}

/* declarations: =======================================================*/
%defines "parser/serverFile.tab.h"
%output "parser/serverFile.tab.c"
%define api.prefix {serv_}
%define api.pure full
%param {yyscan_t yyscanner}
%parse-param {Collection* coll} {Server* server}
%define parse.error verbose
%verbose
%debug

%start file

%token            servMASTER
%token            servSERVER
%token            servCOMMENT
%token            servLABEL
%token            servHOST
%token            servLASTCOMMIT
%token            servNETWORKS
%token            servGATEWAYS
%token            servMDTXPORT
%token            servSSHPORT
%token            servWWWPORT
%token            servCOLLKEY
%token            servUSERKEY
%token            servHOSTKEY
%token            servPROVIDE
%token            servCOMMA
%token            servCOLON
%token            servEQUAL
%token            servENDBLOCK
%token            servCACHESIZE
%token            servCACHETTL
%token            servQUERYTTL
%token            servSUPPTTL
%token            servUPLOADTTL
%token            servSERVERTTL
%token            servMAXSCORE
%token            servBADSCORE
%token            servPOWSUPP
%token            servFACTSUPP
%token            servFILESCORE
%token            servMINGEODUP
%token            servERROR

%token <string>   servSTRING
%token <number>   servNUMBER
%token <score>    servSCORE
%token <hash>     servHASH
%token <size>     servSIZE
%token <time>     servTIME
%token <time>     servDATE

%%
 /* grammar rules: =====================================================*/

file: //empty file 
{
  logParser(LOG_INFO, "the server file was empty");
}
    | headers
{
  logParser(LOG_DEBUG, "line %3-i: file: headers", LINENO);
}
    | headers stanzas
{
  logParser(LOG_DEBUG, "line %3-i: file: headers stanzas", LINENO);
}
;

headers: headers header
{
  logParser(LOG_DEBUG, "line %3-i: headers: headers header", LINENO);
}
       | header
{
  logParser(LOG_DEBUG, "line %3-i: headers: header", LINENO);
}
;

header: servMASTER servHASH
{
  logParser(LOG_DEBUG, "line %-3i master server: %s", LINENO, $2);
  if (!(coll->serverTree->master = addServer(coll, $2))) YYABORT;
}
      | servCOLLKEY servSTRING
{
  strncpy(coll->serverTree->aesKey,
	  "01234567890abcdef", MAX_SIZE_AES);
  strncpy(coll->serverTree->aesKey, $2, MAX_SIZE_AES);
}
      | servSERVERTTL servNUMBER servTIME
{
  coll->serverTree->serverTTL = $2*$3;
}
      | servSUPPTTL servNUMBER servTIME
{
  coll->serverTree->scoreParam.suppTTL = $2*$3;
}
      | servUPLOADTTL servNUMBER servTIME
{
  coll->serverTree->uploadTTL = $2*$3;
}
      | servMAXSCORE servSCORE
{
   coll->serverTree->scoreParam.maxScore = $2;
}
      | servBADSCORE servSCORE
{
  coll->serverTree->scoreParam.badScore = $2;
}
      | servPOWSUPP servSCORE
{
  coll->serverTree->scoreParam.powSupp = $2;
}
      | servFACTSUPP servSCORE
{
  coll->serverTree->scoreParam.factSupp = $2;
}
      | servFILESCORE servSCORE
{
   coll->serverTree->scoreParam.fileScore = $2;
}
      | servMINGEODUP servNUMBER
{
  coll->serverTree->scoreParam.factSupp = $2;
}
;

stanzas: stanzas stanza
{
  logParser(LOG_DEBUG, "line %3-i: stanzas: stanzas stanza", LINENO);
}
       | stanza
{
  logParser(LOG_DEBUG, "line %3-i: stanzas: stanza", LINENO);
}
;

stanza: servSERVER server lines servENDBLOCK
{
  logParser(LOG_DEBUG, "line %3-i: servSERVER server lines servENDBLOCK", 
	    LINENO);
  server->user = destroyString(server->user);
  if (isEmptyString(server->label)) {
    if (!(server->user = createString(env.confLabel))) YYABORT;
  }
  else {
    if (!(server->user = createString(server->label))) YYABORT;
  }
  if (!(server->user = catString(server->user, "-"))) YYABORT;
  if (!(server->user = 
	catString(server->user, coll->label))) YYABORT;
}
;

server: servHASH
{
  logParser(LOG_DEBUG, "line %-3i new server: %s", LINENO, $1);
  if (!(server = addServer(coll, $1))) YYABORT;
}
;

lines: lines line
     | line

line: servLABEL servSTRING
{
  if (!(server->label = createString($2))) YYERROR;
}
    | servCOMMENT servSTRING
{
  if (!server) YYERROR;
  if (!(server->comment = createString($2))) YYERROR;
}
    | servHOST servSTRING
{
  if (!server) YYERROR;
  strncpy(server->host, $2, MAX_SIZE_HOST);
}
    | servLASTCOMMIT servDATE
{
  if (!server) YYERROR;
  server->lastCommit = $2;
}
    | servNETWORKS networks
    | servGATEWAYS gateways
    | servMDTXPORT servNUMBER
{
  if (!server) YYERROR;
  if (server) server->mdtxPort = (int)$2;
}
    | servSSHPORT servNUMBER
{
  if (server) server->sshPort = (int)$2;
}
    | servWWWPORT servNUMBER
{
  if (server) server->wwwPort = (int)$2;
}
    | servUSERKEY servSTRING
{
  if (!server) YYERROR;
  if (!(server->userKey = createString($2))) YYERROR;
}
    | servHOSTKEY servSTRING
{
  if (!server) YYERROR;
  if (!(server->hostKey = createString($2))) YYERROR;
}
    | servCACHESIZE servNUMBER servSIZE
{
  if (server) server->cacheSize = $2*$3;
}
    | servCACHETTL servNUMBER servTIME
{
  if (server) server->cacheTTL = $2*$3;
}
    | servQUERYTTL servNUMBER servTIME
{
  if (server) server->queryTTL = $2*$3;
}
    | servPROVIDE images
;

images: images servCOMMA image
      | image
;

image: servHASH servCOLON servNUMBER servEQUAL servSCORE
{
  Archive* archive = 0;
  Image* image = 0;

  // score is log as 0 if we log it in 4rst position ?!
  // score is log as previous value if we log it in 3rd position ?!
  logParser(LOG_DEBUG, "line %-3i new image %s:%lli", LINENO, $1, $3);
  logParser(LOG_DEBUG, "score = %.2f", $5);

  if (server) {
    if (!(archive = addArchive(coll, $1, $3))) YYERROR;
    if (!(image = addImage(coll, server, archive))) YYERROR;
    image->score = $5;
  } else {
    logParser(LOG_ERR, "line %-3i: server not found (internal error)",
	      LINENO);
    YYABORT;
  }
}
;

networks: networks servCOMMA network
        | network
;

gateways: gateways servCOMMA gateway
        | gateway
;

network: servSTRING
{
  logParser(LOG_DEBUG, "line %-3i network: %s", LINENO, $1);
  if (!addNetworkToRing(server->networks, $1)) YYABORT;
}
;

gateway: servSTRING
{
  logParser(LOG_DEBUG, "line %-3i gateway: %s", LINENO, $1);
  if (!addNetworkToRing(server->gateways, $1)) YYABORT;
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
void serv_error(yyscan_t yyscanner, Collection* coll, Server* server,
		const char* message)
{
  logParser(LOG_ERR, "%s on token '%s' line %i",
	  message, serv_get_text(yyscanner), LINENO);
}


/*=======================================================================
 * Function   : parseServerList
 * Description: Parse a server file
 * Synopsis   : int parseServerList(const char* path)
 * Input      : const char* path: the file to parse
 * Output     : TRUE on success
=======================================================================*/
int parseServerFile(Collection* coll, const char* path)
{ 
  int rc = FALSE;
  FILE* inputStream = stdin;
  yyscan_t scanner;
  Server* server = 0;

  checkCollection(coll);
  logParser(LOG_INFO, "parse %s servers from %s",
	    coll->label, path?path:"stdin");

  // initialise scanner
  if (serv_lex_init(&scanner)) {
    logParser(LOG_ERR, "%s", "error initializing scanner");
    goto error;
  }

  if (path != 0) {
    if (!(inputStream = fopen(path, "r"))) {
      logParser(LOG_ERR, "cannot open input stream: %s", path); 
      goto error;
    }
    if (!lock(fileno(inputStream), F_RDLCK)) goto error2;
  }

  serv_set_in(inputStream, scanner);

  // debug mode for scanner
  serv_set_debug(env.debugLexer, scanner);
  logParser(LOG_DEBUG, "serv_set_debug = %i", serv_get_debug(scanner));

  // call the parser
  if (serv_parse(scanner, coll, server)) {
    logParser(LOG_ERR, "servers file parser error on line %i",
	    serv_get_lineno(scanner));
    logParser(LOG_ERR, "please edit %s", path?path:"stdin");
    goto error3;
  }

  rc = TRUE;
 error3:
  if (inputStream != stdin) {
    if (!unLock(fileno(inputStream))) rc = FALSE;
  }
 error2:
  if (inputStream != stdin) {
    fclose(inputStream);
  }
 error:
  if (!rc) {
    logParser(LOG_ERR, "%s", "servers file parser error");
  }
  serv_lex_destroy(scanner);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
