/*=======================================================================
 * Version: $Id: log.h,v 1.3 2015/06/03 14:03:46 nroche Exp $
 * Project: MediaTex
 * Module : log
 *
 * Logging module public interface.
 * This file was originally written by Peter Felecan under GNU GPL

    MediaTex is an Electronic Records Management System
    Copyright (C) 2012  Roche Nicolas

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

#ifndef MISC_LOG_H
#define MISC_LOG_H 1

#include "../mediatex.h"

#include <syslog.h>
#include <sys/utsname.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>

// Manage log function for parser (very verbose when debuging other stuffs)
extern int debugParser;

typedef struct LogFacility
{
	int code;
	char* name;
}
	LogFacility;

extern LogFacility LogFacilities[];

extern int getLogFacility(char* name);
extern LogFacility* getLogFacilityByName(char* name);
extern LogFacility* getLogFacilityByCode(int code);

typedef struct LogSeverity
{
	int code;
	char* name;
}
	LogSeverity;

extern LogSeverity LogSeverities[];

extern int getLogSeverity(char* name);
extern LogSeverity* getLogSeverityByName(char* name);
extern LogSeverity* getLogSeverityByCode(int code);

#define MISC_LOG_FILE 99

typedef struct LogHandler
{
	char* name;
	LogFacility* facility;
	LogSeverity* severity;
	FILE* hlog;
  //struct utsname uname;
  //pid_t pid;
}
	LogHandler;

extern LogHandler* DefaultLog;

extern LogHandler* logDefault(LogHandler* logHandler);
extern LogHandler* logOpen(char* name, int facility, int severity, 
			   char* logFile);
extern void logEmitFunc(LogHandler* logHandler, int priority, 
			const char* format, ...);
extern LogHandler* logClose(LogHandler* logHandler);

/*=======================================================================
 * Macro      : logEmit
 * Author(s)  : Nicolas Roche
 * Date begin : 2012/05/01
 *     change : 2012/05/01
 * Description: Add file and line to the log (so as to debug more easily)
 * Synopsis   : void logParser(level, format, ...)
 * Input      : Wrapper for logEmitFunc function
 * Output     : N/A
 * Note       : path are troncated to basenames so as to make compatible
 *              "make check" and "make distcheck" output
 =======================================================================*/
#define logEmit(priority, format, ...) do {				\
    /* need local variables to manage concurent calls */		\
    char *file;								\
    char logBuffer[256] = LOG_TEMPLATE;					\
    strncpy(logBuffer+MISC_LOG_OFFSET, format, 256-MISC_LOG_OFFSET);	\
    for (file = __FILE__ + strlen(__FILE__);				\
	 file > (char*)__FILE__ && *(file-1) != '/';			\
	 --file);							\
    logEmitFunc(DefaultLog, priority, logBuffer,			\
		(LogSeverities + (priority))->name,			\
		file, __LINE__, __VA_ARGS__);				\
  } while (0)

/*=======================================================================
 * Macro      : logAlloc
 * Author(s)  : Nicolas Roche
 * Date begin : 2012/05/01
 *     change : 2012/05/01
 * Description: Special log function for the memory allocations
 * Synopsis   : void logAlloc(level, format, ...)
 * Input      : wrapper for logEmit maccro
 * Output     : N/A
 =======================================================================*/
#define logAlloc(priority, format, ...) {				\
    if (priority < LOG_INFO || env.debugMemory == TRUE) {		\
      logEmit(priority, format, __VA_ARGS__);				\
    }									\
  }									\

/*=======================================================================
 * Macro      : logMemory
 * Author(s)  : Nicolas Roche
 * Date begin : 2012/05/01
 *     change : 2012/05/01
 * Description: Special log function for the memory objects
 * Synopsis   : void logMemory(level, format, ...)
 * Input      : wrapper for logEmit maccro
 * Output     : N/A
 =======================================================================*/
#define logMemory(priority, format, ...) {				\
    if (priority < LOG_INFO || env.debugMemory == TRUE) {		\
      logEmit(priority, format, __VA_ARGS__);				\
    }									\
  }									\
    
/*=======================================================================
 * Macro      : logParser
 * Author(s)  : Nicolas Roche
 * Date begin : 2012/05/01
 *     change : 2012/05/01
 * Description: Special log function for the parsers
 * Synopsis   : void logParser(level, format, ...)
 * Input      : wrapper for logEmit maccro
 * Output     : N/A
 =======================================================================*/
#define logParser(priority, format, ...) {				\
    if (priority < LOG_INFO || env.debugParser == TRUE) {		\
      logEmit(priority, format, __VA_ARGS__);				\
    }									\
  }									\

/*=======================================================================
 * Macro      : logCommon
 * Author(s)  : Nicolas Roche
 * Date begin : 2012/05/01
 *     change : 2012/05/01
 * Description: Special log function for the common code
 * Synopsis   : void logCommon(level, format, ...)
 * Input      : wrapper for logEmit maccro
 * Output     : N/A
 =======================================================================*/
#define logCommon(priority, format, ...) {				\
    if (priority < LOG_INFO || env.debugCommon == TRUE) {		\
      logEmit(priority, format, __VA_ARGS__);				\
    }									\
  }									\

#endif /* MISC_LOG_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
