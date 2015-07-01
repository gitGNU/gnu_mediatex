/*=======================================================================
 * Version: $Id: utlog.c,v 1.1 2015/07/01 10:49:56 nroche Exp $
 * Project: MediaTex
 * Module : unit tests
 *
 * test for log

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

#include "mediatex.h"
GLOBAL_STRUCT_DEF;

/*=======================================================================
 * Function   : usage (log) [MediaTeX]
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : char* programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static void 
usage(char* programName)
{
  fprintf(stderr, "The usage for %s is:\n", programName);
  fprintf(stderr, "\t%s ", programName);
  fprintf(stderr, "{ -h | "
	  "[ -f facility ] [ -s severity ] [ -l logFileName ] }");
  fprintf(stderr, "\twhere:\n");
  fprintf(stderr, "\t\t-f   : use facility for logging\n");
  fprintf(stderr, "\t\t-s   : use severity for logging\n");
  fprintf(stderr, "\t\t-l   : log to logFile\n");

  return;
}

/*=======================================================================
 * Function   : main (log) [MediaTeX]
 * Author     : Peter FELECAN
 * modif      : 2006/05/24 15:48:50
 * Description: Unit test for log module.
 * Synopsis   : utlog
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  int rc = 0;

  extern char* optarg;
  extern int optind;
  extern int opterr;
  extern int optopt;
  
  int cOption = EOF;

  char* programName = *argv;
  char* logFile = 0;
	
  int logFacility = -1;
  int logSeverity = -1;
	
  LogHandler* logHandler = 0;
	
  while(TRUE) {
    cOption = getopt(argc, argv, ":f:s:l:h");
    
    if(cOption == EOF) {
      break;
    }
    
    switch(cOption) {

    case 'f':
      if(optarg == 0) {
	fprintf(stderr, "%s: nil argument for the facility name\n", 
		programName);
	rc = 2;
      }
      else {
	if(!strlen(optarg)) {
	  fprintf(stderr, "%s: empty argument for the facility name\n", 
		  programName);
	  rc = 2;
	}
	else {
	  logFacility = getLogFacility(optarg);
	  if(logFacility == -1) {
	    fprintf(stderr, "%s: incorrect facility name '%s'\n", 
		    programName, optarg);
	    rc = 2;
	  }
	}
      }
      break;
      
    case 's':
      if(optarg == 0) {
	fprintf(stderr, "%s: nil argument for the severity name\n", 
		programName);
	rc = 2;
      }
      else {
	if(!strlen(optarg)) {
	  fprintf(stderr, "%s: empty argument for the severity name\n", 
		  programName);
	  rc = 2;
	}
	else {
	  logSeverity = getLogSeverity(optarg);
	  if(logSeverity == -1) {
	    fprintf(stderr, "%s: incorrect severity name '%s'\n", 
		    programName, optarg);
	    rc = 2;
	  }
	}
      }
      break;
	
    case 'l':
      if(optarg == 0) {
	fprintf(stderr, "%s: nil argument for the log stream\n", 
		programName);
	rc = 2;
      }
      else {
	if(!strlen(optarg)) {
	  fprintf(stderr, "%s: empty argument for the log stream\n", 
		  programName);
	  rc = 2;
	}
	else {
	  logFile = (char*)malloc(sizeof(char) * strlen(optarg) + 1);
	  if(logFile != 0) {
	    strcpy(logFile, optarg);
	  }
	  else {
	    fprintf(stderr, 
		    "%s: cannot allocate memory for the log stream name\n",
		    programName);
	    rc = 2;
	  }
	}
      }
      break;
      
    case 'h':
      usage(programName);
      rc = 4;
	  
      break;
	  
    case ':':
      usage(programName);
      rc = 125;

      break;

    case '?':
      usage(programName);
      rc = 126;

      break;

    default:
      usage(programName);
      rc = 127;
			
      break;
    }
  }

  if(rc) goto error;
  rc = 1;
   
  if (!(env.logHandler =
	logOpen(programName, logFacility, logSeverity, logFile)))
    goto error;

  logEmit(LOG_EMERG, "%s", 
	  "A panic condition was reported to all processes.");
  logEmit(LOG_ALERT, "%s", 
	  "A condition that should be corrected immediately.");
  logEmit(LOG_CRIT, "%s", 
	  "A critical condition.");
  logEmit(LOG_ERR, "%s", 
	  "An error message.");
  logEmit(LOG_WARNING, "%s", 
	  "A warning message.");
  logEmit(LOG_NOTICE, "%s", 
	  "A condition requiring special handling.");
  logEmit(LOG_INFO, "%s (%lli)", 
	  "A general information message.", (off_t)42);
  logEmit(LOG_DEBUG, "%s (%i)", 
	  "A message useful for debugging programs.", 42);

  logEmit(LOG_INFO, "%s", "");
  logEmit(LOG_INFO, "%s", "here is somes explanation :");
  logEmit(LOG_INFO, "%s", "");

  logEmit(LOG_INFO, "%s", 
	  "try \'./utlog -f file\' for testing");
  logEmit(LOG_INFO, "%s", 
	  "this will not send log to syslog but display them "
	  "at screen");
  logEmit(LOG_INFO, "%s", 
	  "do it because syslog is configured to hide debug "
	  "informations.");
  logEmit(LOG_INFO, "%s", "");

  logEmit(LOG_INFO, "%s", 
	  "\'./utlog -f file -l xxx.log\' will put them "
	  "into the file");
  logEmit(LOG_INFO, "%s", 
	  "To use syslog you must provide a facility");
  logEmit(LOG_INFO, "%s", 
	  "I advice you to use \'-f user\' or \'-f local[0..7]\'");
  logEmit(LOG_INFO, "%s", "");

  logEmit(LOG_INFO, "%s", 
	  "Severity allows to ignore messages from lower severity");
  logEmit(LOG_INFO, "%s", 
	  "default severity is \'-s info\' as you can read it");
  logEmit(LOG_INFO, "%s", 
	  "in fact this only hide debug messages");
  logEmit(LOG_INFO, "%s", "");

  logEmit(LOG_INFO, "%s", 
	  "try \'./utlog -f file -s debug\' to see debug info");
  logEmit(LOG_INFO, "%s", "");	
  logEmit(LOG_DEBUG, "%s", 
	  "like that, ok now you can have debuging info");
  logEmit(LOG_DEBUG, "%s", "");

  // test the parser maccro
  logParser(LOG_INFO, "%s", 
	    "You shouldn't read this !");
  logParser(LOG_INFO, "%s", 
	    "This is a message from the parser macro");
	
  // test re-opening the logs
  env.logHandler = logClose(env.logHandler);
  if (!(env.logHandler = logOpen(programName, getLogFacility("local4"), 
				 logSeverity, logFile))) goto error;

  logEmit(LOG_INFO, "%s", 
	  "re-opening log using local4 facility");
  env.logHandler = logClose(env.logHandler);

  if (!(env.logHandler = logOpen(programName, getLogFacility("file"), 
				 logSeverity, logFile))) goto error;

  logEmit(LOG_INFO, "%s", 
	  "test success for re-opening log using file facility");
  env.logHandler = logClose(env.logHandler);

  if(logFile != 0) {
    free(logFile);
  }

  rc = 0;
 error:	
  exit(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
