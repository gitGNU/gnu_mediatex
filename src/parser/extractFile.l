/*=======================================================================
 * Version: $Id: extractFile.l,v 1.1 2014/10/13 19:39:48 nroche Exp $
 * Project: Mediatex
 * Module : extraction meta-data scanner

 * extraction meta-data scanner

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
%option outfile="extractFile.c"
%option header-file="extractFile.h"

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
%option prefix="extr_"
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
 * Version: $Id: extractFile.l,v 1.1 2014/10/13 19:39:48 nroche Exp $
 * Project: Mediatex
 * Module : extraction meta-data scanner

 * extraction meta-data scanner

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
#include "../memory/extractTree.h"

#ifndef utMAIN
#include "extractFile.tab.h"
#endif	/*	: utMAIN	*/

typedef struct ExtrExtra {
  int            lineNo;
  // nothing more needed into the scanner
} ExtrExtra;

// %union{} used by flex/bison to write/read values
// (only available when linked with bison)
typedef union ExtrBisonUnion {
  // lex token types
  off_t  size;
  char   string[MAX_SIZE_STRING+1];
  char   hash[MAX_SIZE_HASH+1];
  EType  type; 

  // bison token types
  Archive* archive_t;
} ExtrBisonUnion;

#define YYSTYPE ExtrBisonUnion
#define YY_NO_INPUT // no warning
}

%{
  /* no local variables: reentrant */
%}

 /* scaner states */
%x COMMENT
%x CONTAINERS
%x CONTENTS
%x PATH

%%
   /* rules: ==========================================================*/
 /* first rule: */

<INITIAL>{

  \( {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: OPEN", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(extOPEN);
#endif	/*	: utMAIN	*/
  }

  rec {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: REC", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = REC;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  iso {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: ISO", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = ISO;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }
  
  cat {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: CAT", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = CAT;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  tgz {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: TGZ", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = TGZ;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  tbz {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: TBZ", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = TBZ;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  afio {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: AFIO", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = AFIO;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  tar {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: TAR", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = TAR;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  cpio {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: CPIO", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = CPIO;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  gzip {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: GZIP", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = GZIP;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  bzip {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: BZIP", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = BZIP;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  zip {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: ZIP", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = ZIP;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

  rar {
    BEGIN(CONTAINERS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: RAR", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    yylval->type = RAR;
    return(extTYPE);
#endif	/*	: utMAIN	*/
  }

 }

<CONTAINERS,CONTENTS>{

  : {
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: COLON", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(extCOLON);
#endif	/*	: utMAIN	*/
  }
  
  [[:xdigit:]]{32} {
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: HASH = %s", ((ExtrExtra*)yyextra)->lineNo, yytext);
#else	/*	: utMAIN	*/
    strncpy(yylval->hash, yytext, 32); // must-be
    yylval->hash[MAX_SIZE_HASH] = (char)0;
    return(extHASH);
#endif	/*	: utMAIN	*/
  }

  \) {
    BEGIN(INITIAL);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: CLOSE", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(extCLOSE);
#endif	/*	: utMAIN	*/
  }
 
 }

<CONTAINERS>{

  => {
    BEGIN(CONTENTS);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: IMPLIES", ((ExtrExtra*)yyextra)->lineNo);
#else	/*	: utMAIN	*/
    return(extIMPLIES);
#endif	/*	: utMAIN	*/
  }

  [[:digit:]]+ {
    off_t size = 0;
    size = strtoul(yytext, (char**)0, 10);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: SIZE=%i", ((ExtrExtra*)yyextra)->lineNo, size); 
#else	/*	: utMAIN	*/
    yylval->size = size;
    return(extSIZE);
#endif	/*	: utMAIN	*/
  }
 
 }

<CONTENTS>{

  [[:digit:]]+ {
     BEGIN(PATH);
    off_t size = 0;
    size = strtoul(yytext, (char**)0, 10);
#ifdef utMAIN
    logEmit(LOG_DEBUG, "line %i: SIZE=%i", ((ExtrExtra*)yyextra)->lineNo, size); 
#else	/*	: utMAIN	*/
    yylval->size = size;
    return(extSIZE);
#endif	/*	: utMAIN	*/
  }
 
 }

<PATH>{

  [^[:blank:]:,\)\n\r]* {
    BEGIN(CONTENTS);
#ifdef utMAIN
    logEmit(LOG_INFO, "line %i: STRING = \%s", 
	    ((ExtrExtra*)yyextra)->lineNo, yytext);
#else	/*	: utMAIN	*/
    strncpy(yylval->string, yytext, yyleng); // must-be
    yylval->string[yyleng] = (char)0;
    return(extSTRING);
#endif	/*	: utMAIN	*/
  }

 }

<COMMENT>{

  [^\n\r] {  /* : eat up any unmatched character */
    ++((ExtrExtra*)yyextra)->lineNo;
  }
  
  \*\/ {
    BEGIN(INITIAL);
  }

 } 
   
<*>{

  [[:blank:]]+  /* : eat up any blank character */
    ^#.*$       /* : eat up comments */
    \/\/.*$     /* : eat up comments */

    \/\* {
    BEGIN(COMMENT);
  }
  
  (\n|\r\n) {
    ++((ExtrExtra*)yyextra)->lineNo;
#ifndef utMAIN
    ++env.progBar.cur;
#endif
  }

  . { /* : Eat up any unmatched character */
    logEmit(LOG_ERR, "line %i: exit on scanner error: %s",
	    ((ExtrExtra*)yyextra)->lineNo, yytext);
    /* generate a parsor error (unexpected $end) */
#ifdef utMAIN
    return -1;
#else
    return extERROR;
#endif
  }

 }
   
%%
   /* user code (literally copied to the generated source) :
      ==========*/

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
   //#include <sys/types.h>
   //#include <sys/stat.h>
   //#include <fcntl.h>
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
 * Description: Extract lexer unit test
 * Synopsis   : ./utextractFile
 * Input      : stdin
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputPath = NULL;
  FILE* inputStream = stdin;
  ExtrExtra extra;
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
  if (extr_lex_init(&scanner)) {
    logEmit(LOG_ERR, "%s", "error initializing scanner");
    goto error;
  }

  extr_set_debug(env.debugLexer, scanner);
  logEmit(LOG_DEBUG, "extr_set_debug = %i", extr_get_debug(scanner));
 
  
  // scan input file if defined
  if (inputPath != NULL) {
    if ((inputStream = fopen(inputPath, "r")) == NULL) {
      logEmit(LOG_ERR, "cannot open input stream: %s", inputPath); 
      goto error;
    }
  }

  // call scanner 
  extra.lineNo = 1;
  extr_set_extra (&extra, scanner);
  extr_set_in(inputStream, scanner);
  if (extr_lex(NULL, scanner)) goto error;
  extr_lex_destroy(scanner);
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
