/* prologue: =======================================================*/
%{
  /*=======================================================================
   * Version: $Id: shellQuery.y,v 1.1 2014/10/13 19:38:49 nroche Exp $
   * Project: Mediatex
   * Module : shell parser
   *
   * shellQuery parser.

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

#include "shellQuery.h"
#include "../memory/confTree.h"
#ifndef utMAIN
#include "../misc/command.h"
#include "../misc/setuid.h"
#include "../parser/supportFile.tab.h"
#include "../common/register.h"
#include "../common/openClose.h"
#include "serv.h"
#include "conf.h"
#include "supp.h"
#include "motd.h"
#include "misc.h"
 
  extern int debugFlag;
  extern int parseConfiguration(const char* path);
#endif

  
  void shellQuery_error(const char* message);

  typedef struct BisonSelf {
    yyscan_t scanner;
    Collection* coll;
  } BisonSelf;
  
  // the parameter name (of the reentrant 'yyparse' function)
  // data is a pointer to a 'BisonSelf' structure
#define YYPARSE_PARAM data
  
  // the argument for the 'yylex' function
#define YYLEX_PARAM   ((BisonSelf*)data)->scanner
#define COLL ((BisonSelf*)data)->coll
%}
 
%code provides {
  extern int shellQuery_parse(void*);
  int parseShellQuery(int argc, char** argv, int optind);
}

/* declarations: ===================================================*/
%file-prefix="shellQuery"
%defines
%debug
%name-prefix="shellQuery_"
%error-verbose
%verbose
%define api.pure
 
%start queries

   /*%token            shellEOL */
%token <string>   shellERROR
%token <string>   shellSTRING
%token <number>   shellNUMBER
%token            shellEOL

%token            shellCOLL
%token            shellSUPP
%token            shellKEY
%token            shellUSER

%token            shellADMIN
%token            shellSERVER

%token            shellALL
%token            shellTO
%token            shellFROM
%token            shellAS
%token            shellON

%token            shellINIT
%token            shellREMOVE
%token            shellPURGE
%token            shellCLEAN
%token            shellADD
%token            shellDEL
%token            shellNOTE
%token            shellLIST
%token            shellUPDATE
%token            shellUPGRADE
%token            shellCOMMIT
%token            shellMAKE
%token            shellCHECK
%token            shellBIND
%token            shellUNBIND
%token            shellMOUNT
%token            shellUMOUNT
%token            shellUPLOAD
%token            shellGET
%token            shellSU
%token            shellMOTD
%token            shellDELIVER
%token            shellSAVE
%token            shellEXTRACT
%token            shellNOTIFY

%token            shellAROBASE
%token            shellCOLON
%token            shellMINUS

%type <coll>      newCollection
%type <string>    collection
%type <string>    support
%type <string>    key

%%
   /* grammar rules: ==================================================*/

 /* objects */

collection: shellCOLL shellSTRING
{
  strcpy($$, $2);
};

support: shellSUPP shellSTRING
{
  strcpy($$, $2);
};

key: shellKEY shellSTRING
{
  strcpy($$, $2);
};

newCollection: newCollection2 newCollUser newCollHost newCollPort
{
  $$ = COLL;
  COLL = NULL;
};

newCollection2: shellCOLL
{
  // addCollection will be call later
  if (!(COLL = createCollection())) YYABORT;
};

newCollUser: newCollConf shellMINUS newCollLabel
           | newCollLabel
;

newCollConf: shellSTRING
{
  if (!(COLL->masterLabel = createString($1))) YYABORT;
};

newCollLabel: shellSTRING
{
  strncpy(COLL->label, $1, MAX_SIZE_COLL);
};

newCollHost: shellAROBASE shellSTRING 
{
  strncpy(COLL->masterHost, $2, MAX_SIZE_HOST);
}
             | // empty
;

newCollPort: shellCOLON shellNUMBER
{
  COLL->masterPort = $2;
}
             | // empty
;

 /* main rules */

queries: query
       | shellERROR 
{
  shellQuery_error($1); 
  YYABORT;
}
       | shellEOL /* empty query */
{
  fprintf(stderr, "please use the -h flag to get help\n");
}
;

query: shellADMIN admConfQuery
	  /* mdtx adm init
	     mdtx adm remove
	     mdtx adm purge 
	     mdtx adm add coll COLL@HOST:PORT
	     mdtx adm del coll COLL
	     mdtx adm update coll COLL
	     mdtx adm update all
	     mdtx adm commit coll COLL
	     mdtx adm commit all
	     mdtx adm make coll COLL
	     mdtx adm make all
	     mdtx adm bind 
	     mdtx adm unbind
	     mdtx adm mount ISO on PATH
	     mdtx adm umount PATH
	     mdtx get LOGIN@HOST:/PATH as PATH */
     | shellSERVER srvQuery
          /* mdtx srv save
	     mdtx src scan
	     mdtx srv extract
	     mdtx srv notify */
     | apiQuery
;
        
apiQuery: apiSuppQuery
	  /* mdtx new supp SUPP on PATH
	     mdtx del supp SUPP
	     mdtx add supp SUPP to coll COLL
	     mdtx del supp SUPP from COLL
	     mdtx note supp as TEXT
	     mdtx check supp on PATH 
             mdtx upload PATH to coll COLL */
        | apiCollQuery
	  /* mdtx add key PATH to coll COLL
	     mdtx del key FINGERPRINT from coll COLL
	     mdtx add proxy FINGERPRINT to coll COLL
	     mdtx del proxy FINGERPRINT from coll COLL 
	     mdtx list coll
	     mdtx list supp
	     mdtx motd
	     mdtx deliver
	     mdtx upgrade coll COLL 
	     mdtx upgrade
	     mdtx make coll COLL
	     mdtx make
	     mdtx clean coll COLL
	     mdtx clean
	     mdtx su
	     mdtx su coll COLL */


 /* to call scripts directely (for daemon, admin or expert user) */

admConfQuery: shellINIT shellEOL
{
  logParser(LOG_NOTICE, "%s", "initializing mdtx software");
#ifndef utMAIN
  if (!mdtxInit()) YYABORT;
#endif
}
            | shellREMOVE shellEOL
{
  logParser(LOG_NOTICE, "%s", "removing mdtx software");
#ifndef utMAIN
  if (!mdtxRemove()) YYABORT;
#endif
}
            | shellPURGE shellEOL
{
  logParser(LOG_NOTICE, "%s", "purging mdtx software");
#ifndef utMAIN
  if (!mdtxPurge()) YYABORT;
#endif
}
            | shellADD shellUSER shellSTRING shellEOL
{
  logParser(LOG_NOTICE, "add %s user to groups", $3);
#ifndef utMAIN
  if (!mdtxAddUser($3)) YYABORT;
#endif
}
            | shellDEL shellUSER shellSTRING shellEOL
{
  logParser(LOG_NOTICE, "del %s user from groups", $3);
#ifndef utMAIN
  if (!mdtxDelUser($3)) YYABORT;
#endif
}
            | shellADD newCollection shellEOL
{
  int rc = TRUE;
  logParser(LOG_NOTICE, "create/join new %s collection", $2->label);
#ifndef utMAIN
  rc=rc&& clientWriteLock();
  rc=rc&& mdtxAddCollection($2);
  rc=rc&& clientWriteUnlock();
#endif
  destroyCollection($2);
  if (!rc) YYABORT;
}
            | shellDEL collection shellEOL
{
  logParser(LOG_NOTICE, "free the %s collection", $2);
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxDelCollection($2)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellUPDATE shellEOL
{
  logParser(LOG_NOTICE, "%s", "update all");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!clientLoop(mdtxUpdate)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellUPDATE collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "update collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxUpdate($2)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellCOMMIT shellEOL
{
  logParser(LOG_NOTICE, "%s", "commit all");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!clientLoop(mdtxCommit)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellCOMMIT collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "commit collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxCommit($2)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellMAKE shellEOL
{
  logParser(LOG_NOTICE, "%s", "quickly make all");
#ifndef utMAIN
  env.noCollCvs = TRUE; // disable update/commit
  if (!clientLoop(mdtxMake)) YYABORT;
#endif
}
            | shellMAKE collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "quickly make collection");
#ifndef utMAIN
  env.noCollCvs = TRUE;
  if (!mdtxMake($2)) YYABORT;
#endif
}
            | shellBIND shellEOL
{
  logParser(LOG_NOTICE, "%s", "bind mdtx directories");
#ifndef utMAIN
  if (!allowedUser(env.confLabel)) YYABORT;
  if (!mdtxBind()) YYABORT;
#endif
}
            | shellUNBIND shellEOL
{
  logParser(LOG_NOTICE, "%s", "unbind mdtx directories");
#ifndef utMAIN
  if (!allowedUser(env.confLabel)) YYABORT;
  if (!mdtxUnbind()) YYABORT;
#endif
}
            | shellMOUNT shellSTRING shellON shellSTRING shellEOL
{
  logParser(LOG_NOTICE, "mount %s on %s", $2, $4);
#ifndef utMAIN
  if (!allowedUser(env.confLabel)) YYABORT;
  if (!mdtxMount($2, $4)) YYABORT;
#endif
}
            | shellUMOUNT shellSTRING shellEOL
{
  logParser(LOG_NOTICE, "umount %s", $2);
#ifndef utMAIN
  if (!allowedUser(env.confLabel)) YYABORT;
  if (!mdtxUmount($2)) YYABORT;
#endif
}
            | shellGET shellSTRING shellAS shellSTRING shellON 
	    shellSTRING shellEOL
{
  logParser(LOG_NOTICE, "get %s as %s on %s", $2, $4, $6);
#ifndef utMAIN
  if (!allowedUser(env.confLabel)) YYABORT;
  env.noCollCvs = TRUE; // do not upgrade
  if (!mdtxScp($4, $6, $2)) YYABORT;
#endif
}
;

 /* server queries (sending signals to server) */

srvQuery: shellSAVE shellEOL
{
  logParser(LOG_NOTICE, "%s", "send SAVEMD5 signal to daemon");
#ifndef utMAIN
  if (!allowedUser(env.confLabel)) YYABORT;
  if (!mdtxSyncSignal(MDTX_SAVEMD5)) YYABORT;
#endif
}
        | shellEXTRACT shellEOL
{
  logParser(LOG_NOTICE, "%s", "send EXTRACT signal to daemon");
#ifndef utMAIN
  if (!allowedUser(env.confLabel)) YYABORT;
  if (!mdtxSyncSignal(MDTX_EXTRACT)) YYABORT;
#endif
}
        | shellNOTIFY shellEOL
{
  logParser(LOG_NOTICE, "%s", "send NOTIFY signal to daemon");
#ifndef utMAIN
  if (!allowedUser(env.confLabel)) YYABORT;
  if (!mdtxSyncSignal(MDTX_NOTIFY)) YYABORT;
#endif
}
        | shellDELIVER shellEOL
{
  logParser(LOG_NOTICE, "%s", "send DELIVER signal to daemon");
#ifndef utMAIN
  if (!allowedUser(env.confLabel)) YYABORT;
  if (!mdtxSyncSignal(MDTX_DELIVER)) YYABORT;
#endif
}
;

 /* support queries API */

apiSuppQuery: shellADD support shellON shellSTRING shellEOL
{
  logParser(LOG_NOTICE, "add %s support on %s", $2, $4);
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxAddSupport($2, $4)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellDEL support shellEOL
{
  logParser(LOG_NOTICE, "%s", "remove a support");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxDelSupport($2)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellLIST shellSUPP shellEOL
{
  logParser(LOG_NOTICE, "%s", "list local supports");
#ifndef utMAIN
  if (!mdtxLsSupport()) YYABORT;
#endif
}
            | shellADD support shellTO shellALL shellEOL
{
  logParser(LOG_NOTICE, "%s", "add a support to all collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxShareSupport($2, NULL)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellADD support shellTO collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "add a support to a collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxShareSupport($2, $4)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellDEL support shellFROM shellALL shellEOL
{
  logParser(LOG_NOTICE, "%s", "del a support from all collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxWithdrawSupport($2, NULL)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellDEL support shellFROM collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "del a support from a collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxWithdrawSupport($2, $4)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
           | shellNOTE support shellAS shellSTRING shellEOL
{
  logParser(LOG_NOTICE, "note a support as \"%s\"", $4);
#ifndef utMAIN
  if (!mdtxUpdateSupport($2, $4)) YYABORT;
#endif
}
           | shellCHECK support shellON shellSTRING shellEOL
{
  logParser(LOG_NOTICE, "check content located at %s", $4);
#ifndef utMAIN
  if (!mdtxHaveSupport($2, $4)) YYABORT;
#endif
}
            | shellUPLOAD shellSTRING shellTO collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "upload a file to a collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxUploadFile($4, $2)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
;

 /* collection queries API */

apiCollQuery: shellADD key shellTO collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "add a key to a collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!addKey($4, $2)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellDEL key shellFROM collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "del a key to a collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!delKey($4, $2)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellLIST shellCOLL shellEOL
{
  logParser(LOG_NOTICE, "%s", "listing enabled collections");
#ifndef utMAIN
  if (!mdtxListCollection()) YYABORT;
#endif
}
            | shellMOTD shellEOL
{
  logParser(LOG_NOTICE, "%s", "message of the day");
#ifndef utMAIN
  if (!updateMotd()) YYABORT;
#endif
}
            | shellUPGRADE shellEOL
{
  logParser(LOG_NOTICE, "%s", "upgrade all");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!clientLoop(mdtxUpgrade)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellUPGRADE collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "upgrade a collection");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxUpgrade($2)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellMAKE shellEOL
{
  logParser(LOG_NOTICE, "%s", "make all");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!clientLoop(mdtxMake)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellMAKE collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "build a HTML catalog");
#ifndef utMAIN
  if (!clientWriteLock()) YYABORT;
  if (!mdtxMake($2)) YYABORT;
  if (!clientWriteUnlock()) YYABORT;
#endif
}
            | shellCLEAN shellEOL
{
  logParser(LOG_NOTICE, "%s", "clean all");
#ifndef utMAIN
  if (!clientLoop(mdtxClean)) YYABORT;
#endif
}
;
            | shellCLEAN collection shellEOL
{
  logParser(LOG_NOTICE, "clean an HTML catalog", $2);
#ifndef utMAIN
  if (!mdtxClean($2)) YYABORT;
#endif
}
;
            | shellSU shellEOL
{
  logParser(LOG_NOTICE, "%s", "change to mdtx user");
#ifndef utMAIN
  if (!mdtxSu(NULL)) YYABORT;
#endif
}
            | shellSU collection shellEOL
{
  logParser(LOG_NOTICE, "%s", "change to collection user");
#ifndef utMAIN
  if (!mdtxSu($2)) YYABORT;
#endif
}
;

%%


/*=======================================================================
 * Function   : yyerror
 * Description: Emit an error message. Called by the parser on error
 *              state.
 * Synopsis   : void parsererror(char* message)
 * Input      : char* message = the error message.
 * Output     : N/A
 =======================================================================*/
void
shellQuery_error(const char* message)
{
  if(message != NULL) {
      logEmit(LOG_ERR, "%s", message);
  }
  logEmit(LOG_NOTICE, "%s", "bad invocation of mdtx's shell (try completion)");
}


/*=======================================================================
 * Function   : parseShellQuery
 * Description: Parse the mdtx shell query
 * Synopsis   : int parseShellQuery(int argc, char** argv, int optind)
 * Input      : int argc, char** argv, int optind
 * Output     : TRUE on success
=======================================================================*/
int 
parseShellQuery(int argc, char** argv, int optind)
{
  int rc = FALSE;
  BisonSelf parser;
  YY_BUFFER_STATE state;

  logEmit(LOG_INFO, "%s", "parsing the shell query");
  if (!getCommandLine(argc, argv, optind)) goto error;

  // initialise parser
  if (shellQuery_lex_init(&(parser.scanner))) {
    logEmit(LOG_ERR, "%s", "shellQuery_lex_init fails");
    goto error;
  }
  shellQuery_set_debug(env.debugLexer, parser.scanner);
  logParser(LOG_DEBUG, "shellQuery_set_debug = %i", 
	  shellQuery_get_debug(parser.scanner));
  
  state = shellQuery__scan_string(env.commandLine, parser.scanner);
  
  // call parser 
  if (shellQuery_parse(&parser)) {
    goto error2;
  }

  shellQuery__delete_buffer(state, parser.scanner);
  rc = TRUE;
 error2:
  shellQuery_lex_destroy(parser.scanner);  
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "query fails");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
  fprintf(stderr, " query");

  parserOptions();
  fprintf(stderr, "  ---\n"
	  "  query\t\t\tTo document here...\n");
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for confFile module.
 * Synopsis   : utconfFile
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Configuration* conf = NULL;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = PARSER_SHORT_OPTIONS"i";
  struct option longOptions[] = {
    PARSER_LONG_OPTIONS,
    {0, 0, 0, 0}
  };
       
  // import mdtx environment
  getEnv(&env);
  env.logSeverity = "notice";

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {

      GET_PARSER_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(conf = getConfiguration())) goto error;
  if (!parseShellQuery(argc, argv, optind)) goto error;
  /************************************************************************/
 
  rc = TRUE;
 error:
  freeConfiguration();
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

 
