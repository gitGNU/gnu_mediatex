/*=======================================================================
 * Version: $Id: log.h,v 1.4 2015/06/30 17:37:33 nroche Exp $
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

#ifndef MDTX_MISC_LOG_H
#define MDTX_MISC_LOG_H 1

#include "../mediatex-types.h"

#include <syslog.h>
#include <stdarg.h>

typedef struct LogFacility {
  int code;
  char* name;
} LogFacility;

int getLogFacility(char* name);
LogFacility* getLogFacilityByName(char* name);
LogFacility* getLogFacilityByCode(int code);

typedef struct LogSeverity {
  int code;
  char* name;
} LogSeverity;

int getLogSeverity(char* name);
LogSeverity* getLogSeverityByName(char* name);
LogSeverity* getLogSeverityByCode(int code);

typedef struct LogHandler {
  char* name;
  LogFacility* facility;
  LogSeverity* severity;
  FILE* hlog;
} LogHandler;

LogHandler* logOpen(char* name, int facility, int severity, 
		    char* logFile);

void logEmitFunc(LogHandler* logHandler, int priority,
		 const char* format, ...);

LogHandler* logClose(LogHandler* logHandler);

extern LogSeverity LogSeverities[];

/*=======================================================================
 * Macro      : logEmitMacro
 * Description: Call logEmitFunc without troubles about MISC_LOG_LINES
 * Synopsis   : void logParserMacro(level, format, ...)
 * Input      : Wrapper for logEmitFunc function
 * Output     : N/A
 * Note       : path are troncated so as to make compatible "make check" 
 *              and "make distcheck" output
 *              (https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html)
 =======================================================================*/
#if MISC_LOG_LINES    
#define logEmitMacro(log, priority, file, line, format, ...) {		\
    if (priority <= log->severity->code) {				\
      char* ptr = file + strlen(file);					\
      while (ptr > file && *(ptr-1) != '/') --ptr;			\
      logEmitFunc(log, priority, "[%s %s:%i] " format,			\
		  LogSeverities[priority].name, ptr, line,		\
		  ## __VA_ARGS__);					\
    }									\
  }
#else
#define logEmitMacro(log, priority, file, line, format, ...) {		\
    if (priority <= log->severity->code) {				\
      char* ptr = file + strlen(file);					\
      while (ptr > file && *(ptr-1) != '/') --ptr;			\
      logEmitFunc(log, priority, "[%s %s] " format,			\
		  LogSeverities[priority].name, ptr,			\
		  ## __VA_ARGS__);					\
    }									\
  }
#endif

/*=======================================================================
 * Macro      : logEmit
 * Description: Add file and line to the log (so as to debug more easily)
 * Synopsis   : void logParser(level, format, ...)
 * Input      : Wrapper for logEmitFunc function
 * Output     : N/A
 =======================================================================*/
#define logEmit(priority, format, ...)					\
  logEmitMacro(env.logHandler, priority, __FILE__, __LINE__,		\
	       format, ## __VA_ARGS__);

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
#define logAlloc(priority, file, line, format, ...) {		\
    if (priority < LOG_NOTICE || env.debugAlloc) {		\
      logEmitMacro(env.logHandler, priority, file, line,	\
		   format, ## __VA_ARGS__);			\
    }								\
  }

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
#define logMemory(priority, format, ...) {			\
    if (priority < LOG_NOTICE || env.debugMemory) {		\
      logEmit(priority, format, ## __VA_ARGS__);		\
    }								\
  }
    
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
#define logParser(priority, format, ...) {			\
    if (priority < LOG_NOTICE || env.debugParser) {		\
      logEmit(priority, format, ## __VA_ARGS__);		\
    }								\
  }

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
#define logCommon(priority, format, ...) {			\
    if (priority < LOG_NOTICE || env.debugCommon) {		\
      logEmit(priority, format, ## __VA_ARGS__);		\
    }								\
  }

#endif /* MDTX_MISC_LOG_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
