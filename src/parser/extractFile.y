/*=======================================================================
 * Project: MediaTeX
 * Module : extract parser
 *
 * extraction meta-data parser.

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche
   
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

/* prologue: =======================================================*/

%code provides {
/*=======================================================================
 * Version: this file is generated by BISON using extractFile.y
 * Project: MediaTeX
 * Module : extraction meta-date parser
 *
 * extract parser.

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche
   
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

#define YYSTYPE EXTR_STYPE
}

%{  
#include "mediatex-config.h"
  /* yyscan_t is pre-defined by mediatex-config.h and will be defined
     by Flex header bellow */
%}

%union {
  off_t  size;
  char   string[MAX_SIZE_STRING+1];
  char   hash[MAX_SIZE_MD5+1];
  EType  type; 
  Archive* archive;
}

%{
// we need YYSTYPE defined first to include the Flex headers
#include "parser/extractFile.h"

#define LINENO extr_get_lineno(yyscanner)
  
void extr_error(yyscan_t yyscanner, Collection* coll, 
		Container* container, const char* message);
%}


/* declarations: ===================================================*/
%defines "parser/extractFile.tab.h"
%output "parser/extractFile.tab.c"
%define api.prefix {extr_}
%define api.pure full
%param {yyscan_t yyscanner}
%parse-param {Collection* coll} {Container* container}
%define parse.error verbose
%verbose
%debug
  
%start file

%token            extrERROR
%token            extrOPEN
%token            extrCLOSE
%token <type>     extrTYPE
%token            extrIMPLIES
%token            extrAS
%token <hash>     extrHASH
%token            extrCOLON
%token <size>     extrSIZE
%token            extrCOMMA
%token <string>   extrSTRING

%type  <archive>  archive

%%

/* grammar rules: ==================================================*/

file: stanzas
    | //empty file
{
  logParser(LOG_WARNING, "the %s' extract file was empty", 
	    coll->label);
}
;

stanzas: stanzas stanza
{
  logParser(LOG_DEBUG, "line %i: stanzas: stanzas stanza", LINENO);
}
       | stanza
{
  logParser(LOG_DEBUG, "line %i: stanzas: stanza", LINENO);
}
;

stanza: extrOPEN container extrIMPLIES childs extrCLOSE
{
  logParser(LOG_DEBUG, "line %i: %s", LINENO, 
	    "stanza: (container => childs)");

}
;

container: orphaneContainer
         | stdContainer
         | stdContainer parents
{
  logParser(LOG_DEBUG, "line %i: stdContainer parents", LINENO);
}
;

orphaneContainer: extrTYPE 
{
  logParser(LOG_DEBUG, "line %i: orphaneContainer: %s", 
	    LINENO, strEType($1));
  switch ($1) {
  case INC:
    container = coll->extractTree->inc;
    break;
  case IMG:
    container = coll->extractTree->img;
    break;
  default:
    logParser(LOG_ERR, "line %i: %s", LINENO, 
	      "only 'INC' or 'IMG' containers do not provide parent");
    YYERROR;
  }
}

stdContainer: extrTYPE archive
{
  logParser(LOG_DEBUG, "line %i: stdContainer: %s archive",
	    LINENO, strEType($1));
  if ($1 == INC || $1 == IMG) {
    logParser(LOG_ERR, "line %i: %s (%s)", LINENO, 
	      "'INC' and 'IMG' containers cannot have parent");
    YYERROR;
  }
  if (!(container = addContainer(coll, $1, $2))) YYERROR;
}
;

parents: parents parent
{
  logParser(LOG_DEBUG, "line %i: parents: parents parent", LINENO);
}
       | parent
{
  logParser(LOG_DEBUG, "line %i: parents: parent", LINENO);
}
;

childs: childs2
      | /*empty*/
{
  if (container->type != INC && container->type != IMG) {
    logParser(LOG_ERR, "line %i: %s", LINENO, 
	      "only 'INC' or 'IMG' containers may do not provide childs");
    YYERROR;
  }
}
;

childs2: childs2 child
{
  logParser(LOG_DEBUG, "line %i: childs: childs child", LINENO);
}
      | child
{
  logParser(LOG_DEBUG, "line %i: childs: child", LINENO);
}
;

parent: archive 
{
  logParser(LOG_DEBUG, "line %i: parent: archive", LINENO);
  if (!addFromArchive(coll, container, $1)) YYERROR;
}
;

child: archive extrSTRING
{
  logParser(LOG_DEBUG, "line %i: child: archive %s", LINENO, $2);
  if (!(addFromAsso(coll, $1, container, $2))) YYERROR;
}
;

archive: extrHASH extrCOLON extrSIZE
{
  logParser(LOG_DEBUG, "line %i: archive: %s:%lli", LINENO, $1, $3);
  if (!($$ = addArchive(coll, $1, $3))) YYERROR;
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
void extr_error(yyscan_t yyscanner, Collection* coll,
		Container* container, const char* message)
{
  logParser(LOG_ERR, "%s on token '%s' line %i",
	  message, extr_get_text(yyscanner), LINENO);
}


/*=======================================================================
 * Function   : parseExtractList
 * Description: Parse an extraction meta-data file
 * Synopsis   : int parseExtractList(Collection* coll, const char* path)
 * Input      : Collection* coll: related collection
 *              const char* path: the file to parse
 * Output     : TRUE on success
=======================================================================*/
int parseExtractFile(Collection* coll, const char* path)
{ 
  int rc = FALSE;
  FILE* inputStream = stdin;
  yyscan_t scanner;
  Container* container = 0;

  checkCollection(coll);
  logParser(LOG_INFO, "parse %s extraction data from %s",
	    coll->label, path?path:"stdin");

  // initialise scanner
  if (extr_lex_init(&scanner)) {
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

  extr_set_in(inputStream, scanner);

  // debug mode for scanner
  extr_set_debug(env.debugLexer, scanner);
  logParser(LOG_DEBUG, "extr_set_debug = %i", extr_get_debug(scanner));

  // call the parser
  if (extr_parse(scanner, coll, container)) {
    logParser(LOG_ERR, "extract parser fails on line %i",
	    extr_get_lineno(scanner));
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
    logParser(LOG_ERR, "%s", "extract parser error");
  }
  extr_lex_destroy(scanner);
  return rc;
}

/* Local Variables: */
/* mode: c *//* mode: font-lock */
/* mode: auto-fill */
/* End: */
