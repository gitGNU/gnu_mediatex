/*=======================================================================
 * Project: Mediatex
 * Module : shell parser
 *
 * shell query parser

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 Nicolas Roche
   
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
 Copyright (C) 2014 2015 2016 Nicolas Roche
   
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

void shell_error(yyscan_t yyscanner, Collection* coll, 
		 UploadParams* upParam, const char* message); 
%}

/* declarations: ===================================================*/
%defines "parser/shellQuery.tab.h"
%output "parser/shellQuery.tab.c"
%define api.prefix {shell_}
%define api.pure full
%param {yyscan_t yyscanner}
%parse-param {Collection* coll} {UploadParams* upParam}
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
%token            shellMASTER

%token            shellALL
%token            shellTO
%token            shellFROM
%token            shellAS
%token            shellFOR
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
%token            shellFILE
%token            shellCATALOG
%token            shellRULES
%token            shellUPLOADP
%token            shellUPLOADPP
%token            shellGET
%token            shellSU
%token            shellMOTD
%token            shellAUDIT
%token            shellSAVE
%token            shellEXTRACT
%token            shellNOTIFY
%token            shellQUICK
%token            shellSCAN
%token            shellTRIM
%token            shellSTATUS

%token            shellAROBASE
%token            shellCOLON
%token            shellMINUS

%type <coll>      newCollection
%type <string>    collection
%type <string>    support
%type <string>    file
%type <string>    target
%type <string>    key

%%
   /* grammar rules: ==================================================*/

 /* objects */

collection: shellCOLL shellSTRING
{
  strcpy($$, $2);
}

support: shellSUPP shellSTRING
{
  strcpy($$, $2);
}

file: shellFILE shellSTRING
{
  strcpy($$, $2);
}

key: shellKEY shellSTRING
{
  strcpy($$, $2);
}

newCollection: newCollection2 newCollUser newCollHost newCollPort
{
  $$ = coll;
  coll = 0;
}

newCollection2: shellCOLL
{
  // addCollection will be call later
  if (!(coll = createCollection())) YYABORT;
}

newCollUser: newCollConf shellMINUS newCollLabel
           | newCollLabel


newCollConf: shellSTRING
{
  if (!(coll->masterLabel = createString($1))) YYABORT;
}

newCollLabel: shellSTRING
{
  strncpy(coll->label, $1, MAX_SIZE_COLL);
}

newCollHost: shellAROBASE shellSTRING 
{
  strncpy(coll->masterHost, $2, MAX_SIZE_HOST);
}
           | // empty

newCollPort: shellCOLON shellNUMBER
{
  coll->masterPort = $2;
}
           | // empty

uploadParams: uploadParams uploadParam
            | uploadParam

target: shellAS shellSTRING
{
  strcpy($$, $2);
}
      | // empty
{
  $$[0] = 0;
}     

uploadParam: file target
{
  UploadFile* upFile = 0;

  if (!(upFile = createUploadFile())) YYABORT;
  if (!(upFile->source = createString($1))) YYABORT;
  if ($2 && !(upFile->target = createString($2))) YYABORT;
  if (!rgInsert(upParam->upFiles, upFile)) YYABORT;
}
           | shellCATALOG shellSTRING
{
  if (upParam->catalog) {
    logParser(LOG_ERR, "please only provide catalog file to upload");
    YYABORT;
  }
  if (!(upParam->catalog = createString($2))) YYABORT;
}
           | shellRULES shellSTRING
{
  if (upParam->extract) {
    logParser(LOG_ERR, "please only provide one extract file to upload");
    YYABORT;
  }
  if (!(upParam->extract = createString($2))) YYABORT;
}

 /* main rules */

queries: query
       | shellEOL /* empty query */
{
  logParser(LOG_ERR, "please use the -h flag to get help\n");
}

query: shellADMIN admConfQuery
       /* mediatex adm init
	  mediatex adm remove
	  mediatex adm purge 
	  mediatex adm add user
	  mediatex adm del user
	  mediatex adm add coll COLL@HOST:PORT
	  mediatex adm del coll COLL
	  mediatex adm update
	  mediatex adm update coll COLL
	  mediatex adm commit
	  mediatex adm commit coll COLL
	  mediatex adm make
	  mediatex adm make coll COLL
	  mediatex adm bind 
	  mediatex adm unbind
	  mediatex adm mount ISO on PATH
	  mediatex adm umount PATH
	  mediatex get PATH as COLL on FINGERPRINT as PATH */
     | shellSERVER srvQuery
       /* mediatex srv save
	  mediatex srv extract
	  mediatex srv notify 
	  mediatex src deliver */
     | apiQuery
        
apiQuery: apiSuppQuery
        /* mediatex add supp SUPP to all
	   mediatex add supp SUPP to coll COLL
	   mediatex del supp SUPP from all
	   mediatex del supp SUPP from coll COLL
	   mediatex add supp SUPP on PATH
	   mediatex add file PATH
	   mediatex del supp SUPP
	   mediatex list supp
	   mediatex note supp as TEXT
	   mediatex check supp on PATH 
	   mediatex upload+{0,2} [file PATH as PATH]* [catalog PATH] [extract PATH} to coll COLL 
	*/
          | apiCollQuery
	/* mediatex add key PATH to coll COLL
	   mediatex del key FINGERPRINT from coll COLL
	   mediatex list master coll
	   mediatex list coll
	   mediatex motd
	   mediatex upgrade[+]
	   mediatex upgrade[+] coll COLL 
	   mediatex make
	   mediatex make coll COLL
	   mediatex clean
	   mediatex clean coll COLL
	   mediatex su
	   mediatex su coll COLL 
	   mediatex audit file FILE target PATH catalog FILE rules FILE coll COLL for MAIL
	*/


 /* to call scripts directely (for daemon, admin or expert user) */

admConfQuery: shellINIT shellEOL
{
  logParser(LOG_INFO, "initializing mediatex software");
  if (!env.noRegression) {
    if (!mdtxInit()) YYABORT;
  }
}
            | shellREMOVE shellEOL
{
  logParser(LOG_INFO, "removing mediatex software");
  if (!env.noRegression) {
    if (!mdtxRemove()) YYABORT;
  }
}
            | shellPURGE shellEOL
{
  logParser(LOG_INFO, "purging mediatex software");
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
  logParser(LOG_INFO, "update all");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!clientLoop(mdtxUpdate)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPDATE collection shellEOL
{
  logParser(LOG_INFO, "update collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUpdate($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellCOMMIT shellEOL
{
  logParser(LOG_INFO, "commit all");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!clientLoop(mdtxCommit)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellCOMMIT collection shellEOL
{
  logParser(LOG_INFO, "commit collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxCommit($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellMAKE shellEOL
{
  logParser(LOG_INFO, "quickly make all");
  if (!env.noRegression) {
    env.noGitPullPush = TRUE; // disable git to reach network
    if (!clientLoop(mdtxMake)) YYABORT;
  }
}
            | shellMAKE collection shellEOL
{
  logParser(LOG_INFO, "quickly make collection");
  if (!env.noRegression) {
    env.noGitPullPush = TRUE; // disable git to reach network
    if (!mdtxMake($2)) YYABORT;
  }
}
            | shellBIND shellEOL
{
  logParser(LOG_INFO, "bind mediatex directories");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxBind()) YYABORT;
  }
}
            | shellUNBIND shellEOL
{
  logParser(LOG_INFO, "unbind mediatex directories");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxUnbind()) YYABORT;
  }
}
            | shellMOUNT shellSTRING shellON shellSTRING shellEOL
{
  logParser(LOG_INFO, "mount %s on %s", $2, $4);
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxMount($2, $4)) YYABORT;
  }
}
            | shellUMOUNT shellSTRING shellEOL
{
  logParser(LOG_INFO, "umount %s", $2);
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxUmount($2)) YYABORT;
  }
}
            | shellGET shellSTRING shellAS shellSTRING shellON shellSTRING 
	               shellAS shellSTRING shellEOL
{
  logParser(LOG_INFO, "get %s as %s on %s as %s", $2, $4, $6, $8);
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    env.noGit = TRUE; // do not use git at all (no commit)
    if (!mdtxScp($4, $6, $2, $8)) YYABORT;
  }
}

 /* server queries (sending signals to server) */

srvQuery: shellSAVE shellEOL
{
  logParser(LOG_INFO, "send SAVEMD5 signal to daemon");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxSyncSignal(REG_SAVEMD5)) YYABORT;
  }
}
        | shellEXTRACT shellEOL
{
  logParser(LOG_INFO, "send EXTRACT signal to daemon");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxSyncSignal(REG_EXTRACT)) YYABORT;
  }
}
        | shellNOTIFY shellEOL
{
  logParser(LOG_INFO, "send NOTIFY signal to daemon");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxSyncSignal(REG_NOTIFY)) YYABORT;
  }
}
        | shellQUICK shellSCAN shellEOL
{
  logParser(LOG_INFO, "send QUICKSCAN signal to daemon");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxSyncSignal(REG_QUICKSCAN)) YYABORT;
  }
}
        | shellSCAN shellEOL
{
  logParser(LOG_INFO, "send SCAN signal to daemon");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxSyncSignal(REG_SCAN)) YYABORT;
  }
}
        | shellTRIM shellEOL
{
  logParser(LOG_INFO, "send TRIM signal to daemon");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxSyncSignal(REG_TRIM)) YYABORT;
  }
}
        | shellCLEAN shellEOL
{
  logParser(LOG_INFO, "send CLEAN signal to daemon");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxSyncSignal(REG_CLEAN)) YYABORT;
  }
}
        | shellPURGE shellEOL
{
  logParser(LOG_INFO, "send PURGE signal to daemon");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxSyncSignal(REG_PURGE)) YYABORT;
  }
}
        | shellSTATUS shellEOL
{
  logParser(LOG_INFO, "send  signal to daemon");
  int isUserAllowed = FALSE;
  
  if (!env.noRegression) {
    if (!allowedUser(env.confLabel, &isUserAllowed, 0)) YYABORT;
    if (!isUserAllowed) YYABORT;
    if (!mdtxSyncSignal(REG_STATUS)) YYABORT;
  }
}

 /* support queries API */

apiSuppQuery: shellADD support shellTO shellALL shellEOL
{
  logParser(LOG_INFO, "add a support to all collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxShareSupport($2, 0)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellADD support shellTO collection shellEOL
{
  logParser(LOG_INFO, "add a support to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxShareSupport($2, $4)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellDEL support shellFROM shellALL shellEOL
{
  logParser(LOG_INFO, "del a support from all collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxWithdrawSupport($2, 0)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellDEL support shellFROM collection shellEOL
{
  logParser(LOG_INFO, "del a support from a collection");
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
            | shellADD file shellEOL
{
  logParser(LOG_INFO, "add %s file", $2);
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxAddFile($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellDEL support shellEOL
{
  logParser(LOG_INFO, "remove a support");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxDelSupport($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellLIST shellSUPP shellEOL
{
  logParser(LOG_INFO, "list local supports");
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
            | shellUPLOAD uploadParams shellTO collection shellEOL
{
  logParser(LOG_INFO, "upload a file to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUpload($4, upParam->catalog, upParam->extract, 
		    upParam->upFiles)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPLOADP uploadParams shellTO collection shellEOL
{
  logParser(LOG_INFO, "upload+ a file to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUploadPlus($4, upParam->catalog, upParam->extract, 
		    upParam->upFiles)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellUPLOADPP uploadParams shellTO collection shellEOL
{
  logParser(LOG_INFO, "upload++ a file to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUploadPlusPlus($4, upParam->catalog, upParam->extract, 
		    upParam->upFiles)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}

 /* collection queries API */

apiCollQuery: shellADD key shellTO collection shellEOL
{
  logParser(LOG_INFO, "add a key to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!addKey($4, $2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellDEL key shellFROM collection shellEOL
{
  logParser(LOG_INFO, "del a key to a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!delKey($4, $2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellLIST shellMASTER shellCOLL shellEOL
{
  logParser(LOG_INFO, "listing locally hosted collections");
  if (!env.noRegression) {
    if (!mdtxListCollection(TRUE)) YYABORT;
  }
}
            | shellLIST shellCOLL shellEOL
{
  logParser(LOG_INFO, "listing collections");
  if (!env.noRegression) {
    if (!mdtxListCollection(FALSE)) YYABORT;
  }
}
            | shellMOTD shellEOL
{
  logParser(LOG_INFO, "message of the day");
  if (!env.noRegression) {
    if (!updateMotd()) YYABORT;
  }
}
            | shellUPGRADE shellEOL
{
  logParser(LOG_INFO, "upgrade all");
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
  logParser(LOG_INFO, "upgrade a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUpgrade($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellMAKE shellEOL
{
  logParser(LOG_INFO, "make all");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!clientLoop(mdtxMake)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellMAKE collection shellEOL
{
  logParser(LOG_INFO, "build a HTML catalog");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxMake($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellCLEAN shellEOL
{
  logParser(LOG_INFO, "clean all");
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
  logParser(LOG_INFO, "change to mediatex user");
  if (!env.noRegression) {
    if (!mdtxSu(0)) YYABORT;
  }
}
            | shellSU collection shellEOL
{
  logParser(LOG_INFO, "change to collection user");
  if (!env.noRegression) {
    if (!mdtxSu($2)) YYABORT;
  }
}
            | shellUPGRADEP shellEOL
{
  logParser(LOG_INFO, "upgrade+ all");
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
  logParser(LOG_INFO, "upgrade+ a collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxUpgradePlus($2)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}
            | shellAUDIT collection shellFOR shellSTRING shellEOL
{
  logParser(LOG_INFO, "audit collection");
  if (!env.noRegression) {
    if (!clientWriteLock()) YYABORT;
    if (!mdtxAudit($2, $4)) YYABORT;
    if (!clientWriteUnlock()) YYABORT;
  }
}

%%


/*=======================================================================
 * Function   : yyerror
 * Description: Emit an error message. Called by the parser on error
 *              state.
 * Synopsis   : void parsererror(char* message)
 * Input      : char* message = the error message.
 * Output     : N/A
 =======================================================================*/
void shell_error(yyscan_t yyscanner, Collection* coll, 
		 UploadParams* upParam, const char* message)
{
  logParser(LOG_ERR, "%s on token '%s'",
	  message, shell_get_text(yyscanner));
  logParser(LOG_NOTICE,
	    "bad invocation of mdtx's shell. Try auto-completion:");
  logParser(LOG_NOTICE,
	    "-> $ . /etc/bash_completion.d/mediatex_comp");
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
  UploadParams upParam;

  logParser(LOG_DEBUG, "parseShellQuery");
  if (!getCommandLine(argc, argv, optind)) {
    rc = TRUE; // no query
    goto error;
  }

  memset(&upParam, 0, sizeof(UploadParams));
  if (!(upParam.upFiles = createRing())) goto error;
  
  // initialise parser
  if (shell_lex_init(&scanner)) {
    logParser(LOG_ERR, "shell_lex_init fails");
    goto error;
  }

  shell_set_debug(env.debugLexer, scanner);
  logParser(LOG_DEBUG, "shell_set_debug = %i", shell_get_debug(scanner));
  
  buffer = shell__scan_string(env.commandLine, scanner);
  
  // call parser 
  if (shell_parse(scanner, coll, &upParam)) {
    goto error2;
  }

  rc = TRUE;
 error2:
  shell__delete_buffer(buffer, scanner);
  shell_lex_destroy(scanner);  
 error:
  if (!rc) {
    logParser(LOG_ERR, "query fails");
  }
  destroyString(upParam.catalog);
  destroyString(upParam.extract);
  destroyRing(upParam.upFiles, (void*(*)(void*)) destroyUploadFile);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

 
