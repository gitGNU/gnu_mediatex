/*=======================================================================
 * Version: $Id: catalogFile.l,v 1.1 2014/10/13 19:39:47 nroche Exp $
 * Project: Mediatex
 * Module : catalog scanner

 * catalog scanner

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
%option outfile="catalogFile.c"
%option header-file="catalogFile.h"

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
%option prefix="cata_"
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
 * Version: $Id: catalogFile.l,v 1.1 2014/10/13 19:39:47 nroche Exp $
 * Project: Mediatex
 * Module : catalog scanner

 * catalog scanner

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
#include "catalogFile.tab.h"
#endif	/*	: utMAIN	*/

typedef struct CataExtra {
  int            lineNo;
  // nothing more needed into the scanner
} CataExtra;

// %union{} used by flex/bison to write/read values
// (only available when linked with bison)
typedef union CataBisonUnion {
  // lex token types
  off_t  size;
  char   string[MAX_SIZE_STRING+1];
  char   hash[MAX_SIZE_HASH+1];

  // bison token types
  Category* category_t;
  Human*    human_t;
  Document* document_t;
  Archive*  archive_t;
} CataBisonUnion;

#define YYSTYPE CataBisonUnion
#define YY_NO_INPUT // no warning
}

%{
 /* no local variables: reentrant */
%}

 /* scaner states */
%x Comment

/* maccros */

 /* MAX_SIZE_HASH = 32 */
HASH    [[:xdigit:]]+[:blank:]     

%%
   /* rules: ==========================================================*/
   /* first rule: */

<INITIAL>{

  [[:blank:]]+  /* : eat up any blank character */
    ^#.*$         /* : eat up comments */
    \/\/.*$       /* : eat up comments */
    
    \/\* {
    BEGIN(Comment);
  }
  
  (\n|\r\n) {
    ++((CataExtra*)yyextra)->lineNo;
#ifndef utMAIN
    ++env.progBar.cur;
#endif
  }

  top {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: TOP", ((CataExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(catTOP);
#endif	/*	: utMAIN	*/
  }

  category {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: CATEGORY", ((CataExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(catCATEGORY);
#endif	/*	: utMAIN	*/
  }
	
  document {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: DOCUMENT", ((CataExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(catDOCUMENT);
#endif	/*	: utMAIN	*/
  }
  
  human {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: HUMAN", ((CataExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(catHUMAN);
#endif	/*	: utMAIN	*/
  }
  
  with {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: WITH", ((CataExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(catWITH);
#endif	/*	: utMAIN	*/
  }
  
  archive {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: ARCHIVE", ((CataExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(catARCHIVE);
#endif	/*	: utMAIN	*/
  }
  
  = {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: EQUAL", ((CataExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(catEQUAL);
#endif	/*	: utMAIN	*/
  }
  
  : {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: COLON", ((CataExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(catCOLON);
#endif	/*	: utMAIN	*/
  }
  
  , {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: COMMA", ((CataExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(catCOMMA);
#endif	/*	: utMAIN	*/
  }
  
  {HASH} {
    // force to take 32 char
    // else the ending digit are taken by the next token
    if (yyleng != MAX_SIZE_HASH) yyless(MAX_SIZE_HASH);
#ifdef utMAIN
      logEmit(LOG_INFO, "line %i: HASH = %s", 
	      ((CataExtra*)yyextra)->lineNo, yytext);
#else	/*	: utMAIN	*/
      strncpy(yylval->hash, yytext, MAX_SIZE_HASH); // must-be
      yylval->hash[MAX_SIZE_HASH] = (char)0;
      return(catHASH);
#endif	/*	: utMAIN	*/
    }

  [[:digit:]]+ {
    off_t size = 0;
    sscanf(yytext, "%llu", &size);
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: NUMBER = %lli", 
	    ((CataExtra*)yyextra)->lineNo, size);
#else	/*	: utMAIN	*/
    yylval->size = size;
    return(catNUMBER);
#endif	/*	: utMAIN	*/
	}

  \"[^\n\"]*\" { 
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: STRING = %s"
	    , ((CataExtra*)yyextra)->lineNo, yytext);
#else	/*	: utMAIN	*/
    strncpy(yylval->string, yytext+1, yyleng-2); // must-be
    yylval->string[yyleng-2] = (char)0;
    return(catSTRING);
#endif	/*	: utMAIN	*/
  }
 }

<Comment>{
  [^\n\r]   /* : eat up any unmatched character */
    
    \*\/ {
    BEGIN(INITIAL);
  }
 }

<*>{
  [^\n\r] { /* : eat up any unmatched character */

    logEmit(LOG_ERR, "line %i: exit on scanner error: %s", 
	    ((CataExtra*)yyextra)->lineNo, yytext);
    /* generate a parsor error (unexpected $end) */
#ifdef utMAIN
    return -1;
#else	/*	: utMAIN	*/
    return(catERROR);
#endif	/*	: utMAIN	*/
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
 * modif      : 2012/05/01
 * Description: Catalog lexer unit test
 * Synopsis   : ./utcatalogFile
 * Input      : stdin
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputPath = NULL;
  FILE* inputStream = stdin;
  CataExtra extra;
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
  if (cata_lex_init(&scanner)) {
    logEmit(LOG_ERR, "%s", "error initializing scanner");
    goto error;
  }

  cata_set_debug(env.debugLexer, scanner);
  logEmit(LOG_DEBUG, "cata_set_debug = %i", cata_get_debug(scanner));
 
  
  // scan input file if defined
  if (inputPath != NULL) {
    if ((inputStream = fopen(inputPath, "r")) == NULL) {
      logEmit(LOG_ERR, "cannot open input stream: %s", inputPath); 
      goto error;
    }
  }

  // call scanner 
  extra.lineNo = 1;
  cata_set_extra (&extra, scanner);
  cata_set_in(inputStream, scanner);
  if (cata_lex(NULL, scanner)) goto error;
  cata_lex_destroy(scanner);
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
