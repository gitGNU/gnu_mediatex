/*=======================================================================
 * Version: $Id: shellQuery.y,v 1.5 2015/07/28 11:45:49 nroche Exp $
 * Project: Mediatex
 * Module : shell parser
 *
 * shell query parser

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
 * Version: this file is generated by BISON using shellQuery.y
 * Project: Mediatex
 * Module : shell parser
 *
 * shell query parser.

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

#define YYSTYPE SHELL_STYPE
}

%{  
#include "mediatex-config.h"
#include "client/mediatex-client.h"
  /* yyscan_t is pre-defined by mediatex-config.h and will be defined
     by Flex header bellow */
%}

%union {
  int         number;
  char        string[MAX_SIZE_STRING+1];
  Collection* coll;
}

%{
  // upper YYSTYPE union is required by Flex headers
#include "parser/shellQuery.h"

#define LINENO shell_get_lineno(yyscanner)
  
void shell_error(yyscan_t yyscanner, Collection* coll, const char* message);
%}

/* declarations: ===================================================*/
%defines "parser/shellQuery.tab.h"
%output "parser/shellQuery.tab.c"
%define api.prefix {shell_}
%define api.pure full
%param {yyscan_t yyscanner}
%parse-param {Collection* coll}
%define parse.error verbose
%verbose
%debug
 
%start queries

%token            shellERROR
%token            shellEOL

%token <string>   shellSTRING
%token <number>   shellNUMBER

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
%token            shellUPGRADEP
%token            shellCOMMIT
%token            shellMAKE
%token            shellCHECK
%token            shellBIND
%token            shellUNBIND
%token            shellMOUNT
%token            shellUMOUNT
%token            shellUPLOAD
%token            shellUPLOADP
%token            shellUPLOADPP
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
  $$ = coll;
  coll = 0;
};

newCollection2: shellCOLL
{
  // addCollection will be call later
  if (!(coll = createCollection())) YYABORT;
};

newCollUser: newCollConf shellMINUS newCollLabel
           | newCollLabel
;

newCollConf: shellSTRING
{
  if (!(coll->masterLabel = createString($1))) YYABORT;
};

newCollLabel: shellSTRING
{
  strncpy(coll->label, $1, MAX_SIZE_COLL);
};

newCollHost: shellAROBASE shellSTRING 
{
  strncpy(coll->masterHost, $2, MAX_SIZE_HOST);
}
             | // empty
;

newCollPort: shellCOLON shellNUMBER
{
  coll->masterPort = $2;
}
             | // empty
;

 /* main rules */

queries: query
       | shellEOL /* empty query */
{
  fprintf(stderr, "please use the -h flag to get help\n");
}
;

query: shellADMIN admConfQuery
	  /* mediatex adm init
	     mediatex adm remove
	     mediatex adm purge 
	     mediatex adm add user
	     mediatex adm del user
	     mediatex adm add coll COLL@HOST:PORT
	     mediatex adm del coll COLL
	     mediatex adm update coll COLL
	     mediatex adm update
	     mediatex adm commit coll COLL
	     mediatex adm commit
	     mediatex adm make coll COLL
	     mediatex adm make
	     mediatex adm bind 
	     mediatex adm unbind
	     mediatex adm mount ISO on PATH
	     mediatex adm umount PATH
	     mediatex get PATH as COLL on FINGERPRINT */
     | shellSERVER srvQuery
          /* mediatex srv save
	     mediatex srv extract
	     mediatex srv notify 
	     mediatex src deliver */
     | apiQuery
;
        
apiQuery: apiSuppQuery
	  /* mediatex add supp SUPP to all
	     mediatex del supp SUPP from all
	     mediatex add supp SUPP to coll COLL
	     mediatex del supp SUPP from coll COLL
	     mediatex add supp SUPP on PATH
	     mediatex del supp SUPP
	     mediatex list supp
	     mediatex note supp as TEXT
	     mediatex check supp on PATH 
             mediatex upload PATH to coll COLL */
        | apiCollQuery
	  /* mediatex add key PATH to coll COLL
	     mediatex del key FINGERPRINT from coll COLL
	     mediatex list coll
	     mediatex motd
	     mediatex upgrade
	     mediatex upgrade coll COLL 
	     mediatex make coll COLL
	     mediatex make
	     mediatex clean coll COLL
	     mediatex clean
	     mediatex su
	     mediatex su coll COLL */


 /* to call scripts directely (for daemon, admin or expert user) */

admConfQuery: shellINIT shellEOL
{
  logParser(LOG_INFO, "%s", "initializing mediatex software");
  if (!env.noRegression) {
    if (!mdtxInit()) YYABORT;
  }
}
            | shellREMOVE shellEOL
{
  logParser(LOG_INFO, "%s", "removing mediatex software");
  if (!env.noRegression) {
    if (!mdtxRemove()) YYABORT;
  }
}
            | shellPURGE shellEOL
{
  logParser(LOG_INFO, "%s", "purging mediatex software");
  if (!env.noRegression) {
    if (!mdtxPurge()) YYABORT;
  }
}
            | shellADD shellUSER shellSTRING shellEOL
{
  logParser(LOG_INFO, "add %s user to groups", $3);
  if (!env.noRegression) {
    if (!mdtxAddUser($3)) YYABORT;
  }
}
            | shellDEL shellUSER shellSTRING shellEOL
{
  logParser(LOG_INFO, "del %s user from groups", $3);
  if (!env.noRegression) {
    if (!mdtxDelUser($3)) YYABORT;
  }
}
            | shellADD newCollection shellEOL
{
  int rc = TRUE;
  logParser(LOG_INFO, "create/join new %s collection", $2->label);
  if (!env.noRegression) {
    rc=rc&& clientWriteLock();
    rc=rc&& mdtxAddCollection($2);
    rc=rc&& clientWriteUnlock();
  }
  destroyCollection($2);
  if (!rc) YYABORT;
}
            | shellDEL collection shellEOL
{
  logParser(LOG_INFO, "free the %s collection", $2);
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxDelCollection($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPDATE shellEOL
{
  logParser(LOG_INFO, "%s", "update all");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!clientLoop(mdtxUpdate)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPDATE collection shellEOL
{
  logParser(LOG_INFO, "%s", "update collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUpdate($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellCOMMIT shellEOL
{
  logParser(LOG_INFO, "%s", "commit all");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!clientLoop(mdtxCommit)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellCOMMIT collection shellEOL
{
  logParser(LOG_INFO, "%s", "commit collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxCommit($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellMAKE shellEOL
{
  logParser(LOG_INFO, "%s", "quickly make all");
  if (!env.noRegression) {
    env.noCollCvs = TRUE; // disable update/commit
    if (!clientLoop(mdtxMake)) YYABORT;
  }
}
            | shellMAKE collection shellEOL
{
  logParser(LOG_INFO, "%s", "quickly make collection");
  if (!env.noRegression) {
    env.noCollCvs = TRUE;
    if (!mdtxMake($2)) YYABORT;
  }
}
            | shellBIND shellEOL
{
  logParser(LOG_INFO, "%s", "bind mediatex directories");
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel)) YYABORT;
    if (!mdtxBind()) YYABORT;
  }
}
            | shellUNBIND shellEOL
{
  logParser(LOG_INFO, "%s", "unbind mediatex directories");
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel)) YYABORT;
    if (!mdtxUnbind()) YYABORT;
  }
}
            | shellMOUNT shellSTRING shellON shellSTRING shellEOL
{
  logParser(LOG_INFO, "mount %s on %s", $2, $4);
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel)) YYABORT;
    if (!mdtxMount($2, $4)) YYABORT;
  }
}
            | shellUMOUNT shellSTRING shellEOL
{
  logParser(LOG_INFO, "umount %s", $2);
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel)) YYABORT;
    if (!mdtxUmount($2)) YYABORT;
  }
}
            | shellGET shellSTRING shellAS shellSTRING shellON 
	    shellSTRING shellEOL
{
  logParser(LOG_INFO, "get %s as %s on %s", $2, $4, $6);
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel)) YYABORT;
    env.noCollCvs = TRUE; // do not upgrade
    if (!mdtxScp($4, $6, $2)) YYABORT;
  }
}
;

 /* server queries (sending signals to server) */

srvQuery: shellSAVE shellEOL
{
  logParser(LOG_INFO, "%s", "send SAVEMD5 signal to daemon");
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel)) YYABORT;
    if (!mdtxSyncSignal(MDTX_SAVEMD5)) YYABORT;
  }
}
        | shellEXTRACT shellEOL
{
  logParser(LOG_INFO, "%s", "send EXTRACT signal to daemon");
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel)) YYABORT;
    if (!mdtxSyncSignal(MDTX_EXTRACT)) YYABORT;
  }
}
        | shellNOTIFY shellEOL
{
  logParser(LOG_INFO, "%s", "send NOTIFY signal to daemon");
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel)) YYABORT;
    if (!mdtxSyncSignal(MDTX_NOTIFY)) YYABORT;
  }
}
        | shellDELIVER shellEOL
{
  logParser(LOG_INFO, "%s", "send DELIVER signal to daemon");
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel)) YYABORT;
    if (!mdtxSyncSignal(MDTX_DELIVER)) YYABORT;
  }
}
;

 /* support queries API */

apiSuppQuery: shellADD support shellTO shellALL shellEOL
{
  logParser(LOG_INFO, "%s", "add a support to all collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxShareSupport($2, 0)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellADD support shellTO collection shellEOL
{
  logParser(LOG_INFO, "%s", "add a support to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxShareSupport($2, $4)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellDEL support shellFROM shellALL shellEOL
{
  logParser(LOG_INFO, "%s", "del a support from all collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxWithdrawSupport($2, 0)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellDEL support shellFROM collection shellEOL
{
  logParser(LOG_INFO, "%s", "del a support from a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxWithdrawSupport($2, $4)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellADD support shellON shellSTRING shellEOL
{
  logParser(LOG_INFO, "add %s support on %s", $2, $4);
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxAddSupport($2, $4)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellDEL support shellEOL
{
  logParser(LOG_INFO, "%s", "remove a support");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxDelSupport($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellLIST shellSUPP shellEOL
{
  logParser(LOG_INFO, "%s", "list local supports");
  if (!env.noRegression) {
    if (!mdtxLsSupport()) YYABORT;
  }
}
           | shellNOTE support shellAS shellSTRING shellEOL
{
  logParser(LOG_INFO, "note a support as \"%s\"", $4);
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUpdateSupport($2, $4)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
           | shellCHECK support shellON shellSTRING shellEOL
{
  logParser(LOG_INFO, "check content located at %s", $4);
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxHaveSupport($2, $4)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPLOAD shellSTRING shellTO collection shellEOL
{
  logParser(LOG_INFO, "%s", "upload a file to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUploadFile($4, $2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPLOADP shellSTRING shellTO collection shellEOL
{
  logParser(LOG_INFO, "%s", "upload+ a file to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUploadPlus($4, $2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPLOADPP shellSTRING shellTO collection shellEOL
{
  logParser(LOG_INFO, "%s", "upload++ a file to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUploadPlusPlus($4, $2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
;

 /* collection queries API */

apiCollQuery: shellADD key shellTO collection shellEOL
{
  logParser(LOG_INFO, "%s", "add a key to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!addKey($4, $2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellDEL key shellFROM collection shellEOL
{
  logParser(LOG_INFO, "%s", "del a key to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!delKey($4, $2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellLIST shellCOLL shellEOL
{
  logParser(LOG_INFO, "%s", "listing enabled collections");
  if (!env.noRegression) {
    if (!mdtxListCollection()) YYABORT;
  }
}
            | shellMOTD shellEOL
{
  logParser(LOG_INFO, "%s", "message of the day");
  if (!env.noRegression) {
    if (!updateMotd()) YYABORT;
  }
}
            | shellUPGRADE shellEOL
{
  logParser(LOG_INFO, "%s", "upgrade all");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    
    // needed if there is still no collection
    if (!loadConfiguration(CFG)) YYABORT;
    getConfiguration()->fileState[iCFG] = MODIFIED; 
    
    if (!clientLoop(mdtxUpgrade)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPGRADE collection shellEOL
{
  logParser(LOG_INFO, "%s", "upgrade a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUpgrade($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellMAKE shellEOL
{
  logParser(LOG_INFO, "%s", "make all");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!clientLoop(mdtxMake)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellMAKE collection shellEOL
{
  logParser(LOG_INFO, "%s", "build a HTML catalog");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxMake($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellCLEAN shellEOL
{
  logParser(LOG_INFO, "%s", "clean all");
  if (!env.noRegression) {
    if (!clientLoop(mdtxClean)) YYABORT;
  }
}
            | shellCLEAN collection shellEOL
{
  logParser(LOG_INFO, "clean an HTML catalog", $2);
  if (!env.noRegression) {
    if (!mdtxClean($2)) YYABORT;
  }
}
            | shellSU shellEOL
{
  logParser(LOG_INFO, "%s", "change to mediatex user");
  if (!env.noRegression) {
    if (!mdtxSu(0)) YYABORT;
  }
}
            | shellSU collection shellEOL
{
  logParser(LOG_INFO, "%s", "change to collection user");
  if (!env.noRegression) {
    if (!mdtxSu($2)) YYABORT;
  }
}
            | shellUPGRADEP shellEOL
{
  logParser(LOG_INFO, "%s", "upgrade+ all");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    
    // needed if there is still no collection
    if (!loadConfiguration(CFG)) YYABORT;
    getConfiguration()->fileState[iCFG] = MODIFIED; 

    if (!clientLoop(mdtxUpgradePlus)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPGRADEP collection shellEOL
{
  logParser(LOG_INFO, "%s", "upgrade+ a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUpgradePlus($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
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
void shell_error(yyscan_t yyscanner, Collection* coll, const char* message)
{
  logParser(LOG_ERR, "%s on token '%s' line %i\n",
	  message, shell_get_text(yyscanner), LINENO);
  logParser(LOG_NOTICE, "%s", 
	  "bad invocation of mdtx's shell (try completion)");
}


/*=======================================================================
 * Function   : parseShellQuery
 * Description: Parse the mediatex shell query
 * Synopsis   : int parseShellQuery(int argc, char** argv, int optind)
 * Input      : int argc, char** argv, int optind
 * Output     : TRUE on success
=======================================================================*/
int 
parseShellQuery(int argc, char** argv, int optind)
{
  int rc = FALSE;
  void* buffer = 0;
  yyscan_t scanner;
  Collection* coll = 0;

  logParser(LOG_INFO, "%s", "parsing the shell query");
  if (!getCommandLine(argc, argv, optind)) goto error;

  // initialise parser
  if (shell_lex_init(&scanner)) {
    logParser(LOG_ERR, "%s", "shell_lex_init fails");
    goto error;
  }

  shell_set_debug(env.debugLexer, scanner);
  logParser(LOG_DEBUG, "shell_set_debug = %i", shell_get_debug(scanner));
  
  buffer = shell__scan_string(env.commandLine, scanner);
  
  // call parser 
  if (shell_parse(scanner, coll)) {
    goto error2;
  }

  rc = TRUE;
 error2:
  shell__delete_buffer(buffer, scanner);
  shell_lex_destroy(scanner);  
 error:
  if (!rc) {
    logParser(LOG_ERR, "%s", "query fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

 
