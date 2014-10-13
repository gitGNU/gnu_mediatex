/* prologue: =======================================================*/
%{
  /*=======================================================================
   * Version: $Id: extractFile.y,v 1.1 2014/10/13 19:39:48 nroche Exp $
   * Project: MediaTeX
   * Module : extractFile parser
   *
   * extract parser.

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
#include "../memory/extractTree.h"
#include "extractFile.h"

void extr_error(const char* message);

// scanner + anything we may want
// (parameter type of reentrant 'yyparse' function)
typedef struct ExtrBisonSelf {
  yyscan_t    scanner;
  // ... anything we want here
  Collection* collection;
  Container*  container;
  FromAsso*   fromAsso;
} ExtrBisonSelf;

// name of the belows parameter into bison: must use a common name
// define YYLEX yylex (&yylval, YYLEX_PARAM)
#define YYPARSE_PARAM extrBisonSelf
#define YYLEX_PARAM   ((ExtrBisonSelf*)extrBisonSelf)->scanner

// shortcut maccros
#define EXTR_LINE_NO   ((ExtrExtra*)extr_get_extra(YYLEX_PARAM))->lineNo
#define EXTR_COLL      ((ExtrBisonSelf*)extrBisonSelf)->collection
#define EXTR_CONTAINER ((ExtrBisonSelf*)extrBisonSelf)->container
#define EXTR_FROM_ASSO ((ExtrBisonSelf*)extrBisonSelf)->fromAsso
%}

%code provides {
  #include "../memory/extractTree.h"
  int parseExtractFile(Collection* coll, const char* path);
}

/* declarations: ===================================================*/
%file-prefix="extractFile"
%defines
%debug
%name-prefix="extr_"
%error-verbose
%verbose
%define api.pure

%start file

   /*%token            extEOL */
%token            extOPEN
%token            extCLOSE
%token <type>     extTYPE
%token            extIMPLIES
%token            extAS
%token <hash>     extHASH
%token            extCOLON
%token <size>     extSIZE
%token            extCOMMA
%token <string>   extSTRING
%token <string>   extERROR

%type  <archive_t> archive

%%

/* grammar rules: ==================================================*/

file: stanzas
    | //empty file
{
  logParser(LOG_WARNING, "%s", "the extract file was empty");
}
;

stanzas: stanzas stanza
       | stanza
       ;

stanza: extOPEN container parents extIMPLIES childs extCLOSE
      | extOPEN container extIMPLIES childs extCLOSE
      | extOPEN container extCLOSE
;

container: extTYPE archive
{
  if (!(EXTR_CONTAINER = addContainer(EXTR_COLL, $1, $2))) YYERROR;
}
;

parents: parents parent
       | parent
;

childs: childs child
      | child
;

parent: archive 
{
  if (!addFromArchive(EXTR_COLL, EXTR_CONTAINER, $1)) YYERROR;
}
;

child: archive extSTRING
{
  if (!(addFromAsso(EXTR_COLL, $1, EXTR_CONTAINER, $2))) YYERROR;
}
;

archive: extHASH extCOLON extSIZE
{
  if (!($$ = addArchive(EXTR_COLL, $1, $3))) YYERROR;
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
void extr_error(const char* message)
{
  if(message != NULL) {
    logEmit(LOG_ERR, "%s", message);
    //logEmit(LOG_ERR, "line %-3i %s", extLocalCounter, message);
    //logEmit(LOG_ERR, "(the bad token is: \"%s\")", extractFile_text);
    }
}


/*=======================================================================
 * Function   : parseExtractList
 * Description: Parse a file
 * Synopsis   : int parseExtractList(const char* path, int debugFlag)
 * Input      : const char* path: the file to parse
 * Output     : TRUE on success
=======================================================================*/
int parseExtractFile(Collection* coll, const char* path)
{ 
  int rc = FALSE;
  ExtrBisonSelf parser;
  ExtrExtra extra;
  FILE* inputStream = stdin;

  checkCollection(coll);
  logParser(LOG_NOTICE, "parse %s extraction data", coll->label);

  // initialise scanner
  parser.scanner = NULL;
  if (extr_lex_init(&parser.scanner)) {
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
    logEmit(LOG_DEBUG, "parse extraction file: %s", path);
  }
  extr_set_in(inputStream, parser.scanner);

  // extra scanner data
  extra.lineNo = 1;
  extr_set_extra (&extra, parser.scanner);

  // debug mode for scanner
  extr_set_debug(env.debugLexer, parser.scanner);
  logEmit(LOG_DEBUG, "extr_set_debug = %i", 
	  extr_get_debug(parser.scanner));

  // extra parser data 
  parser.collection = coll;
  parser.container = NULL;
  parser.fromAsso = NULL;

  // call the parser
  if (extr_parse(&parser)) {
    logEmit(LOG_ERR, "extract file parser error on line %i",
	    ((ExtrExtra*)extr_get_extra(parser.scanner))->lineNo);
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
    logEmit(LOG_ERR, "%s", "extract file parser error");
  }
  extr_lex_destroy(parser.scanner);
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "confFile.tab.h"
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
 * Description: unit test the the extract parser
 * Synopsis   : ./utextractFile.tab
 * Input      : -i inputPath
 * Output     : Should display the same content
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = (Configuration*)conf;
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
  if (!parseConfiguration(conf->confFile)) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!expandCollection(coll)) goto error;

  if (!parseExtractFile(coll, inputPath)) goto error;
  if (!serializeExtractTree(coll)) goto error;
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
/* mode: c *//* mode: font-lock */
/* mode: auto-fill */
/* End: */
