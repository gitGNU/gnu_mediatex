/*=======================================================================
 * Version: $Id: serverFile.l,v 1.2 2014/11/13 16:36:59 nroche Exp $
 * Project: Mediatex
 * Module : server scanner

 * server scanner

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

   /* definitions: ====================================================*/

/* Options for Specifing Filenames: */
%option outfile="serverFile.c"
%option header-file="serverFile.h"

/* Options Affecting Scanner Behavior: */
%option case-insensitive
%option nolex-compat
/* option batch OU interactive */
%option batch
%option 8bit
%option noposix-compat
%option nostdinit
%option noyywrap

/* Code-Level And API Options: */
%option ansi-definitions
%option ansi-prototypes
%option reentrant
%option bison-bridge
%option noc++
%option pointer
%option prefix="serv_"
%option nomain
%option unistd

/* Options for Scanner Speed and Size: */
%option noalign
%option ecs
%option nometa-ecs
%option noread

/* Debugging Options: */
%option nobackup
%option debug
%option noperf-report
%option warn

/* Miscellaneous Options: */
%option nounput

%top{
/*=======================================================================
 * Version: $Id: serverFile.l,v 1.2 2014/11/13 16:36:59 nroche Exp $
 * Project: Mediatex
 * Module : serverFile scanner

 * server scanner
 
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

#include "../mediatex.h"
#include "../misc/log.h"
#include "../memory/strdsm.h"

#ifndef utMAIN
#include "serverFile.tab.h"
#endif	/*	: utMAIN	*/

typedef struct ServExtra {
  int            lineNo;
  // nothing more needed into the scanner
} ServExtra;

// %union{} used by flex/bison to write/read values
// (only available when linked with bison)
typedef union ServBisonUnion {
  // lex token types
  off_t  number;
  off_t  size;
  time_t time;
  float  score;
  char*  string;
  char   hash[MAX_SIZE_HASH+1];

  // bison token types
  struct Server* server_t;
  struct Image*  image_t;
  struct RG*     imagesRing;
} ServBisonUnion;

#define YYSTYPE ServBisonUnion
#define YY_NO_INPUT // no warning
}

%{
  /* no local variables: reentrant */
%}

 /* scaner states */
%x COMMENT
%x USERVALUE

 /* maccros */
 /* MAX_SIZE_HASH = 32 */
 //HASH    [[:xdigit:]]+[:blank:] (we have a bug here)
HASH    [[:xdigit:]]{32}

%%
 /* rules: ==========================================================*/
 /* first rule: */

<INITIAL>{

  server {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: SERVER", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvSERVER);
#endif	/*	: utMAIN	*/
  }

  label {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: LABEL", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvLABEL);
#endif	/*	: utMAIN	*/
  }

  host {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: HOST", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvHOST);
#endif	/*	: utMAIN	*/
  }

 networks {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: NETWORKS", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvNETWORKS);
#endif	/*	: utMAIN	*/
  }

 gateways {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: GATEWAYS", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvGATEWAYS);
#endif	/*	: utMAIN	*/
  }

  mdtxport {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: MDTXPORT", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvMDTXPORT);
#endif	/*	: utMAIN	*/
  }

  sshport {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: HOSTPORT", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvSSHPORT);
#endif	/*	: utMAIN	*/
  }

  comment {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: COMMENT", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvCOMMENT);
#endif	/*	: utMAIN	*/
  }

  master {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: MASTER", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvMASTER);
#endif	/*	: utMAIN	*/
  }

  collKey {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: COLLKEY", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvCOLLKEY);
#endif	/*	: utMAIN	*/
  }

  userKey {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: USERKEY", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvUSERKEY);
#endif	/*	: utMAIN	*/
  }

  hostKey {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: HOSTKEY", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvHOSTKEY);
#endif	/*	: utMAIN	*/
  }

  provide {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: PROVIDE", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvPROVIDE);
#endif	/*	: utMAIN	*/
  }

  , {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: COMMA", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvCOMMA);
#endif	/*	: utMAIN	*/
  }

  : {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: COLON", ((ServExtra*)yyextra)->lineNo); 
#else	/*	: utMAIN	*/
    return(srvCOLON);
#endif	/*	: utMAIN	*/
  } 

  = {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: EQUAL", ((ServExtra*)yyextra)->lineNo); 
#else	/*	: utMAIN	*/
    return(srvEQUAL);
#endif	/*	: utMAIN	*/
  } 

 end(block)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: ENDBLOCK", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvENDBLOCK);
#endif	/*	: utMAIN	*/
  }

  cachesize {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: CACHE_SIZE", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvCACHESIZE);
#endif	/*	: utMAIN	*/
  }

  cachettl {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: CACHE_TTL", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvCACHETTL);
#endif	/*	: utMAIN	*/
  }

  queryttl {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: QUERY_TTL", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvQUERYTTL);
#endif	/*	: utMAIN	*/
  }

  suppttl {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: SUPP_TTL", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvSUPPTTL);
#endif	/*	: utMAIN	*/
  }

  maxscore {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: MAXSCORE", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvMAXSCORE);
#endif	/*	: utMAIN	*/
  }

  badscore {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: BADSCORE", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvBADSCORE);
#endif	/*	: utMAIN	*/
  }

  powsupp {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: POWSUPP", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvPOWSUPP);
#endif	/*	: utMAIN	*/
  }

  factsupp {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: FACTSUPP", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvFACTSUPP);
#endif	/*	: utMAIN	*/
  }

  mingeodup {
    BEGIN(USERVALUE);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: MINGEODUP", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(srvMINGEODUP);
#endif	/*	: utMAIN	*/
  }

  g(o)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: GIGA", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->size = GIGA;
    return(srvSIZE);
#endif	/*	: utMAIN	*/
  }

  m(o)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: MEGA", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
     yylval->size = MEGA;
    return(srvSIZE);
#endif	/*	: utMAIN	*/
  }

  k(o)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: KILO", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->size = KILO;
    return(srvSIZE);
#endif	/*	: utMAIN	*/
  }

  o {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: OCTET", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->size = 1;
    return(srvSIZE);
#endif	/*	: utMAIN	*/
  }

  y(ear)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: YEAR", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->time = YEAR;
    return(srvTIME);
#endif	/*	: utMAIN	*/
  }  

  mon(th)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: MONTH", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->time = MONTH;
    return(srvTIME);
#endif	/*	: utMAIN	*/
  }

  w(eek)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: WEEK", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->time = WEEK;
    return(srvTIME);
#endif	/*	: utMAIN	*/
  }

  [Dd](ay)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: DAY", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->time = DAY;
    return(srvTIME);
#endif	/*	: utMAIN	*/
  }

  h(our)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: HOUR", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->time = HOUR;
    return(srvTIME);
#endif	/*	: utMAIN	*/
  }

  min(ute)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: MINUTE", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->time = MINUTE;
    return(srvTIME);
#endif	/*	: utMAIN	*/
  }

  s(econde)? {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: SECONDE", ((ServExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->time = 1;
    return(srvTIME);
#endif	/*	: utMAIN	*/
  }
 }

 /* Allow to use a value wich begin by a token name */
<USERVALUE>{
  
  {HASH} {
    BEGIN(INITIAL);
    /* bug with the line bellow:
    echo "Server aa495f55d629c1dd663667735fd63cf0" | ./utserverFile -L
    [debug serverFile.l:222] line 1   HASH =aa495f55d629c1dd663667735fd63cf0

    echo "Server a1495f55d629c1dd663667735fd63cf0" | ./utserverFile -L
    --accepting rule at line 291 ("a")

    // force to take 32 char
    // else the ending digit are taken by the=
    //if (yyleng != MAX_SIZE_HASH) yyless(MAX_SIZE_HASH);
    */
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: HASH =%s", ((ServExtra*)yyextra)->lineNo, yytext);
#else	/*	: utMAIN	*/
    strncpy(yylval->hash, yytext, MAX_SIZE_HASH);
    yylval->hash[MAX_SIZE_HASH] = (char)0;
    return(srvHASH);
#endif	/*	: utMAIN	*/
  }

  [[:digit:]]+\.[[:digit:]]+ {
    float score = -1;
    BEGIN(INITIAL);
    sscanf(yytext, "%f", &score);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: SCORE=%.2f", ((ServExtra*)yyextra)->lineNo, score); 
#else	/*	: utMAIN	*/
    yylval->score = score;
    return(srvSCORE);
#endif	/*	: utMAIN	*/
  }

  [[:digit:]]+ {
    off_t number = 0;
    BEGIN(INITIAL);
    sscanf(yytext, "%llu", &number);
    //number = strtoul(yytext, (char**)0, 10);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: NUMBER=%i", ((ServExtra*)yyextra)->lineNo, number); 
#else	/*	: utMAIN	*/
    yylval->number = number;
    return(srvNUMBER);
#endif	/*	: utMAIN	*/
  }

  \"[^\n\"]*\" { 
    BEGIN(INITIAL);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: ID=     '%s'", ((ServExtra*)yyextra)->lineNo, yytext);
#else	/*	: utMAIN	*/
    yylval->string = createSizedString(yyleng-2, yytext+1);
    return(srvSTRING);
#endif	/*	: utMAIN	*/
  }

  [^[:blank:]:,=\r\n]+ {
    BEGIN(INITIAL);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: ID ='%s'", ((ServExtra*)yyextra)->lineNo, yytext);
#else	/*	: utMAIN	*/
    yylval->string = createSizedString(yyleng, yytext);
    return(srvSTRING);
#endif	/*	: utMAIN	*/
  }
 }

<INITIAL,USERVALUE>{
    [[:blank:]]+  /* : eat up any blank character */
    ^#.*$         /* : eat up comments */
    \/\/.*$       /* : eat up comments */
    
    \/\* {
    BEGIN(COMMENT);
    }

    (\n|\r\n) {
      ++((ServExtra*)yyextra)->lineNo;
    }
 }

<COMMENT>{

  [^\n\r]   /* : eat up any unmatched character */

    \*\/ {
    BEGIN(INITIAL);
  }
 } 


<*>{
  . { /* : Eat up any unmatched character */

    logEmit(LOG_ERR, "line %i: exit on scanner error: \'%s\'",
    	    ((ServExtra*)yyextra)->lineNo, yytext);
    
    /* generate a parsor error (unexpected $end) */
#ifdef utMAIN
    return -1;
#else
    return srvERROR;
#endif
  }
 }
   
%%
   /* user code (literally copied to the generated source) :
      ==========*/

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
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
  char* inputPath = NULL;
  FILE* inputStream = stdin;
  ServExtra extra;
  yyscan_t scanner;
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
  if (serv_lex_init(&scanner)) {
    logEmit(LOG_ERR, "%s", "error initializing scanner");
    goto error;
  }

  serv_set_debug(env.debugLexer, scanner);
  logEmit(LOG_DEBUG, "serv_set_debug = %i", serv_get_debug(scanner));
 
  
  // scan input file if defined
  if (inputPath != NULL) {
    if ((inputStream = fopen(inputPath, "r")) == NULL) {
      logEmit(LOG_ERR, "cannot open input stream: %s", inputPath); 
      goto error;
    }
  }

  // call scanner 
  extra.lineNo = 1;
  serv_set_extra (&extra, scanner);
  serv_set_in(inputStream, scanner);
  if (serv_lex(NULL, scanner)) goto error;
  serv_lex_destroy(scanner);
  fclose(inputStream);
  /************************************************************************/

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
