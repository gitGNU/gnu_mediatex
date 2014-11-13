/*=======================================================================
 * Version: $Id: log.c,v 1.2 2014/11/13 16:36:41 nroche Exp $
 * Project: MediaTeX
 * Module : log
 *
 * Logging module implementation.
 * This file was originally written by Peter Felecan under GNU GPL
 * Uses semantics taken from RFC 3165 The BSD syslog Protocol.
 * Instrumentation:
 *
 * Solaris:
 *	/etc/syslog.conf:
 *		local3.debug /var/log/mediatex
 *	# svcadm restart svc:/system/system-log:default
 *
 *	/etc/logadm.conf:
 *		/var/log/mediatex -C 10 -N -c -s 32m -z 0
 *	for:
 *		-c     Rotate the log file by copying it and  trun-
 *                     cating  the original logfile to zero length,
 *                     rather than renaming the file.
 *		-s 32m Rotate a log file when it exceeds 32MB.
 *		-C 10  Keep one month worth of log files
 *              -N     Prevent an error message if the specified
 *                     logfile does not exist.
 *		-z 0   Compress old log files as they are  created.
 *                     The compression is done with gzip(1) and the
 *                     resulting log file has the suffix of .gz.
 *
 * 	Verify the configuration file:
 *	# logadm -V
 *
 * GNU/Linux: 
 *	/etc/syslog.conf:
 *		local3.debug /var/log/mediatex
 *	# kill -1 $(cat /var/run/syslogd.pid)
 *
 *      edit/create /etc/logrotate.d/mediatex with the following content:
 *      (it's roughly equivalent to the Solaris version)
 *      /var/log/scoredit {
 *         daily
 *         rotate 10
 *         size 32M
 *         compress
 *         copytruncate
 *         missingok
 *      }
 *
 
 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Felecan Peter

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

#include "log.h"

LogHandler* DefaultLog = NULL;

/*=======================================================================
 * Function   : logDefault (log) [MediaTeX]
 * Description: Set the default log handler --- i.e., all the log
 *              is done using this handler.
 * Synopsis   : LogHandler* logDefault(LogHandler* logHandler)
 * Input      : LogHandler* logHandler = the new default log;
 * Output     : the handler of the new log; nil if error.
 =======================================================================*/
LogHandler* 
logDefault(LogHandler* logHandler)
{
  LogHandler* rc = NULL;

  if(logHandler != NULL) {
    DefaultLog = logHandler;
  }

  rc = DefaultLog;	
  return(rc);
}

/*=======================================================================
 * Function   : logOpen (log) [MediaTeX]
 * Description: Depending on the facility, severity and log handler
 *              given, the logging is set up.
 * Synopsis   : LogHandler* logOpen(char* name, int facility, int severity,
 *              ioStream* hlog)
 * Input      : char* name = name to recordin the log line's prefix
 *              int facility = facility code
 *              int severity = severity code
 *              char* logFile = the log stream file name if the
 *              facility is MISC_LOG_FILE.
 * Output     : the address of the associated log handler or nil if
 *              the opening failed.
 =======================================================================*/
LogHandler* 
logOpen(char* name, int facility, int severity, char* logFile)
{
  static const char* NilName = "unknown:nil";
  static const char* EmptyName = "unknown:empty";
	
  LogHandler* rc = NULL;
  int fopenError = FALSE;
	
  rc = (LogHandler*)malloc(sizeof(LogHandler));

  // rc->hlog is sometine already defined when re-opening the log
  memset(rc, 0, sizeof(LogHandler)); 
				
  if(rc != NULL) {
    if(name == NULL) {
      name = (char*)NilName;
    }
    else {
      if(strlen(name) == 0){
	name = (char*)EmptyName;
      }
    }

    char* baseName = strrchr(name, '/');
    name = (baseName == NULL) ? name : (baseName + 1);
    
    rc->name = (char*)malloc(sizeof(char) * (strlen(name) + 1));
    if(rc->name != NULL) {
      strcpy(rc->name, name);
      
      rc->facility = getLogFacilityByCode(facility);
      if(rc->facility == NULL) {
	rc->facility = getLogFacilityByCode(MISC_LOG_FILE);
	logFile = NULL;
      }
      
      rc->severity = getLogSeverityByCode(severity);
      if(rc->severity == NULL) {
	rc->severity = getLogSeverityByCode(LOG_INFO);
      }
      
      if(rc->facility->code != MISC_LOG_FILE) {
	openlog(rc->name,
		LOG_PID|LOG_NDELAY|LOG_NOWAIT,
		rc->facility->code);
      }
      else {
	if(logFile != NULL && *logFile != (char)0) {
	  rc->hlog = fopen(logFile, "a");
	  if(rc->hlog == NULL) {
	    fopenError = TRUE;
	  }
	}
      }

      //memset((void*)(&(rc->uname)), '\0', sizeof(struct utsname));
      //uname(&(rc->uname));
      //rc->pid = getpid();			

      if(rc->facility->code == MISC_LOG_FILE && rc->hlog == NULL) {
	if(rc->hlog == NULL) {
	  rc->hlog = (FILE*)stderr;
	  if(fopenError) {
	    logEmit(LOG_WARNING, 
		    "cannot open '%s' file for logging, "
		    "will try to revert to the standard error", 
		    logFile);
	  }
	  //logEmit(LOG_INFO, "logging to the standard error");
	}
      }
      
      //logEmit(LOG_INFO, "started");
    }
    else {
      free(rc);
      rc = NULL;
    }
  }
	
  return(rc);
}


#define TimeBufLen 32

/*=======================================================================
 * Function   : logEmitFunc (log) [MediaTeX]
 * Description: Emit a log message.
 * Synopsis   : void logEmitFunc(LogHandler* logHandler, int priority,
 *              const char* message, va_list ap)
 * Input      : LogHandler* logHandler = the log handler returned by a
 *				openLog()
 *              int priority = the priority of the message --- see
 *              syslog(3);
 *              const char* format = the format of the message --- see
 *              vsyslog(3).
 *              va_list ap = the pointer to the begining of the
 *              variable list of argument.
 * Output     : N/A
 =======================================================================*/
void 
logEmitFunc(LogHandler* logHandler, int priority, const char* format, ...)
{
  LogSeverity* severity = NULL;
  va_list args;

  if(logHandler != NULL) {
    if(format != NULL) {
      //severity = getLogSeverityByCode(priority); // speed-up
      severity = LogSeverities + priority;
      if(severity == NULL) {
	severity = getLogSeverityByCode(LOG_INFO);
      }

      if(severity->code <= logHandler->severity->code) {
	va_start(args, format);
	if(logHandler->hlog == NULL) {
	  vsyslog(priority, format, args);
	}
	else {
	  // The above code simulate the vsyslog output on stderr

/* 	  time_t tloc; */
/* 	  if(time(&tloc) != (time_t)-1) { */
/* 	    struct tm* ltime = localtime(&tloc); */
/* 	    if(ltime != (struct tm*)0) { */
/* 	      char timeBuf[TimeBufLen]; */
	      
/* 	      memset(timeBuf, '\0', TimeBufLen); */
/* 	      if(strftime(timeBuf, TimeBufLen, "%b %d %H:%M:%S",ltime)) { */
/* 		fprintf(logHandler->hlog, */
/* #if defined(__sun__) */
/* 			"%s %s[%d]: [ID 123456 %s.%s] ", */
/* #endif /\* : defined(__sun_) *\/ */
/* #if defined(__gnu_linux__) */
/* 			"%s %s[%d] %s: ", */
/* #endif /\* defined(__gnu_linux__) *\/ */
/* 			timeBuf, */
/* 			//logHandler->uname.nodename, */
/* 			logHandler->name, */
/* 			logHandler->pid, */
/* #if defined(__sun__) */
/* 			logHandler->facility->name, */
/* 			severity->name */
/* #endif /\* : defined(__sun_) *\/ */
/* #if defined(__gnu_linux__) */
/* 			severity->name */
/* #endif /\* defined(__gnu_linux__) *\/ */
/* 			); */
/* 	      } */
/* 	      else { */
/* 		fprintf(logHandler->hlog, "error: too many characters in the time prefix "); */
/* 	      } */
/* 	    } */
/* 	    else { */
/* 	      fprintf(logHandler->hlog, "error: cannot get the local time "); */
/* 	    } */
/* 	  } */
/* 	  else { */
/* 	    fprintf(logHandler->hlog, "error: cannot get the time "); */
/* 	  } */

	  /* TODO: I would like to have an ioVPrintf: */
	  vfprintf(logHandler->hlog, format, args);
	  if(*(format + strlen(format)) != '\n') {
	    fprintf(logHandler->hlog, "\n");
	  }
	}

	va_end(args);
      }
    }
    else {
      logEmitFunc(DefaultLog, LOG_WARNING, "nil log format");
    }
  }
  else {
    fprintf((FILE*)stderr, "\nerror: nil log handler\n");
  }
  
  return;
}


/*=======================================================================
 * Function   : logClose (log) [MediaTeX]
 * Description: Close the syslog.
 * Synopsis   : void logClose(LogHandler* logHandler)
 * Input      : LogHandler* logHandler = the handler of the log to
 *              close.
 * Output     : N/A
 =======================================================================*/
LogHandler* 
logClose(LogHandler* logHandler)
{
  if(logHandler != NULL) {
      //logEmit(LOG_INFO, "stopped");
		
      if(logHandler->hlog == NULL) {
	closelog();
      }
      else {
	if(logHandler->hlog != (FILE*)stderr) {
	  fclose(logHandler->hlog);
	}
	logHandler->hlog = NULL;
      }

      if(logHandler->name != NULL) {
	free(logHandler->name);
      }
      
      free(logHandler);
  }
  
  return (LogHandler*)0;
}


LogFacility LogFacilities[] = {
  {LOG_KERN,      "kernel"},
  {LOG_USER,      "user"},
  {LOG_MAIL,      "mail"},
  {LOG_DAEMON,    "daemon"},
  {LOG_AUTH,      "auth"},
  {LOG_SYSLOG,    "syslog"},
  {LOG_LPR,       "lpr"},
  {LOG_NEWS,      "news"},
  {LOG_UUCP,      "uucp"},
#if defined(__sun__)
  {LOG_AUDIT,     "audit"},
#endif /* : defined(__sun_) */
  {LOG_CRON,      "cron"},
#if defined(__gnu_linux__)
  {LOG_AUTHPRIV,  "authpriv"},
  {LOG_FTP,       "ftp"},
#endif /* defined(__gnu_linux__) */
  {LOG_LOCAL0,    "local0"},
  {LOG_LOCAL1,    "local1"},
  {LOG_LOCAL2,    "local2"},
  {LOG_LOCAL3,    "local3"},
  {LOG_LOCAL4,    "local4"},
  {LOG_LOCAL5,    "local5"},
  {LOG_LOCAL6,    "local6"},
  {LOG_LOCAL7,    "local7"},
  {MISC_LOG_FILE, "file"},
  {-1,            (char*)0}};
 
/*=======================================================================
 * Function   : getLogFacility (log) [MediaTeX]
 * Description: Get the log facility corresponding to a given name.
 * Synopsis   : int getLogFacility(char* name)
 * Input      : char* name = the sought facility name.
 * Output     : the facility code or -1 if there is no facility
 *              corresponding to the given name.
 =======================================================================*/
int 
getLogFacility(char* name)
{
  int rc = (int)-1;

  if(name != NULL) {
    LogFacility* facility = LogFacilities;
    
    for(; facility->name != NULL; facility++) {
      if(strcmp(facility->name, name) == 0) {
	rc = facility->code;
      }
    }
  }
  
  return(rc);
}

/*=======================================================================
 * Function   : getLogFacilityByName (log) [MediaTeX]
 * Description: Get the log facility corresponding to a given name.
 * Synopsis   : LogFacility* getLogFacilityByName(char* name)
 * Input      : char* name = the sought facility name.
 * Output     : the address of the facility descriptor or nil if there
 *              is no facility corresponding to the given name.
 =======================================================================*/
LogFacility* 
getLogFacilityByName(char* name)
{
  LogFacility* rc = NULL;

  if(name != NULL) {
    LogFacility* facility = LogFacilities;
    
    for(; facility->name != NULL; facility++) {
      if(strcmp(facility->name, name) == 0) {
	rc = facility;
      }
    }
  }
	
  return(rc);
}

/*=======================================================================
 * Function   : getLogFacilityByCode (log) [MediaTeX]
 * Description: Get the log facility corresponding to a given code.
 * Synopsis   : LogFacility* getLogFacilityByCode(int code)
 * Input      : int code = the sought facility code.
 * Output     : the address of the facility descriptor or nil if there
 *              is no facility corresponding to the given code.
 =======================================================================*/
LogFacility* 
getLogFacilityByCode(int code)
{
  LogFacility* rc = NULL;

  LogFacility* facility = LogFacilities;

  for(; facility->name != NULL; facility++) {
    if(code == facility->code) {
      rc = facility;
    }
  }
  
  return(rc);
}


LogSeverity LogSeverities[] = {
  {LOG_EMERG,   "emerg"},
  {LOG_ALERT,   "alert"},
  {LOG_CRIT,    "crit"},
  {LOG_ERR,     "err"},
  {LOG_WARNING, "warning"},
  {LOG_NOTICE,  "notice"},
  {LOG_INFO,    "info"},
  {LOG_DEBUG,   "debug"},
  {-1,      (char*)0}};

/*=======================================================================
 * Function   : getLogSeverity (log) [MediaTeX]
 * Description: Get the log severity corresponding to a given name.
 * Synopsis   : int getLogSeverity(char* name)
 * Input      : char* name = the sought severity name.
 * Output     : the severity code or -1 if there is no severity
 *              corresponding to the given name.
 =======================================================================*/
int 
getLogSeverity(char* name)
{
  int rc = (int)-1;

  if(name != NULL) {
    LogSeverity* severity = LogSeverities;
    
    for(; severity->name != NULL; severity++) {
      if(strcmp(severity->name, name) == 0) {
	rc = severity->code;
      }
    }
  }
  
  return(rc);
}

/*=======================================================================
 * Function   : getLogSeverityByName (log) [MediaTeX]
 * Description: Get the log severity corresponding to a given name.
 * Synopsis   : LogSeverity* getLogSeverityByName(char* name)
 * Input      : char* name = the sought severity name.
 * Output     : the address of the corrsponding severity or nil if
 *              there is no severity corresponding to the given name.
 =======================================================================*/
LogSeverity* 
getLogSeverityByName(char* name)
{
  LogSeverity* rc = NULL;

  if(name != NULL) {
    LogSeverity* severity = LogSeverities;
    
    for(; severity->name != NULL; severity++) {
	if(strcmp(severity->name, name) == 0) {
	  rc = severity;
	}
    }
  }
  
  return(rc);
}

/*=======================================================================
 * Function   : getLogSeverityByCode (log) [MediaTeX]
 * Description: Get the log severity corresponding to a given name.
 * Synopsis   : LogSeverity* getLogSeverityByCode(int code)
 * Input      : int code = the sought severity code.
 * Output     : the address of the corrsponding severity or nil if
 *              there is no severity corresponding to the given code.
 =======================================================================*/
LogSeverity* 
getLogSeverityByCode(int code)
{
  LogSeverity* rc = NULL;

  LogSeverity* severity = LogSeverities;

  for(; severity->name != NULL; severity++) {
    if(code == severity->code) {
      rc = severity;
    }
  }
  
  return(rc);
}

/************************************************************************/

#ifdef utMAIN
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
  char* logFile = NULL;
	
  int logFacility = -1;
  int logSeverity = -1;
	
  LogHandler* logHandler = NULL;
	
  while(TRUE) {
    cOption = getopt(argc, argv, ":f:s:l:h");
    
    if(cOption == EOF) {
      break;
    }
    
    switch(cOption) {

    case 'f':
      if(optarg == NULL) {
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
      if(optarg == NULL) {
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
      if(optarg == NULL) {
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
	  if(logFile != NULL) {
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

  if(!rc)
    {
      logHandler = logOpen(programName, logFacility, logSeverity, logFile);
      if(logHandler != NULL) {

	  logDefault(logHandler);
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
      }
	
      // test re-opening the logs
      logClose(logHandler);
      logHandler = logOpen(programName, 
			   getLogFacility("local4"), logSeverity, logFile);
      logDefault(logHandler);

      logEmit(LOG_INFO, "%s", 
	      "test fails: re-opening log using local4 facility");
      logClose(logHandler);

      logHandler = logOpen(programName, 
			   getLogFacility("file"), logSeverity, logFile);
      logDefault(logHandler);

      logEmit(LOG_INFO, "%s", 
	      "test success for re-opening log using file facility");
      logClose(logHandler);

      logHandler = logOpen(programName, 
			   getLogFacility("local4"), logSeverity, logFile);
      logDefault(logHandler);

      logEmit(LOG_INFO, "%s", 
	      "second test fails: re-opening log using local4 facility");
      logClose(logHandler);
    }

  if(logFile != NULL) {
    free(logFile);
  }
	
  exit(rc);
}
#endif	/*	: utMAIN	*/

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
