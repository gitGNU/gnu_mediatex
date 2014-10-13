/* prologue: =======================================================*/
%{
  /*=======================================================================
   * Version: $Id: recordList.y,v 1.1 2014/10/13 19:39:49 nroche Exp $
   * Project: MediaTeX
   * Module : recordList parser
   *
   * recordList parser

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
#include "../memory/recordTree.h"
#include "recordList.h"
  
#include <sys/socket.h> // inet_ntoa
#include <netinet/in.h>
#include <arpa/inet.h>

void record_error(const char* message);

typedef struct RecordBisonSelf {
  yyscan_t      scanner;
  RecordTree*   rc;
  //Record*       record;
  int           err;
  char*         string;
} RecordBisonSelf;
  
  // the parameter name (of the reentrant 'yyparse' function)
  // data is a pointer to a 'BisonSelf' structure
#define YYPARSE_PARAM recordBisonSelf
#define YYLEX_PARAM   ((RecordBisonSelf*)recordBisonSelf)->scanner
  
// shortcut maccros
#define RECORD_LINE_NO ((RecordExtra*)record_get_extra(YYLEX_PARAM))->lineNo
#define RECORD_TREE    ((RecordBisonSelf*)recordBisonSelf)->rc

//#define RC            ((BisonSelf*)data)->err
//#define STRING        ((BisonSelf*)data)->string
//#define DATE          ((BisonSelf*)data)->time
//#define IPV4          ((BisonSelf*)data)->ipv4
//#define TREE          ((BisonSelf*)data)->rc
%}

%code provides {
  #include "../memory/recordTree.h"
  extern RecordTree* parseRecordList(int inputPath);
}

/* declarations: ===================================================*/
%file-prefix="recordList"
%defines
%debug
%name-prefix="record_"
%error-verbose
%verbose
%define api.pure

%start file

   /*%token            recordEOL */
%token          recordHEADER
%token          recordBODY
%token          recordCOLL
%token          recordDOCYPHER
%token          recordSERVER
%token          recordMSGTYPE
%token <string> recordSTRING

%token <type>   recordTYPE
%token <time>   recordDATE
%token <hash>   recordHASH
%token <size>   recordSIZE
%token <string> recordPATH
%token <msgval> recordMSGVAL
%token          recordERROR

%type <record>  line

%%

/* grammar rules: ==================================================*/

file: //empty file 
{
  logParser(LOG_WARNING, "%s", "the records file was empty");
} 
    | header
    | header lines
; 

header: recordHEADER hLines recordBODY
{
  // enable encryption for body if needed
  RECORD_TREE->aes.doCypher = RECORD_TREE->doCypher;
}
;

hLines: hLines hLine
     | hLine

hLine: recordCOLL recordSTRING
{
  Collection* coll = NULL;
  if (!(coll = getCollection($2))) {
    logParser(LOG_WARNING, "unknown collection: %s", $2);
    YYABORT;
  }
  destroyString($2);
  RECORD_TREE->collection = coll;
#ifndef utMAIN
  aesInit(&RECORD_TREE->aes, coll->serverTree->aesKey, DECRYPT);
#else
  aesInit(&RECORD_TREE->aes, "1000000000000000", DECRYPT);
#endif
}
     | recordSERVER recordSTRING
{
  strncpy(RECORD_TREE->fingerPrint, $2, MAX_SIZE_HASH);
  destroyString($2);
}
     | recordMSGTYPE recordMSGVAL
{
    RECORD_TREE->messageType = $2;
}
     | recordDOCYPHER recordSTRING
{
  if (!strcmp($2, "TRUE")) {
    RECORD_TREE->doCypher = TRUE;
  }
  destroyString($2);
}
;

lines: lines newLine
     | newLine

newLine: line
{
  logParser(LOG_DEBUG, "%s", "add new record");

  struct tm date;
  if (localtime_r(&$1->date, &date) == (struct tm*)0) {
    logParser(LOG_ERR, "%s", "localtime_r returns on error");
    YYABORT;
  }

  logParser(LOG_DEBUG, "line %i: %c \
%04i-%02i-%02i,%02i:%02i:%02i \
%*s %*s %*lli %s", 
	    RECORD_LINE_NO, 
	    //($1->type & REMOVE)?'-':' ', // (no more used)
	    ($1->type & 0x3) == DEMAND?'D':($1->type & 0x3) == SUPPLY?'S':'?',
	    date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	    date.tm_hour, date.tm_min, date.tm_sec,
	    MAX_SIZE_HASH, $1->server->fingerPrint, 
	    MAX_SIZE_HASH, $1->archive->hash, 
	    MAX_SIZE_SIZE, (long long int)$1->archive->size, 
	    $1->extra?$1->extra:"");
 
  if (!rgInsert(RECORD_TREE->records, $1)) YYERROR;
  // RQ: YYABORT will close the socket, do YYERROR close it ?
  $1 = NULL;

}
;

line: recordTYPE recordDATE recordHASH recordHASH recordSIZE recordPATH
{
  Server* server = NULL;
  Archive* archive = NULL;
  logParser(LOG_DEBUG, "%s", "new record");

  /* TODO: do we refuse unknown servers or not ?
  if (!(server = getServer(RECORD_TREE->collection, $3))) {
    logEmit(LOG_ERR, "unknown server: %s", $3);
    YYERROR;
  }
  */
  if (!(server = addServer(RECORD_TREE->collection, $3))) {
    logEmit(LOG_ERR, "unknown server: %s", $3);
    YYERROR;
  }

  if (!(archive = addArchive(RECORD_TREE->collection, $4, $5))) 
    YYERROR;

  if (!($$ = 
	addRecord(RECORD_TREE->collection, server, archive, $1, $6)))
    YYABORT;
  $$->date = $2;
}

%%

/* epilogue: =======================================================*/



/*=======================================================================
 * Function   : yyerror
 * Description: Emit an error message. Called by the parser on error
 *              state.
 * Synopsis   : void parsererror(char* message)
 * Input      : char* message = the error message.
 * Output     : N/A
 =======================================================================*/
void record_error(const char* message)
{
  if(message != NULL) {
      logEmit(LOG_ERR, "parser error: %s", message);
    }
}


/*=======================================================================
 * Function   : parseRecordList
 * Description: Parse a file
 * Synopsis   : int parseRecordList(const char* path)
 * Input      : const char* path: the file to parse
 * Output     : TRUE on success
=======================================================================*/
RecordTree* parseRecordList(int fd)
{ 
  RecordTree* rc = NULL;
  RecordTree* tree = NULL;
  RecordBisonSelf parser;
  RecordExtra extra;

  logParser(LOG_NOTICE, "%s", "parse record list");
  parser.scanner = NULL;

  // scanner input file
  if (fd == -1) {
    logEmit(LOG_ERR, "%s", 
	    "recordList parser need a valid inputPath parameter");
    goto error;
  }
  
  // initialise parser
  if (record_lex_init(&parser.scanner)) {
    logEmit(LOG_ERR, "%s", "error initializing scanner");
    goto error;
  }

  // debug mode for scanner
  record_set_debug(env.debugLexer, parser.scanner);
  logEmit(LOG_DEBUG, "record_set_debug = %i", 
	  record_get_debug(parser.scanner));

  /* // scan stdin or input file if defined */
  /* record_set_in(fd, parser.scanner); */

  // initialise parser
  //parser.record = NULL;
  parser.string = NULL;
  if ((parser.rc = createRecordTree()) == NULL)
      goto error2;

  // use extra data with reentrant scanner
  extra.aesData = &parser.rc->aes;
  extra.aesData->fd = fd;
  extra.aesData->way = DECRYPT;

  extra.lineNo = 1;
  record_set_extra (&extra, parser.scanner);

  // call the parser
  if (record_parse(&parser)) {
    logEmit(LOG_ERR, "catalog file parser error on line %i",
	    ((RecordExtra*)record_get_extra(parser.scanner))->lineNo);
    goto error2;
  }
  tree = parser.rc;

  // set aes data for default serialization
  tree->aes.fd = STDOUT_FILENO;
  tree->aes.way = ENCRYPT;
  if (!aesInit(&tree->aes, "1000000000000000", ENCRYPT)) goto error;

  rc = tree;
  tree = NULL;
 error2:
  destroyRecordTree(tree);
  record_lex_destroy(parser.scanner);
 error:  
  return rc;
}


/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "confFile.tab.h"
#include "serverFile.tab.h"

#include <sys/types.h> // 
#include <sys/stat.h>  // open 
#include <fcntl.h>     //
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
 * modif      : 2012/12/12
 * Description: unitary test for this parser module
 * Synopsis   : ./utrecordList.tab
 * Input      : stdin
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = (Configuration*)conf;
  Collection* coll = NULL;
  RecordTree* rcTree = NULL;
  char* inputPath = NULL;
  char* outputPath = NULL;
  int inputFd = -1;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = PARSER_SHORT_OPTIONS"i:o:";
  struct option longOptions[] = {
    {"input", required_argument, NULL, 'i'},
    {"output", required_argument, NULL, 'o'},
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

    case 'o':
      if(isEmptyString(optarg)) {
	fprintf(stderr, 
		"%s: nil or empty argument for the output stream\n",
		programName);
	rc = 2;
      }
      else {
	if ((outputPath = (char*)malloc(sizeof(char) * strlen(optarg) + 1))
	    == NULL) {
	  fprintf(stderr, 
		  "%s: cannot allocate memory for the output stream name\n", 
		  programName);
	  rc = 3;
	}
	else {
	  strcpy(outputPath, optarg);
	}
      }
      break;

      GET_PARSER_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  env.logSeverity = "debug"; // add comments on md5sum file
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(conf = getConfiguration())) goto error;
  if (!parseConfiguration(conf->confFile)) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!expandCollection(coll)) goto error;
  if (!parseServerFile(coll, coll->serversDB)) goto error;

  if (!isEmptyString(inputPath)) {
    if ((inputFd = open(inputPath, O_RDONLY)) == -1) {
      logEmit(LOG_ERR, "cannot open input file: %s", inputPath); 
      goto error;
    }
  }

  if (!(rcTree = parseRecordList(inputFd))) goto error;
  
  // serialize the resulting tree :
  if (isEmptyString(outputPath)) {
    // on stdout: do not crypt the crypted input
    rcTree->doCypher = FALSE; 
  }
  else {
    // disable stdout by default because we need no ending cariage return
    env.dryRun = FALSE; 
  }

  if (!serializeRecordTree(rcTree, outputPath, NULL)) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  ENDINGS;
  freeConfiguration();
  destroyRecordTree(rcTree);
  destroyString(inputPath);
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

 
