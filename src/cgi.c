/*=======================================================================
 * Version: $Id: cgi.c,v 1.12 2015/10/20 19:41:50 nroche Exp $
 * Project: MediaTeX
 * Module : cgi script software
 *
 * Cgi script software's main function

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

#include "mediatex-config.h"
#include <regex.h>

static char* confLabel = 0;


/*=======================================================================
 * Function   : loadTemplate
 * Description: Send a template file (xxxHeader.shtml or footer.html)
 * Synopsis   : int sendTemplate(char* filename)
 * Input      : char* filename
 * Output     : TRUE on success
 =======================================================================*/
int sendTemplate(Collection* coll, char* filename)
{
  int rc = FALSE;
  char* path  = 0;
  FILE* fd = 0;
  char buffer[256];
  int len = 0;
  
  logMain(LOG_DEBUG, "sendTemplate %s", filename);

  if (coll->htmlCgiDir == 0) {
    logMain(LOG_ERR, "cannot match cgi to collection");
    goto error;
  }

  if ((path = createString(coll->htmlCgiDir)) == 0 ||
      (path = catString(path, "/../")) == 0 ||
      (path = catString(path, filename)) == 0) {
    logMain(LOG_ERR, "mdtx-cgi cannot malloc path for %s", filename);
    goto error;
  }
  
  if ((fd = fopen(path, "r")) == 0) {
    logMain(LOG_ERR, "mdtx-cgi cannot open template %s", path);
    goto error;
  }
  
  while ((len = fread(buffer, 1, 256, fd)) > 0) {
    fwrite(buffer, 1, len, stdout);
  }
  
  rc = TRUE;
 error:
  if (fd && fclose(fd)) rc = FALSE;
  if (!rc) {
    logMain(LOG_ERR, "sendTemplate fails");
  }
  destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : queryServer
 * Description: write the "GET hash size" query into the soket
 *              and extract the server response from the socket
 * Synopsis   : void queryServer(Server* server, RecordTree* tree, 
 *                               char* reply)
 * Input      : Server* server: server to query
 *              RecordTree* tree: tree with a single record to query
 * Output     : char* reply: the response from server
 *              rc: TRUE on success
 =======================================================================*/
int queryServer(Server* server, RecordTree* tree, char* reply)
{
  int rc = FALSE;
  int socket = -1;
  int n = 0;
  
  logMain(LOG_DEBUG, "queryServer");
  reply[0] = 0;

  // connect server and write query
  if ((socket = connectServer(server)) == -1) goto error;
  if (!upgradeServer(socket, tree, 0)) goto error;

  // read reply
  if (env.dryRun) goto end;
  n = tcpRead(socket, reply, 255);
  // erase the \n send by server
  if (n<=0) n = 1;
  reply[n-1] = (char)0; 

  logMain(LOG_INFO, "receive: %s", reply);
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "queryServer fails");
  }
  if (!env.dryRun && socket != -1) close(socket);
  return rc;
}


/*=======================================================================
 * Function   : mdtxSearch
 * Description: ask all the server from the list until it found 
 *              a server that heberge the hash/size couple we are 
 *              looking for
 * Synopsis   : int mdtxSearch(RecordTre* rTree, char* reply)
 * Input      : RecordTre* rTree: tree to send, having a single record
 * Output     : char* reply: response from the server (may provide an url)
 *              rc: TRUE on succes
 =======================================================================*/
int mdtxSearch(RecordTree* rTree, char* reply)
{ 
  int rc = FALSE;
  int status = 0;
  Collection* coll = 0;
  ServerTree *sTree = 0;
  Server* localhost = 0;
  Server *server = 0;

  logMain(LOG_DEBUG, "mdtxSearch");

  if (!(coll = rTree->collection)) goto error;
  if (!(localhost = getLocalHost(coll))) goto error;

  logMain(LOG_WARNING, "localhost: %s/%s", 
	  localhost->host, localhost->fingerPrint);

  if ((sTree = coll->serverTree) == 0) goto error;
  if (isEmptyRing(sTree->servers)) goto error;

  /* loop on every server */
  rgRewind(sTree->servers);
  while ((server = rgNext(sTree->servers))) {

    logMain(LOG_WARNING, "server: %s/%s", 
	    server->host, server->fingerPrint);

    // skip server not directly connected to us
    if (!rgShareItems(localhost->networks, server->networks)) {
      logMain(LOG_INFO, "do not connect unreachable server %s:%i",
	      server->host, server->mdtxPort);
      continue;
    }

    // query server
    if (!server) break;
    if (!queryServer(server, rTree, reply)) goto error;
     
    if (env.dryRun) {
      strcpy(reply, "120 not found");
    }

    // looking for the reply
    if (sscanf(reply, "%i", &status) < 1) {
      logMain(LOG_ERR, "error reading server reply: %s", reply);
    }
    else {
      logMain(LOG_INFO, "%s:%i tel (%i) %s",
	      server->host, server->mdtxPort, status, reply+4);
	
      /* stop research when founded */
      if (status == 220) break;
    } 
  }
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxSearch fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : mdtxFind
 * Description: ask to do the job and send the CGI response
 *              
 * Synopsis   : int mdtxFind(Record *)
 * Input      : hash and size of the record we are looking for
 * Output     : TRUE on success
 =======================================================================*/
int mdtxFind(RecordTree *tree)
{
  int rc = FALSE;
  char *url;
  int status = 0;
  Collection* coll = 0;
  Record* record = 0;
  char reply[255] = "100 nobody";
  
  logMain(LOG_DEBUG, "mdtxFind");

  if (!(coll = tree->collection)) goto error;
  if (!(record = (Record*)tree->records->head->it)) goto error;

  // send query
  if (!mdtxSearch(tree, reply)) goto error;
  
  // read reply
  if (sscanf(reply, "%i", &status) < 1) {
    logMain(LOG_ERR, "error re-reading reply: ",
	    reply);
    goto error;
  }
    
  switch (status) {
  case 220:
    url = reply + 7; // "220 ok URL"
    logMain(LOG_DEBUG, "found at %s", url);
    
    fprintf(stdout, "Content-Type: text/html\r\n");
    fprintf(stdout, "Refresh: 0; url=%s\r\n", url);
    fprintf(stdout, "\r\n");

    sendTemplate(coll, "cgiHeader.shtml");
    fprintf(stdout, "Please follow <a href=\"%s\"> link</a>!", url);
    sendTemplate(coll, "footer.html");
    break;

  default:
    logMain(LOG_DEBUG, "not found");
    
    fprintf(stdout, "Content-Type: text/html\r\n");
    fprintf(stdout, "\r\n");
    
    sendTemplate(coll, "cgiHeader.shtml");

    fprintf(stdout, 
	    "<br>Sorry, this document is not available yet.\r\n<br>");
    fprintf(stdout, "<h5><i>hash=%s\r\n</i><br>", record->archive->hash);
    fprintf(stdout, "<i>size=%llu\r\n</i><br></h5>",
	    (unsigned long long int)record->archive->size);
    
    fprintf(stdout, 
	    "<br>Please provide a mail to process exctraction.\r\n<br>");
    fprintf(stdout, "<FORM method=post action=\"%s\">\r\n", coll->cgiUrl);
    fprintf(stdout, "<INPUT TYPE=HIDDEN NAME=hash VALUE=%s>", 
	    record->archive->hash);
    fprintf(stdout, "<INPUT TYPE=HIDDEN NAME=size VALUE=%lli>", 
	    (long long int)record->archive->size);

    fprintf(stdout, "Mail <INPUT type=text name=mail>");
    fprintf(stdout, "<INPUT type=submit value=submit>");
    fprintf(stdout, "</FORM>\n");

    sendTemplate(coll, "footer.html");
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxFind fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : mdtxRegister
 * Description: ask to do the job ans send the CGI response
 * Synopsis   : int mdtxFind(Record *)
 * Input      : hash and size of the record we are looking for
 * Output     : ok
 =======================================================================*/
int mdtxRegister(RecordTree *tree)
{
  int rc = FALSE;
  int status = 0;
  Server* server = 0;
  Collection* coll = 0;
  Record* record = 0;
  char reply[255] = "100 nobody";
  
  logMain(LOG_DEBUG, "mdtxRegister");

  /* querying the local server */
  if (!(coll = tree->collection)) goto error;
  if (!(record = (Record*)tree->records->head->it)) goto error;
  if (!(server = getLocalHost(coll))) goto error;
  if (!queryServer(server, tree, reply)) goto error;
  
  if (env.dryRun) {
    strcpy(reply, "221 ok");
  }

  if (sscanf(reply, "%i", &status) < 1) {
    logMain(LOG_ERR, "error reading reply: ", reply);
    status = 500;
  }
  
  logMain(LOG_INFO, "%s:%i tel (%i) %s",
	  server->host, server->mdtxPort, status, reply+4);

  switch (status) {
  case 221:
    logMain(LOG_DEBUG, "registered at localhost");
    
    fprintf(stdout, "%s", "Content-Type: text/html\r\n");
    fprintf(stdout, "%s", "Refresh: 3; url=../index/\r\n");
    fprintf(stdout, "%s", "\r\n");

    sendTemplate(coll, "cgiHeader.shtml");

    fprintf(stdout, "%s",
	    "You will be notified by mail as soon as the file become "
	    "available\n"
	    "Please follow this <a href=\"../index\"> link</a>!");

    sendTemplate(coll, "footer.html");
    
    break;

  default:
    logMain(LOG_ERR, "registration fails");
    
    fprintf(stdout, "Content-Type: text/html\r\n");
    fprintf(stdout, "\r\n");
    
    sendTemplate(coll, "cgiHeader.shtml");

    fprintf(stdout, 
	    "<br>Sorry, localhost cannot register it (errno %i).\r\n<br>",
	    status);
    fprintf(stdout, "<h5><i>hash=%s\r\n</i><br>", record->archive->hash);
    fprintf(stdout, "<i>size=%llu\r\n</i><br>",
	    (unsigned long long int)record->archive->size);
    fprintf(stdout, "<i>mail=%s</i><br></h5>", record->extra);

    sendTemplate(coll, "footer.html");
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxRegister fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : scanCgiQuery
 * Description: parse the CGI query parameters
 *              
 * Synopsis   : RecordTree* scanCgiQuery()
 * Input      : nothing
 * Output     : Record* with hash and size into
 =======================================================================*/
RecordTree* scanCgiQuery(Collection* coll)
{
  Configuration* conf = 0;
  RecordTree* rc = 0;
  RecordTree* tree = 0;
  Archive* archive = 0;
  Record* record = 0;
  off_t size = 0;
  char* extra = 0;
  char **cgivars = (char**)0;

  logMain(LOG_DEBUG, "scanCgiQuery");

  /** First, get the CGI variables into a list of strings **/
  if ((cgivars = getcgivars()) == (char**)0) {
    logMain(LOG_ERR, 
	    "bad usage: you must call this CGI script via apache");
    logMain(LOG_ERR, "or providing following global variables:");
    logMain(LOG_ERR, 
	    "REQUEST_METHOD, QUERY_STRING and SCRIPT_FILENAME");
    goto error;
  }
 
  /** scan the CGI variables sent by the user.  Note the list of **/
  /** variables alternates names and values, and ends in 0.  **/
  if (cgivars[0] == 0     ||
      strcmp(cgivars[0], "hash") ||
      cgivars[1] == 0     ||
      cgivars[1][0] == (char)0   ||
      
      cgivars[2] == 0     ||
      strcmp(cgivars[2], "size") ||
      cgivars[3] == 0     ||
      cgivars[3][0] == (char)0) {
    logMain(LOG_ERR, 
	    "bad usage: please use ?hash=<val>&size=<val> syntax\n");
    goto error;
  }

  if (cgivars[3]     &&
      strcmp(cgivars[3], "mail") &&
      cgivars[4]     &&
      cgivars[4][0] != (char)0) {  
    extra = createString(cgivars[5]);
  }

  if (!(conf = getConfiguration())) goto error;

  if (sscanf(cgivars[3], "%llu", (unsigned long long int*)&size) != 1)
    goto error;
  if (!(archive = addArchive(coll, cgivars[1], size))) goto error;
  if (!(tree = createRecordTree())) goto error;

  tree->collection = coll;
  tree->messageType = CGI;
  //recordTree->doCypher = FALSE;

  if (!getLocalHost(coll)) goto error;
  if (!extra && !(extra = createString("!wanted"))) goto error; 
  if (!(record = newRecord(coll->localhost, archive, DEMAND, extra))) 
    goto error;
  extra = 0;
  if (!rgInsert(tree->records, record)) goto error;
  logMain(LOG_INFO, "querying for %s %i", 
	  record->archive->hash, record->archive->size);
  record = 0;

  rc = tree;
  tree = 0;
 error:
  if (!rc) {
    logMain(LOG_ERR, "scanCgiQuery fails");
  }
  destroyString(extra);
  destroyRecord(record);
  destroyRecordTree(tree);
  freecgivars(cgivars);
  return rc;
}


/*=======================================================================
 * Function   : getIndexLabel
 * Description: Extract index's label from environement
 * Synopsis   : char* getIndexLabel()
 * Input      : N/A
 * Output     : index's label
 =======================================================================*/
char* getIndexLabel()
{
  char* rc = 0;
  char* path = 0;
  regex_t motif_compile;
  int err = 0;
  size_t size;
  regmatch_t matchingPart[3]; // = motif_compile.re_nsub+1

  logMain(LOG_DEBUG, "getIndexLabel");

  // build regex
  if ((err = regcomp(&motif_compile, 
		     "^.*/\\(.*\\)-\\(.*\\)/public_html/cgi/get.cgi$", 
		     0))) {
    size = regerror(err, &motif_compile, 0, 0);
    if ((rc = (char*)malloc(size)) == 0) {
      logMain(LOG_ERR, 
	      "regcomp error (malloc failed for regerror meessage");
      goto error;
    }
    regerror(err, &motif_compile, rc, 20);
    logMain(LOG_ERR, "regcomp error: %s", rc);
    free(rc); rc = 0;
    goto error;
  }

  // cgi script own url
  if ((path = getenv("SCRIPT_FILENAME")) == 0) {
    logMain(LOG_ERR, "cannot retrieve SCRIPT_FILENAME in env");
    logMain(LOG_ERR, 
	    "cgi script is not compatible with your browser");
    goto error;
  }

  err = regexec(&motif_compile, path, 3, matchingPart, 0);
  switch (err) {
  case REG_NOMATCH:
    logMain(LOG_ERR, "regexec: no match");
    goto error;
    break;
  case REG_ESPACE:
    logMain(LOG_ERR, "regexec: no more memory");
    goto error;
    break;
  }

  size = matchingPart[1].rm_eo - matchingPart[1].rm_so;
  if ((confLabel = (char*)malloc(size+1)) == 0) {
    logMain(LOG_ERR, "malloc failed to allocate confLabel string");
    goto error;
  }

  strncpy(confLabel, path + matchingPart[1].rm_so, size);
  confLabel[size] = '\0';
  env.confLabel = confLabel;

  size = matchingPart[2].rm_eo - matchingPart[2].rm_so;
  if ((rc = (char*)malloc(size+1)) == 0) {
    logMain(LOG_ERR, "malloc failed to allocate collection string");
    goto error;
  }
  strncpy(rc, path + matchingPart[2].rm_so, size);
  rc[size] = '\0';

 error:
  if (rc == 0) {
    logMain(LOG_ERR, "regex fails to retrieve collection from url: %s", 
	    path);
  }
  regfree(&motif_compile);
  return rc;
}


/*=======================================================================
 * Function   : usageHtml
 * Description: Send the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : char* programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static void usageHtml(Collection* coll, char* programName)
{
  logMain(LOG_INFO, "sending usage message");

  fprintf(stdout, "Content-Type: text/html\r\n");
  fprintf(stdout, "\r\n");

  sendTemplate(coll, "cgiHeader.shtml");
 
  fprintf(stdout, "<br>Internal error, sorry\r\n<br>");
  fprintf(stdout, "<h5><i>methode = %s\r\n<br>", getenv("REQUEST_METHOD"));
  fprintf(stdout, "query = %s\r\n<br>", getenv("QUERY_STRING"));
  fprintf(stdout, "path = %s\r\n<br></h5></i>", getenv("SCRIPT_FILENAME"));
  
  fprintf(stdout, "<br>The usage for %s is:\r\n<br>", 
	  basename(programName));
  fprintf(stdout, 
	  "<h5><i>url?hash=HASH&size=SIZE<br>"
	  "\twhere:\r\n<br>"
	  "\t\tHASH   : the md5sum of the requested file\r\n<br>"
	  "\t\tSIZE   : the size of the requested file\r\n<br></h5></i>");
  
  sendTemplate(coll, "footer.html");
  return;
}


/*=======================================================================
 * Function   : iamAloneHtml
 * Description: Send a specific error message
 * Synopsis   : static void usage(char* programName)
 * Input      : char* programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static void iamAloneHtml(Collection* coll, char* programName)
{
  logMain(LOG_INFO, "sending no server awake message");

  fprintf(stdout, "Content-Type: text/html\r\n");
  fprintf(stdout, "\r\n");

  sendTemplate(coll, "cgiHeader.shtml");
 
  fprintf(stdout, "<br>Server has gone, sorry\r\n<br>");
  
  fprintf(stdout, "<br>To launch server do:\r\n<br>");
  fprintf(stdout, "<h5><i>/etc/init.d/mediatexd start</h5></i>");
  
  sendTemplate(coll, "footer.html");
  return;
}


/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static void 
usage(char* programName)
{
  fprintf(stderr,
	  "`" PACKAGE_NAME "' "
	  "is an Electronic Record Management System (ERMS), "
	  "focusing on the archival storage entity define by the `OAIS' "
	  "draft and on the `NF Z 42-013' requirements.\n");

  mdtxUsage(programName);
  mdtxOptions();

  fprintf(stderr, "\nEnvironment:\n"
	  "This program should be served by Apache "
	  "and then called by your browser."
	  "So as to test it, call it like as described bellow.\n"
	  "  ask for content:  (get query)\n"
	  "REQUEST_METHOD=GET"
	  " QUERY_STRING=\"hash=HASH&size=SIZE\""
	  " SCRIPT_FILENAME=/MDTX-COLL/public_html/cgi/get.cgi"
	  " cgi [OPTIONS]\n"
	  "  provide email:    (put query)\n"
	  "echo \"hash=HASH&size=SIZE&mail=MAIL\" |"
	  " REQUEST_METHOD=POST"
	  " SCRIPT_FILENAME=/MDTXUSER-COLL/public_html/cgi/get.cgi"
	  " CONTENT_TYPE=application/x-www-form-urlencoded"
	  " CONTENT_LENGTH=64"
	  " cgi [OPTIONS]\n");

  fprintf(stderr, "\nExamples:\n"
	  "  ask for content:  (get query)\n"
	  "REQUEST_METHOD=GET"
	  " QUERY_STRING=\"hash=40485334450b64014fd7a4810b5698b3&size=12\""
	  " SCRIPT_FILENAME=/mdtx-hello/public_html/cgi/get.cgi" 
	  " cgi\n"
	  "  provide email:    (put query)\n"
	  "echo \"hash=40485334450b64014fd7a4810b5698b3&size=12&mail=test@test.org\" |"
	  " REQUEST_METHOD=POST"
	  " SCRIPT_FILENAME=/mdtx-hello/public_html/cgi/get.cgi"
	  " CONTENT_TYPE=application/x-www-form-urlencoded"
	  " CONTENT_LENGTH=64"
	  " cgi\n");

  mdtxHelp();

  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/11/04
 * Description: CGI script
 * Synopsis   : mdtx-client
 * Input      : CGI context given by Apache
 * Output     : url redirection that point on a mdtx-server

 (gdb) set env REQUEST_METHOD GET 
 (gdb) set env QUERY_STRING hash=40485334450b64014fd7a4810b5698b3&size=12 
 (gdb) set env SCRIPT_FILENAME /mdtx-test1/public_html/cgi/mdtx.cgi 
 (gdb) set env MDTX_NO_REGRESSION 1
 =======================================================================*/
int 
main(int argc, char** argv)
{
  //Configuration* conf = 0;
  Collection* coll = 0;
  RecordTree* tree = 0;
  Record* record = 0;
  char* label = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS;
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  //env.logFacility = "local2";
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  logMain(LOG_INFO, "* mediatex cgi script *");

  // match collection
  if ((label = getIndexLabel()) == 0) {
    usage(programName);
    goto error;
  }

  logMain(LOG_DEBUG, "%s-cgi lunched from %s collection", 
	  env.confLabel, label);

  // load configuration (goto error as htmlError need a coll)
  if (!(coll = mdtxGetCollection(label))) goto error;
  
  // build record query from query parameters
  if ((tree = scanCgiQuery(coll)) == 0) goto htmlError;
  if (!(record = (Record*)tree->records->head->it)) goto htmlError;
  
  // relay query to daemon(s)
  if (isEmptyString(record->extra) || record->extra[0]=='!') {
    // first call: ask if servers have the record in cache
    if (!loadCollection(coll, SERV)) goto error;
    if (!mdtxFind(tree)) goto iamAloneHtmlError;
    if (!releaseCollection(coll, SERV)) goto error;
  }
  else {
    // second call: ask local server remind the provided mail
    if (!mdtxRegister(tree)) goto error;
  }

  rc = TRUE;
  /************************************************************************/

 iamAloneHtmlError:
  if (!rc) iamAloneHtml(coll, programName);
  goto error;
 htmlError:
  if (!rc) usageHtml(coll, programName);
 error:
  destroyString(label);
  destroyRecordTree(tree);
  freeConfiguration();
  if (confLabel) free(confLabel);
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
