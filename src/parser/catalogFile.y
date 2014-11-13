/* prologue: =======================================================*/
%{
  /*=======================================================================
   * Version: $Id: catalogFile.y,v 1.2 2014/11/13 16:36:57 nroche Exp $
   * Project: MediaTeX
   * Module : catalogFile parser
   *
   * catalog parser

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
#include "../misc/locks.h"
#include "../memory/ardsm.h"
#include "../memory/catalogTree.h"
#include "catalogFile.h"

void cata_error(const char* message);

// scanner + anything we may want
// (parameter type of reentrant 'yyparse' function)
typedef struct CataBisonSelf {
  yyscan_t    scanner;
  // ... anything we want here
  Collection* collection;
  Category*   category;
  Human*      human;
  Document*   document;
  Archive*    archive;
} CataBisonSelf;

// name of the belows parameter into bison: must use a common name
// define YYLEX yylex (&yylval, YYLEX_PARAM)
#define YYPARSE_PARAM cataBisonSelf
#define YYLEX_PARAM   ((CataBisonSelf*)cataBisonSelf)->scanner

// shortcut maccros
#define CATA_LINE_NO  ((CataExtra*)cata_get_extra(YYLEX_PARAM))->lineNo
#define CATA_COLL     ((CataBisonSelf*)cataBisonSelf)->collection
#define CATA_CATEGORY ((CataBisonSelf*)cataBisonSelf)->category
#define CATA_HUMAN    ((CataBisonSelf*)cataBisonSelf)->human
#define CATA_DOCUMENT ((CataBisonSelf*)cataBisonSelf)->document
#define CATA_ARCHIVE  ((CataBisonSelf*)cataBisonSelf)->archive
%}

%code provides {
  #include "../memory/catalogTree.h"
  int parseCatalogFile(Collection* coll, const char* path);
}

/* declarations: ===================================================*/
%file-prefix="catalogFile"
%defines
%debug
%name-prefix="cata_"
%error-verbose
%verbose
%define api.pure

%start file

   /*%token            catEOL */
%token            catHUMCARAC
%token            catCONCARAC
%token            catRECCARAC
%token            catDOCCARAC
%token            catTOP
%token            catCATEGORY
%token            catROLE
%token            catHUMAN
%token            catDOCUMENT
%token            catWITH
%token            catARCHIVE
%token            catCOLON
%token            catEQUAL
%token            catDOT
%token            catCOMMA
%token <hash>     catHASH
%token <size>     catNUMBER
%token <string>   catSTRING
%token            catERROR

%type <category_t> newCategory
%type <category_t> category
%type <human_t>    human
%type <document_t> document
%type <archive_t>  archive     

%%

/* grammar rules: ==================================================*/

file: stanzas 
    | //empy file 
{
  logParser(LOG_WARNING, "%s", "the catalog file was empty");
}

stanzas: stanzas stanza
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "stanzas: stanzas stanza");
}
       | stanza
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "stanzas: stanza");
}
;

stanza: defCategory
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "stanza: defCategory");
}
      | defHuman
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "stanza: defHuman");
}
      | defDocument
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "stanza: defDocument");
}
      | defArchive
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "stanza: deFaRCHIVE");
}
;

/* Carac */

cateCaracs: cateCaracs cateCarac
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "cateCaracs: cateCaracs cateCarac");
}
          | cateCarac
{
 logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "cateCaracs: cateCarac");
}

cateCarac: catSTRING catEQUAL catSTRING
{
  Carac* carac = NULL;
  logParser(LOG_INFO, "line %i: cateCarac: %s = %s", CATA_LINE_NO, $1, $3);
  if (!(carac = addCarac(CATA_COLL, $1))) YYERROR;
  if (!addAssoCarac(CATA_COLL, carac, CATE, CATA_CATEGORY, $3)) YYERROR;
}

docCaracs: docCaracs docCarac
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "docCaracs: docCaracs docCarac");
}
         | docCarac
{
 logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "docCaracs: docCarac");
}

docCarac: catSTRING catEQUAL catSTRING
{
  Carac* carac = NULL;
  logParser(LOG_INFO, "line %i: docCarac: %s = %s", CATA_LINE_NO, $1, $3);
  if (!(carac = addCarac(CATA_COLL, $1))) YYERROR;
  if (!addAssoCarac(CATA_COLL, carac, DOC, CATA_DOCUMENT, $3)) YYERROR;
}

humCaracs: humCaracs humCarac
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "humCaracs: humCaracs humCarac");
}
         | humCarac
{
 logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "humCaracs: humCarac");
}

humCarac: catSTRING catEQUAL catSTRING
{
  Carac* carac = NULL;
  logParser(LOG_INFO, "line %i: humCarac: %s = %s", CATA_LINE_NO, $1, $3);
  if (!(carac = addCarac(CATA_COLL, $1))) YYERROR;
  if (!addAssoCarac(CATA_COLL, carac, HUM, CATA_HUMAN, $3)) YYERROR;
}

archCaracs: archCaracs archCarac
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "archCaracs: archCaracs archCarac");
}
         | archCarac
{
 logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "archCaracs: archCarac");
}

archCarac: catSTRING catEQUAL catSTRING
{
  Carac* carac = NULL;
  logParser(LOG_INFO, "line %i: archCarac: %s = %s", CATA_LINE_NO, $1, $3);
  if (!(carac = addCarac(CATA_COLL, $1))) YYERROR;
  if (!addAssoCarac(CATA_COLL, carac, ARCH, CATA_ARCHIVE, $3)) YYERROR;
}

/* defCategory */

defCategory: newCategory catCOLON categories cateCaracs
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "newCategory : category caracs");
  CATA_CATEGORY = NULL;
}
            | newCategory catCOLON categories
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "newCategory : category");
  CATA_CATEGORY = NULL;
}
            | newCategory cateCaracs
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "newCategory cateCaracs");
  CATA_CATEGORY = NULL;
}
            | newCategory
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "newCategory");
  CATA_CATEGORY = NULL;
}
;

newCategory: catTOP catCATEGORY catSTRING
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "newCategory: TOP CATEGORY category");
  if (!($$ = addCategory(CATA_COLL, $3, TRUE))) YYERROR;
  CATA_CATEGORY = $$;
}
           | catCATEGORY catSTRING
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "newCategory: CATEGORY category");
  if (!($$ = addCategory(CATA_COLL, $2, FALSE))) YYERROR;
  CATA_CATEGORY = $$;
}

categories: categories catCOMMA category
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "categories: categories , category");
  if (!addCategoryLink(CATA_COLL, $3, CATA_CATEGORY)) YYERROR;
}
          | category
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "categories: category");
  if (!addCategoryLink(CATA_COLL, $1, CATA_CATEGORY)) YYERROR;
}
;

category: catSTRING
{
  logParser(LOG_INFO, "line %i: category: %s", CATA_LINE_NO, $1);
  if (!($$ = addCategory(CATA_COLL, $1, FALSE))) YYERROR;
}
;

/* defHuman */

defHuman: newHuman catCOLON humCategories humCaracs
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "defHuman: newHuman catCOLON humCategories humCaracs");
  CATA_HUMAN = NULL;
}
        | newHuman catCOLON humCategories
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "defHuman: newHuman : humCategories");
  CATA_HUMAN = NULL;
}
        | newHuman humCaracs
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "defHuman: newHuman humCaracs");
  CATA_HUMAN = NULL;
}
        | newHuman
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "defHuman: newHuman");
  CATA_HUMAN = NULL;
}
;

newHuman: catHUMAN human
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "newHuman: catHUMAN human");
  CATA_HUMAN = $2;
}
;

human: catSTRING catSTRING
{
    logParser(LOG_INFO, "line %i: human: %s %s", CATA_LINE_NO, $1, $2);
    if (!($$ = addHuman(CATA_COLL, $1, $2))) YYERROR;
}
;

humCategories: humCategories catCOMMA category
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO,
	    "humCategories: humCategories , category");
  if (!addHumanToCategory(CATA_COLL, CATA_HUMAN, $3)) YYERROR;
}
          | category
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "humCategories: category");
  if (!addHumanToCategory(CATA_COLL, CATA_HUMAN, $1)) YYERROR;
}
;

/* defArchive */

defArchive: newArchive archCaracs
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "defArchive: newArchive caracs");
  CATA_ARCHIVE = NULL;
}
         | newArchive
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "defArchive: neWArchive");
  CATA_ARCHIVE = NULL;
}
;

newArchive: catARCHIVE archive
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "newArchive: catARCHIVE archive");
  CATA_ARCHIVE = $2;
}
;

archive: catHASH catCOLON catNUMBER
{
  logParser(LOG_INFO, "line %i: archive: %s:%lli", CATA_LINE_NO, $1, $3);
  if (!($$ = addArchive(CATA_COLL, $1, $3))) YYERROR;
}
;

/* defDocument */

defDocument: newDocument docCategories2 docWiths docCaracs2 docArchives 
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "defDocument: newDocument docCategories "
	    "docWiths docCaracs docArchives");
  CATA_DOCUMENT = NULL;
}
;

newDocument: catDOCUMENT document
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "newDocument: catDOCUMENT document");
  CATA_DOCUMENT = $2;
}
;

document: catSTRING
{
  logParser(LOG_INFO, "line %i: document: %s", CATA_LINE_NO, $1);
  if (!($$ = addDocument(CATA_COLL, $1))) YYERROR;
}
;

docCategories2: catCOLON docCategories
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "docCategories2: : docCategories");
}
             | 
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "docCategories: nil");
}
;

docCategories: docCategories catCOMMA category
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "docCategories: docCategories , category");
  if (!addDocumentToCategory(CATA_COLL, CATA_DOCUMENT, $3)) YYERROR;
}
          | category
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "docCategories: category");
  if (!addDocumentToCategory(CATA_COLL, CATA_DOCUMENT, $1)) YYERROR;
}
;

docCaracs2: docCaracs
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "docCaracs2: docCaracs");
}
          | 
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, 
	    "docCaracs2: (NULL)");
}
;

docWiths: roles
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "docWiths: roles");
}
       | 
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "docWiths: (NULL)");
}

roles: roles role
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "roles: roles role");
}
      | role
{
 logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "roles: role");
}

role: catWITH catSTRING catEQUAL human
{
  Role* role = NULL;
  logParser(LOG_INFO, "line %i: role: WITH %s = human", CATA_LINE_NO, $2);
  if (!(role = addRole(CATA_COLL, $2))) YYERROR;
  if (!addAssoRole(CATA_COLL, role, $4, CATA_DOCUMENT)) YYERROR;
}

docArchives: archives
{
  logParser(LOG_INFO, "line %i: docArchives: archives", CATA_LINE_NO);
}
          | 
{
  logParser(LOG_INFO, "line %i: docArchives: (NULL)", CATA_LINE_NO);
}

archives: archives archive
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO,
	    "archives: archives archive");
  if (!addArchiveToDocument(CATA_COLL, $2, CATA_DOCUMENT)) YYERROR;
}
        | archive
{
  logParser(LOG_INFO, "line %i: %s", CATA_LINE_NO, "archives: archive");
 if (!addArchiveToDocument(CATA_COLL, $1, CATA_DOCUMENT)) YYERROR;
}

%%

/* epilogue: =======================================================*/


/*=======================================================================
 * Function   : catalogFile_error
 * Description: Emit an error message. Called by the parser on error
 *              state.
 * Synopsis   : void catalogFile_error(char* message)
 * Input      : char* message = the error message.
 * Output     : N/A
 =======================================================================*/

void cata_error(const char* message)
{
  if(message != NULL) {
    logEmit(LOG_ERR, "%s", message);
    //logEmit(LOG_ERR, "line %lu, %s", CATA_LINE_NO, message);
    //logEmit(LOG_ERR, "(the bad token is: \"%s\")", catalogFile_text);
  }
}


/*=======================================================================
 * Function   : parseCatalogFile
 * Description: Parse the catalog
 * Synopsis   : int parseCatalogTree(const char* path, int debugFlag)
 * Input      : const char* catalog: the catalog path
 * Output     : TRUE on success
=======================================================================*/
int parseCatalogFile(Collection* coll, const char* path)
{
  int rc = FALSE;
  CataBisonSelf parser;
  CataExtra extra;
  FILE* inputStream = stdin;

  checkCollection(coll);
  logParser(LOG_NOTICE, "parse %s catalog", coll->label);

  // initialise scanner
  parser.scanner = NULL;
  if (cata_lex_init(&parser.scanner)) {
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
    logEmit(LOG_DEBUG, "parse catalog file: %s", path);
  }
  cata_set_in(inputStream, parser.scanner);

  // cata scanner data
  extra.lineNo = 1;
  cata_set_extra (&extra, parser.scanner);

  // debug mode for scanner
  cata_set_debug(env.debugLexer, parser.scanner);
  logEmit(LOG_DEBUG, "cata_set_debug = %i", 
	  cata_get_debug(parser.scanner));

  // extra parser data 
  parser.collection = coll;
  parser.category = NULL;
  parser.human = NULL;
  parser.document = NULL;
  parser.archive = NULL;

  // call the parser
  if (cata_parse(&parser)) {
    logEmit(LOG_ERR, "catalog file parser error on line %i",
	    ((CataExtra*)cata_get_extra(parser.scanner))->lineNo);
    logEmit(LOG_ERR, "please edit %s", path);
    goto error;
  }

  if (inputStream != stdin) {
    if (!unLock(fileno(inputStream))) goto error;
    fclose(inputStream);
  }
  rc = TRUE;
 error:
  cata_lex_destroy(parser.scanner);
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
 * Description: unitary test for this parser module
 * Synopsis   : ./utcatalogFile.tab
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

  if (!parseCatalogFile(coll, inputPath)) goto error;
  if (!serializeCatalogTree(coll)) goto error;
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
