/*=======================================================================
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

typedef enum {
   LOG_ALLOC,
   LOG_SCRIPT,
   LOG_MISC,
   LOG_MEMORY,
   LOG_PARSER,
   LOG_COMMON,
   LOG_MAIN,
   LOG_MAX_MODULE
} LogModule;

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
  LogSeverity* severity[LOG_MAX_MODULE];
  FILE* hlog;
} LogHandler;

int parseLogSeverityOption(char* parameter, int* logSeverity);
LogHandler* logOpen(char* name, int facility, int* logSeverity,
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
    char* ptr = file + strlen(file);					\
    while (ptr > (char*)file && *(ptr-1) != '/') --ptr;			\
    logEmitFunc(log, priority, "[%s %s:%i] " format,			\
		LogSeverities[priority].name, ptr, line,		\
		## __VA_ARGS__);					\
  }
#else
#define logEmitMacro(log, priority, file, line, format, ...) {		\
    char* ptr = file + strlen(file);					\
    while (ptr > (char*)file && *(ptr-1) != '/') --ptr;			\
    logEmitFunc(log, priority, "[%s %s] " format,			\
		LogSeverities[priority].name, ptr,			\
		## __VA_ARGS__);					\
  }
#endif

/*=======================================================================
 * Macro      : logEmit
 * Description: Add file and line to the log (so as to debug more easily)
 * Synopsis   : void logParser(level, format, ...)
 * Input      : Wrapper for logEmitFunc function
 * Output     : N/A
 =======================================================================*/
#define logEmit(module, priority, format, ...)	{			\
    if (priority <= env.logHandler->severity[module]->code) {		\
      logEmitMacro(env.logHandler, priority, __FILE__, __LINE__,	\
		    format, ## __VA_ARGS__);				\
    }									\
  }

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
#define logAlloc(priority, file, line, format, ...) {			\
    if (priority <= env.logHandler->severity[LOG_ALLOC]->code) {	\
      logEmitMacro(env.logHandler, priority, file, line,		\
		    format, ## __VA_ARGS__);				\
    }									\
  }

/*=======================================================================
 * Macro      : logScript
 * Author(s)  : Nicolas Roche
 * Date begin : 2012/05/01
 *     change : 2012/05/01
 * Description: Special log function for the scripts
 * Synopsis   : void logScript(level, format, ...)
 * Input      : wrapper for logEmit maccro
 * Output     : N/A
 =======================================================================*/
#define logScript(priority, format, ...) {			\
    logEmit(LOG_SCRIPT, priority, format, ## __VA_ARGS__);	\
  }

/*=======================================================================
 * Macro      : logMisc
 * Author(s)  : Nicolas Roche
 * Date begin : 2012/05/01
 *     change : 2012/05/01
 * Description: Special log function for the misc modules
 * Synopsis   : void logMisc(level, format, ...)
 * Input      : wrapper for logEmit maccro
 * Output     : N/A
 =======================================================================*/
#define logMisc(priority, format, ...) {			\
    logEmit(LOG_MISC, priority, format, ## __VA_ARGS__);	\
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
    logEmit(LOG_MEMORY, priority, format, ## __VA_ARGS__);	\
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
    logEmit(LOG_PARSER, priority, format, ## __VA_ARGS__);	\
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
    logEmit(LOG_COMMON, priority, format, ## __VA_ARGS__);	\
  }

/*=======================================================================
 * Macro      : logMain
 * Author(s)  : Nicolas Roche
 * Date begin : 2012/05/01
 *     change : 2012/05/01
 * Description: Special log function for the common code
 * Synopsis   : void logCommon(level, format, ...)
 * Input      : wrapper for logEmit maccro
 * Output     : N/A
 =======================================================================*/
#define logMain(priority, format, ...) {			\
    logEmit(LOG_MAIN, priority, format, ## __VA_ARGS__);	\
  }

#endif /* MDTX_MISC_LOG_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
