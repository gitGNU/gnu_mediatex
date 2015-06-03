/*=======================================================================
 * Version: $Id: command.h,v 1.3 2015/06/03 14:03:44 nroche Exp $
 * Project: MediaTeX
 * Module : command
 *
 * system API

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

#ifndef MISC_COMMAND_H
#define MISC_COMMAND_H 1

#include "alloc.h"
#include <getopt.h>

#define MISC_SHORT_OPTIONS "hvf:s:l:na:AS"
#define MISC_LONG_OPTIONS				\
  {"help", no_argument, 0, 'h'},			\
  {"version", no_argument, 0, 'v'},			\
  {"facility", required_argument, 0, 'f'},		\
  {"severity", required_argument, 0, 's'},		\
  {"log-file", required_argument, 0, 'l'},		\
  {"dry-run", no_argument, 0, 'n'},			\
  {"alloc-limit", required_argument, 0, 'a'},		\
  {"debug-alloc", no_argument, 0, 'A'},			\
  {"debug-script", required_argument, 0, 'S'}

#define MEMORY_SHORT_OPTIONS MISC_SHORT_OPTIONS "M"
#define MEMORY_LONG_OPTIONS				\
  MISC_LONG_OPTIONS,					\
  {"debug-memory", no_argument, 0, 'M'}			

#define PARSER_SHORT_OPTIONS MEMORY_SHORT_OPTIONS "LP"
#define PARSER_LONG_OPTIONS			        \
  MEMORY_LONG_OPTIONS,					\
  {"debug-lexer", no_argument, 0, 'L'},			\
  {"debug-parser", no_argument, 0, 'P'}

#define MDTX_SHORT_OPTIONS PARSER_SHORT_OPTIONS "c:C"
#define MDTX_LONG_OPTIONS				\
  PARSER_LONG_OPTIONS,					\
  {"conf-label", required_argument, 0, 'c'},		\
  {"debug-Common", no_argument, 0, 'C'}

void version();

void miscUsage(char* programName);
void memoryUsage(char* programName);
void parserUsage(char* programName);
void mdtxUsage(char* programName);

void miscOptions();
void memoryOptions();
void parserOptions();
void mdtxOptions();

int getMiscOptions(char cOption, char *optarg);
int getMemoryOptions(char cOption, char *optarg);
int getParserOptions(char cOption, char *optarg);
int getMdtxOptions(char cOption, char *optarg);

void getEnv(MdtxEnv* self);
int setEnv(char* programName, MdtxEnv *self);
int execScript(char** argv, char* user, char* pwd, int doHideStderr);

/*=======================================================================
 * Macro      : GET_MISC_OPTIONS
 * Description: read the misc commandline parameters
 * Synopsis   : GET_MISC_OPTIONS
 * Input      : N/A
 * Output     : N/A
 * Note       : EXIT_SUCCESS = 0
 *              EINVAL = 22
 *              ENOMEM = 12
 =======================================================================*/
#define GET_MISC_OPTIONS						\
  case 'h':								\
  usage(programName);							\
    rc = EXIT_SUCCESS;							\
    goto optError;							\
    break;								\
									\
  case 'v':								\
    version();								\
    rc = EXIT_SUCCESS;							\
    goto optError;							\
    break;								\
									\
  case 'f':								\
    if(optarg == (char*)0 || *optarg == (char)0) {			\
      fprintf(stderr,							\
	      "%s: nil or empty argument for the facility name\n",	\
	      programName);						\
      rc = EINVAL;							\
    }									\
    env.logFacility = optarg;						\
    break;								\
									\
  case 's':								\
    if(optarg == (char*)0 || *optarg == (char)0) {			\
      fprintf(stderr,							\
	      "%s: nil or empty argument for the severity name\n",	\
	      programName);						\
      rc = EINVAL;							\
    }									\
    env.logSeverity = optarg;						\
    break;								\
									\
  case 'l':								\
    if(optarg == (char*)0 || *optarg == (char)0) {			\
      fprintf(stderr,							\
	      "%s: nil or empty argument for the log stream\n",		\
	      programName);						\
      rc = EINVAL;							\
    }									\
    env.logFile = optarg;						\
    break;								\
									\
  case 'n':								\
    env.dryRun = 1;							\
    break;								\
									\
   case 'a':								\
    if(optarg == (char*)0 || *optarg == (char)0) {			\
      fprintf(stderr,							\
	      "%s: nil or empty argument for the nice limit\n",		\
	      programName);						\
      rc = EINVAL;							\
      rc = 1;								\
    }									\
    env.allocLimit = atoi(optarg);					\
    break;								\
									\
  case 'A':								\
    env.debugAlloc = 1;							\
    break;								\
									\
  case 'S':								\
    env.debugScript = 1;						\
    break;								\
									\
  default:								\
    rc = EINVAL;							\
    usage(programName);

/*=======================================================================
 * Macro      : GET_MEMORY_OPTIONS
 * Description: read the memory commandline parameters
 * Synopsis   : GET_MEMORY_OPTIONS
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
#define GET_MEMORY_OPTIONS 						\
  case 'M':								\
    env.debugMemory = 1;						\
    break;								\
									\
  GET_MISC_OPTIONS;

/*=======================================================================
 * Macro      : GET_PARSER_OPTIONS
 * Description: read the parser commandline parameters
 * Synopsis   : GET_PARSER_OPTIONS
 * Input      : N/A
 * Output     : N/A
 * Note       : you must build the scanner using %option debug 
 *              to include debugging information in it.
 =======================================================================*/
#define GET_PARSER_OPTIONS 						\
  case 'L':								\
    env.debugLexer = 1;							\
    break;								\
  									\
 case 'P':								\
   env.debugParser = 1;							\
   break;								\
    									\
 GET_MEMORY_OPTIONS;

/*=======================================================================
 * Macro      : GET_MDTX_OPTIONS
 * Description: read the mdtx commandline parameters
 * Synopsis   : GET_MDTX_OPTIONS
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
#define GET_MDTX_OPTIONS 						\
  case 'c':								\
    if(optarg == (char*)0 || *optarg == (char)0) {			\
      fprintf(stderr,							\
	      "%s: nil or empty argument for the conf label\n",		\
	      programName);						\
      rc = EINVAL;							\
      rc = 1;								\
    }									\
    env.confLabel = optarg;						\
    break;								\
									\
  case 'C':								\
    env.debugCommon = 1;						\
    break;								\
   									\
  GET_PARSER_OPTIONS;

/*=======================================================================
 * Macro      : ENDINGS
 * Description: stuff to do before exit
 * Synopsis   : ENDINGS
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
#define ENDINGS							\
  logEmit(LOG_INFO, "exit on %s", rc?"success":"error");	\
  if (env.debugAlloc) memoryStatus(LOG_NOTICE);			\
  exitMalloc();							\
  DefaultLog = logClose(DefaultLog)

#endif /* MISC_COMMAND_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
