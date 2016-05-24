/*=======================================================================
 * Project: MediaTex
 * Module : unit tests
 *
 * test for log

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

#include "mediatex.h"

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
  miscUsage(programName);
  miscOptions();

  return;
}

/*=======================================================================
 * Function   : main
 * Description: Unit test for log module.
 * Synopsis   : utlog
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS;
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }
   
  // set the log handler
  if (!(env.logHandler = 
	logOpen(programName, env.logFacility, env.logSeverity, 
		env.logFile))) goto error;

  // lowlevel API
  logEmitFunc(env.logHandler, LOG_INFO, "test using %s", "logEmitFunc");
  logEmitMacro(env.logHandler, LOG_INFO, __FILE__, __LINE__, 
	       "test using %s", "logEmitMacro");
  logEmit(LOG_MAIN, LOG_INFO, "test using %s", "logEmit");

  // facilities
  logMain(LOG_INFO, "");
  logMain(LOG_INFO, "*******************************");
  logMain(LOG_INFO, "* here is somes explanation : *");
  logMain(LOG_INFO, "*******************************");

  logMain(LOG_INFO, "- default is \'./utlog -f file\'");
  logMain(LOG_INFO,
  	  "  this will not send log to syslog but directely at screen");
  logMain(LOG_INFO, "- try \'./utlog -f file -l xxx.log\'");
  logMain(LOG_INFO, "  this will write logs into the file");
  logMain(LOG_INFO, "- try \'./utlog -f user\'");
  logMain(LOG_INFO, 
	  "  in order to use syslog daemon (check /var/log/syslog)");

  // severities
  logMain(LOG_INFO, "");
  logMain(LOG_INFO, "**************");
  logMain(LOG_INFO, "* severities *");
  logMain(LOG_INFO, "**************");
  logMain(LOG_EMERG, "%s",
  	  "A panic condition was reported to all processes.");
  logMain(LOG_ALERT, "%s",
  	  "A condition that should be corrected immediately.");
  logMain(LOG_CRIT, "A critical condition.");
  logMain(LOG_ERR, "An error message.");
  logMain(LOG_WARNING, "A warning message.");
  logMain(LOG_NOTICE, "A condition requiring special handling.");
  logMain(LOG_INFO, "%s (%lli)",
  	  "A general information message.", (off_t)42);
  logMain(LOG_DEBUG, "%s (%i)",
  	  "A message useful for debugging programs.", 42);
  logMain(LOG_INFO, "");
  logMain(LOG_INFO,
  	  "Severity allows to ignore messages from lower severity");
  logMain(LOG_INFO,
  	  "default severity is \'-s info\' for unit tests,");
  logMain(LOG_INFO,
  	  "and \'-s notice\' for mediatex binaries");
  logMain(LOG_INFO,
	 "- try \'./utlog -f file -s debug\' to see debug message");
  logMain(LOG_DEBUG,
  	    "<<< this is a debug message >>>");
  
  // modules
  logMain(LOG_INFO, "");
  logMain(LOG_INFO, "**************");
  logMain(LOG_INFO, "* modules *");
  logMain(LOG_INFO, "**************");
  logAlloc(LOG_INFO, __FILE__, __LINE__, "test using %s", "logAlloc");
  logScript(LOG_INFO, "test using %s", "logScript");
  logMisc(LOG_INFO, "test using %s", "logMisc");
  logMemory(LOG_INFO, "test using %s", "logMemory");
  logParser(LOG_INFO, "test using %s", "logParser");
  logCommon(LOG_INFO, "test using %s", "logCommon");
  logMain(LOG_INFO, "test using %s", "logMain");
  logMain(LOG_INFO, "");
  logMain(LOG_INFO, "Each module has its own severity");
  logMain(LOG_INFO, "- try \'./utlog -s error -s info:misc,main\'");
  logMain(LOG_INFO, 
	  "  this select >=info messages from main and misc modules");
   logMisc(LOG_INFO,
  	    "<<< this is an info message from MISC module >>>");
  	
  // test re-opening the logs
  env.logHandler = logClose(env.logHandler);
  if (!(env.logHandler = 
	logOpen(programName, getLogFacility("local4"),
		env.logSeverity, 0))) goto error;
  
  logMain(LOG_INFO, "%s",
  	  "re-open log using local4 facility");
  env.logHandler = logClose(env.logHandler);

  if (!(env.logHandler = 
	logOpen(programName, env.logFacility,
		env.logSeverity, env.logFile))) 
    goto error;

  logMain(LOG_INFO, "%s",
  	  "re-open log success");

  rc = TRUE;
 error:	
  if(env.logFile) {
    free(env.logFile);
  }
  env.logHandler = logClose(env.logHandler);
  rc=!rc;
 optError:
  exit(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
