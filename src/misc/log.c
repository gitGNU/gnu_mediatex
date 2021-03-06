/*=======================================================================
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

/* Note: Do not include "mediatex-config.h" here as to not use
 * malloc. (because malloc use logging and we need it enabled first) */
#include "mediatex.h"


static LogFacility LogFacilities[] = {
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
  {99,            "file"},
  {-1,            (char*)0}};
 
/*=======================================================================
 * Function   : getLogFacility
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

  if(name) {
    LogFacility* facility = LogFacilities;
    
    for(; facility->name; facility++) {
      if(strcmp(facility->name, name) == 0) {
	rc = facility->code;
      }
    }
  }
  
  return(rc);
}

/*=======================================================================
 * Function   : getLogFacilityByName
 * Description: Get the log facility corresponding to a given name.
 * Synopsis   : LogFacility* getLogFacilityByName(char* name)
 * Input      : char* name = the sought facility name.
 * Output     : the address of the facility descriptor or nil if there
 *              is no facility corresponding to the given name.
 =======================================================================*/
LogFacility* 
getLogFacilityByName(char* name)
{
  LogFacility* rc = 0;

  if(name) {
    LogFacility* facility = LogFacilities;
    
    for(; facility->name; facility++) {
      if(strcmp(facility->name, name) == 0) {
	rc = facility;
      }
    }
  }
	
  return(rc);
}

/*=======================================================================
 * Function   : getLogFacilityByCode
 * Description: Get the log facility corresponding to a given code.
 * Synopsis   : LogFacility* getLogFacilityByCode(int code)
 * Input      : int code = the sought facility code.
 * Output     : the address of the facility descriptor or nil if there
 *              is no facility corresponding to the given code.
 =======================================================================*/
LogFacility* 
getLogFacilityByCode(int code)
{
  LogFacility* rc = 0;

  LogFacility* facility = LogFacilities;

  for(; facility->name; facility++) {
    if(code == facility->code) {
      rc = facility;
    }
  }
  
  return(rc);
}


// see values in /usr/include/i386-linux-gnu/sys/syslog.h (from 0 to 7)
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
 * Function   : getLogSeverity
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

  if(name) {
    LogSeverity* severity = LogSeverities;
    
    for(; severity->name; severity++) {
      if(strcmp(severity->name, name) == 0) {
	rc = severity->code;
      }
    }
  }
  
  return(rc);
}

/*=======================================================================
 * Function   : getLogSeverityByName
 * Description: Get the log severity corresponding to a given name.
 * Synopsis   : LogSeverity* getLogSeverityByName(char* name)
 * Input      : char* name = the sought severity name.
 * Output     : the address of the corrsponding severity or nil if
 *              there is no severity corresponding to the given name.
 =======================================================================*/
LogSeverity* 
getLogSeverityByName(char* name)
{
  LogSeverity* rc = 0;

  if(name) {
    LogSeverity* severity = LogSeverities;
    
    for(; severity->name; severity++) {
      if(strcmp(severity->name, name) == 0) {
	rc = severity;
      }
    }
  }
  
  return(rc);
}

/*=======================================================================
 * Function   : getLogSeverityByCode
 * Description: Get the log severity corresponding to a given name.
 * Synopsis   : LogSeverity* getLogSeverityByCode(int code)
 * Input      : int code = the sought severity code.
 * Output     : the address of the corrsponding severity or nil if
 *              there is no severity corresponding to the given code.
 =======================================================================*/
LogSeverity* 
getLogSeverityByCode(int code)
{
  LogSeverity* rc = 0;

  LogSeverity* severity = LogSeverities;

  for(; severity->name; severity++) {
    if(code == severity->code) {
      rc = severity;
    }
  }
  
  return(rc);
}

/*=======================================================================
 * Function   : logOpen
 * Description: create a log handler
 * Synopsis   : LogHandler* logOpen(char* name, 
 *                  LogSeverity** logSeverity, int severity, char* hlog)
 * Input      : char* name = name to recordin the log line's prefix
 *              int facility = facility code
 *              int* logSeverity = severity codes
 *              char* logFile = the log stream file name if the
 *              facility is MISC_LOG_FILE.
 * Output     : the address of the associated log handler or nil if
 *              the opening failed.
 =======================================================================*/
LogHandler* 
logOpen(char* name, int facility, int* logSeverity, 
	char* logFile)
{
  static const char* NilName = "unknown:nil";
  static const char* EmptyName = "unknown:empty";
	
  LogHandler* rc = 0;
  int fopenError = FALSE;
  char* baseName = 0;
  int i = 0;
	
  if (!(rc = (LogHandler*)malloc(sizeof(LogHandler)))) {
    fprintf(stderr, "error: fails to allocate log handler\n");
    goto error;
  }

  memset(rc, 0, sizeof(LogHandler)); 
				
  if(name == 0) {
    name = (char*)NilName;
  }
  else {
    if(strlen(name) == 0){
      name = (char*)EmptyName;
    }
  }

  baseName = strrchr(name, '/');
  name = (baseName == 0) ? name : (baseName + 1);
    
  rc->name = (char*)malloc(sizeof(char) * (strlen(name) + 1));
  if(rc->name) {
    strcpy(rc->name, name);
      
    rc->facility = getLogFacilityByCode(facility);
    if(rc->facility == 0) {
      rc->facility = getLogFacilityByCode(MISC_LOG_FILE);
      logFile = 0;
    }

    for (i=0; i<LOG_MAX_MODULE; ++i) {
      rc->severity[i] = getLogSeverityByCode(logSeverity[i]);
      if(rc->severity[i] == 0) {
	rc->severity[i] = getLogSeverityByCode(LOG_NOTICE);
      }
    }
      
    if(rc->facility->code != MISC_LOG_FILE) {
      openlog(rc->name,
	      LOG_PID|LOG_NDELAY|LOG_NOWAIT,
	      rc->facility->code);
    }
    else {
      if(logFile && *logFile != (char)0) {
	rc->hlog = fopen(logFile, "a");
	if(rc->hlog == 0) {
	  fopenError = TRUE;
	}
      }
    }		

    if(rc->facility->code == MISC_LOG_FILE && rc->hlog == 0) {
      if(rc->hlog == 0) {
	rc->hlog = stderr;
	if(fopenError) {
	  logEmitMacro(rc, LOG_WARNING, __FILE__, __LINE__,
		      "cannot open '%s' file for logging, "
		      "will try to revert to the standard error", 
		      logFile);
	}
	//logMiscMacro(rc, LOG_DEBUG, __FILE__, __LINE__,
	//	       "logging to the standard error");
      }
    }
    //logEmitMacro(rc, LOG_DEBUG, __FILE__, __LINE__,
    //             "started");
  }
  else {
    free(rc);
    rc = 0;
  }

 error:
  return(rc);
}


/*=======================================================================
 * Function   : logEmitFunc
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
 * Note       : You should better call the logEmit variadic macros
 =======================================================================*/
void 
logEmitFunc(LogHandler* logHandler, int priority, const char* format, ...)
{
  va_list args;

  // print messages to stderr if logger is not yet initialise
  if(!logHandler) {
    fprintf(stderr, "logger not yet initialized... "); 
    va_start(args, format);
    fprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    goto end;
  }

  va_start(args, format);

  if (!(logHandler->hlog)) {
    // call syslog
    vsyslog(priority, format, args);
  }
  else {  
    // call fprintf
    vfprintf(logHandler->hlog, format, args);
    fprintf(logHandler->hlog, "\n");
  }
  
  va_end(args);

  end:
  return;
}


/*=======================================================================
 * Function   : logClose
 * Description: Close the syslog.
 * Synopsis   : void logClose(LogHandler* logHandler)
 * Input      : LogHandler* logHandler = the handler of the log to
 *              close.
 * Output     : N/A
 =======================================================================*/
LogHandler* 
logClose(LogHandler* logHandler)
{
  if(logHandler) {
    // "stopped"
		
    if(logHandler->hlog == 0) {
      closelog();
    }
    else {
      if(logHandler->hlog != stderr) {
	fclose(logHandler->hlog);
      }
      logHandler->hlog = 0;
    }

    if(logHandler->name) {
      free(logHandler->name);
    }
      
    free(logHandler);
  }
  
  return (LogHandler*)0;
}


/*=======================================================================
 * Function   : parseLogSeverityOption
 * Description: parse the --severity option value
 * Synopsis   : int parseLogOption(char* parameter, int* logSeverity)
 * Input      : char* parameter: the option value
 * Output     : int* logSeverity
 *              TRUE on success
 =======================================================================*/
int parseLogSeverityOption(char* parameter, int* logSeverity)
{
  int rc = 0;
  int i = 0;
  int iSev = -1;
  int iMod = -1;
  char delim = ':';
  char* ptr = parameter;
  LogSeverity* severity = LogSeverities;

  static char* module[] = {
    "alloc",
    "script",
    "misc",
    "memory",
    "parser",
    "common",
    "main",
  };

  // severity
  for(i=0; severity->name; ++severity,++i) {
    if (!strncmp(ptr, severity->name, strlen(severity->name))) {
      iSev = i;
      break;
    }
  }
  if (iSev == -1) {
    printf("incorrect severity name: %s\n", ptr);
    goto error;
  }

  // no module specified so all modules are inpacted
  if (strlen(ptr) == strlen(severity->name)) {
    for (i=0; i<LOG_MAX_MODULE; ++i) {

      // except alloc for debug that must be set explicitelly
      if (severity->code == LOG_DEBUG && i == LOG_ALLOC) continue;

      logSeverity[i] = severity->code;
    }
    goto end;
  }

  // severity:module(,module)*
  do {
    if (!(ptr = strchr(ptr, delim))) {
      fprintf(stderr, "bad syntaxe: '%s', "
	      "expected 'severity[:module(,module)*]'\n", parameter);
      goto error;
    }
    *ptr++ = 0;
    if (!*ptr) {
      *(ptr-1) = delim;
      fprintf(stderr, "null module: '%s', "
	      "expected 'severity[:module(,module)*]'\n", parameter);
      goto error;
    }
    for (i=0; i<LOG_MAX_MODULE; ++i) {
      if (!strncmp(ptr, module[i], strlen(module[i]))) {
	iMod = i;
	break;
      }
    }
    if (i == LOG_MAX_MODULE) {
      printf("incorrect module name: '%s'\n", ptr);
      goto error;
    }
    logSeverity[iMod] = severity->code;
    delim = ',';
  } while (strlen(ptr) > strlen(module[iMod]));
  
 end:
  rc = 1;
 error:
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
