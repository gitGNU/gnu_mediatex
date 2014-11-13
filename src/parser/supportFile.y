/* prologue: =======================================================*/
%{
  /*=======================================================================
   * Version: $Id: supportFile.y,v 1.2 2014/11/13 16:37:01 nroche Exp $
   * Project: MediaTeX
   * Module : supportFile parser
   *
   * supportFile parser.

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
#include "../memory/supportTree.h"
#include "supportFile.h"
 
void supp_error(const char* message);

// scanner + anything we may want
// (parameter type of reentrant 'yyparse' function)
typedef struct SuppBisonSelf {
  yyscan_t       scanner;
  // ... anything we want here
} SuppBisonSelf;

// name of the belows parameter into bison: must use a common name
// define YYLEX yylex (&yylval, YYLEX_PARAM)
#define YYPARSE_PARAM suppBisonSelf
#define YYLEX_PARAM   ((SuppBisonSelf*)suppBisonSelf)->scanner

// shortcut maccros
#define SUPP_LINE_NO ((SuppExtra*)supp_get_extra(YYLEX_PARAM))->lineNo
%}

%{
  // data initialisation
%}

%code provides {
  #include "../memory/supportTree.h"
  int parseSupports(const char* path);
}

/* declarations: ===================================================*/
%file-prefix="supportFile"
%defines
%debug
%name-prefix="supp_"
%error-verbose
%verbose
%define api.pure

%start file

   /*%token            catEOL */
%token <time>   suppDATE
%token <hash>   suppHASH
%token <size>   suppSIZE
%token <status> suppSTATUS
%token <name>   suppNAME
%token          suppERROR

%%

   /* grammar rules: ==================================================*/

file: lines
    | //empty file
{
  logParser(LOG_WARNING, "%s", "the supports file was empty");
}
;

lines: lines line
     | line
;

line: suppDATE suppDATE suppDATE suppHASH suppHASH suppSIZE suppSTATUS suppNAME
{
  Support *support = NULL;
  struct tm date1;
  struct tm date2;
  struct tm date3;

  if (localtime_r(&$1, &date1) == (struct tm*)0 ||
      localtime_r(&$2, &date2) == (struct tm*)0 ||
      localtime_r(&$3, &date3) == (struct tm*)0) {
    logParser(LOG_ERR, "%s", "localtime_r returns on error");
    YYABORT;
  }

  logParser(LOG_DEBUG, "line %-3i: %04i-%02i-%02i,%02i:%02i:%02i "
	    "%04i-%02i-%02i,%02i:%02i:%02i %04i-%02i-%02i,%02i:%02i:%02i "
	    "%*s %*s %*lli %*s %s",
	    SUPP_LINE_NO,
	    date1.tm_year + 1900, date1.tm_mon+1, date1.tm_mday,
	    date1.tm_hour, date1.tm_min, date1.tm_sec,
	    date2.tm_year + 1900, date2.tm_mon+1, date2.tm_mday,
	    date2.tm_hour, date2.tm_min, date2.tm_sec,
	    date3.tm_year + 1900, date3.tm_mon+1, date3.tm_mday,
	    date3.tm_hour, date3.tm_min, date3.tm_sec,
	    MAX_SIZE_HASH, $4, MAX_SIZE_HASH, $5,
	    MAX_SIZE_SIZE, (long long int)$6, 
	    MAX_SIZE_STAT, $7, $8);

  if ((support = addSupport($8)) == NULL) YYERROR;
  support->firstSeen = $1;
  support->lastCheck = $2;
  support->lastSeen = $3;
  strncpy(support->quickHash, $4, MAX_SIZE_HASH);
  strncpy(support->fullHash, $5, MAX_SIZE_HASH);
  support->size = $6;
  strncpy(support->status, $7, MAX_SIZE_STAT);
  strncpy(support->name, $8, MAX_SIZE_NAME);
  support = NULL;
}
;

%%

/* epilogue: =======================================================*/


/*=======================================================================
 * Function   : supportFile_error
 * Description: Emit an error message. Called by the parser on error
 *              state.
 * Synopsis   : void supportFile_error(char* message)
 * Input      : char* message = the error message.
 * Output     : N/A
 =======================================================================*/
void 
supp_error(const char* message)
{
  if(message != NULL) {
    logEmit(LOG_ERR, "parser error: %s", message);
  }
}

/*=======================================================================
 * Function   : parseLocalSuppTree
 * Description: Parse the support File
 * Synopsis   : int parseLocalSuppTree(const char* path, int debugFlag)
 * Input      : const char* path: the support file path
 * Output     : TRUE on success
=======================================================================*/
int 
parseSupports(const char* path)
{
  int rc = FALSE;
  SuppBisonSelf parser;
  SuppExtra extra;
  FILE* inputStream = stdin;

  logParser(LOG_NOTICE, "%s", "parse supports");

  // initialise scanner
  parser.scanner = NULL;
  if (supp_lex_init(&parser.scanner)) {
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
    logEmit(LOG_DEBUG, "parse support file: %s", path);
  }
  supp_set_in(inputStream, parser.scanner);

  // extra scanner data
  extra.lineNo = 1;
  supp_set_extra (&extra, parser.scanner);

  // debug mode for scanner
  supp_set_debug(env.debugLexer, parser.scanner);
  logEmit(LOG_DEBUG, "supp_set_debug = %i", 
	  supp_get_debug(parser.scanner));

  // extra parser data (none)

  // call the parser
  if (supp_parse(&parser)) {
    logEmit(LOG_ERR, "support file parser error on line %i",
	    ((SuppExtra*)supp_get_extra(parser.scanner))->lineNo);
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
    logEmit(LOG_ERR, "%s", "support file parser error");
  }
  supp_lex_destroy(parser.scanner);
  return rc;
}

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
 * modif      : 2010/11/10
 * Description: unitary test for supportFile module
 * Synopsis   : ./utsupportFile.tab
 * Input      : -i inputPath
 * Output     : Should display the same content
 =======================================================================*/
int 
main(int argc, char** argv)
{
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
  if (!parseSupports(inputPath)) goto error;
  if (!serializeSupports()) goto error;
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
