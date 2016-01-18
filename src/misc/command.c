/*=======================================================================
 * Version: $Id: command.c,v 1.18 2015/10/08 21:43:50 nroche Exp $
 * Project: MediaTeX
 * Module : command
 *
 * system API

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
#include <libgen.h>    // basename
#include <sys/wait.h>  // waitpid

extern char **environ;

/*=======================================================================
 * Function   : version
 * Description: Print the software version.
 * Synopsis   : void version()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
void 
version()
{
  fprintf(stderr, 
	  PACKAGE_STRING "\n"
	  "\nCopyright (C) 2014 2015 2016 Nicolas Roche.\n"
	  "License GPLv3+: GNU GPL version 3 or later"
	  " <http://www.gnu.org/licenses/gpl.html>\n"
	  "This is free software, and you are welcome to redistribute it.\n"
	  "There is NO WARRANTY, to the extent permitted by law.\n");
}

/*=======================================================================
 * Function   : help
 * Description: Print ending help
 * Synopsis   : void help()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
void 
mdtxHelp()
{
  fprintf(stderr,
	  "\nReport bugs to: <" PACKAGE_BUGREPORT ">.\n"
	  PACKAGE_NAME " home page: <http://www.nongnu.org/mediatex>\n"
	  "General help using GNU software: "
	  "<http://www.gnu.org/gethelp/>\n");
}

/*=======================================================================
 * Function   : miscUsage
 * Description: Print the usage header for misc unit tests
 * Synopsis   : void miscUsage(char* programName)
 * Input      : programName = the name of the program; usually argv[0]
 * Output     : N/A
 =======================================================================*/
void 
miscUsage(char* programName)
{
  fprintf(stderr, "\nUsage: %s", basename(programName));
  fprintf(stderr, 
	  "-h | -v |"
	  " [ -f facility ] [ -l logFile ]"
	  " [ -s severity[:module(,module)*] ]"
	  " [ -m memoryLimit ] [ -n ]");
}

/*=======================================================================
 * Function   : memoryUsage
 * Description: Print the usage header for memory unit tests
 * Synopsis   : void memoryUsage(char* programName)
 * Input      : programName = the name of the program; usually argv[0]
 * Output     : N/A
 =======================================================================*/
void 
memoryUsage(char* programName)
{
  miscUsage(programName);
}

/*=======================================================================
 * Function   : parserUsage
 * Description: Print the usage header for parser unit tests
 * Synopsis   : void parserUsage(char* programName)
 * Input      : programName = the name of the program; usually argv[0]
 * Output     : N/A
 =======================================================================*/
void 
parserUsage(char* programName)
{
  memoryUsage(programName);
  fprintf(stderr, " [ -L ]");
}

/*=======================================================================
 * Function   : mdtxUsage
 * Description: Print the usage header for mdtx unit tests
 * Synopsis   : void mdtxUsage(char* programName)
 * Input      : programName = the name of the program; usually argv[0]
 * Output     : N/A
 =======================================================================*/
void 
mdtxUsage(char* programName)
{
  parserUsage(programName);
  fprintf(stderr, " [ -c confFile ]");
}

/*=======================================================================
 * Function   : miscOptions
 * Description: Print the misc usage options.
 * Synopsis   : void miscOptions()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
void 
miscOptions()
{
  fprintf(stderr, 
	  "\n\nOptions:\n"
	  "  -h, --help\t\tdisplay this message\n"
	  "  -v, --version\t\tdisplay the " PACKAGE_NAME 
	  " software version\n"
	  "  -f, --facility\tset facility to use for logging\n"
	  "\t\t\tsee syslog(3) : mainly 'file' or 'local2' here\n"
	  "  -l, --log-file\tlog into a file\n"
	  "\t\t\tneeds 'file' facility (default)\n"
	  "  -s, --severity\tset severity to log, by modules.\n"
	  "\t\t\tamongs 'err', 'warning', 'notice', 'info', 'debug';\n"
	  "\t\t\tmodules amongs 'alloc', 'script', 'misc', 'memory', "
	  "'parser', 'common', 'main'\n"
	  "  -n, --dry-run\t\tdo a dry run\n"
	  "  -m, --memory-limit\tnice limit for malloc in Mo\n");
}

/*=======================================================================
 * Function   : memoryOptions
 * Description: Print the memory usage options.
 * Synopsis   : void memoryOptions()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
void 
memoryOptions()
{
  miscOptions();
}

/*=======================================================================
 * Function   : parserOptions
 * Description: Print the parser usage options.
 * Synopsis   : void parserOptions()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
void 
parserOptions()
{
  memoryOptions();
  fprintf(stderr, 
	  "  -L, --debug-lexer\tdebug information from lexers\n");
}

/*=======================================================================
 * Function   : mdtxOptions
 * Description: Print the mdtx usage options.
 * Synopsis   : void mdtxOptions()
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
void 
mdtxOptions()
{
  parserOptions();
  fprintf(stderr, 
	  "  -c, --conf-label\toverhide 'mdtx.conf' configuration file\n");
}

/*=======================================================================
 * Function   : getEnv
 * Description: void getEnv(MdtxEnv* self)
 * Synopsis   : import the MDTX environment variables
 * Input      : self = the MDTX environment variables
 * Output     : N/A
 =======================================================================*/
void 
getEnv(MdtxEnv* self)
{
  char* tmpValue = 0;
  int i =0;

  static char logModuleVar[7][32] = {
    "MDTX_LOG_SEVERITY_ALLOC",
    "MDTX_LOG_SEVERITY_SCRIPT",
    "MDTX_LOG_SEVERITY_MISC",
    "MDTX_LOG_SEVERITY_MEMORY",
    "MDTX_LOG_SEVERITY_PARSER",
    "MDTX_LOG_SEVERITY_COMMON",
    "MDTX_LOG_SEVERITY_MAIN"
  };

  // no logging available here
 
  // check the environment variables
  if ((tmpValue = getenv("MDTX_LOG_FACILITY")))
    self->logFacility = getLogFacility(tmpValue);
  if ((tmpValue = getenv("MDTX_LOG_FILE")))
    self->logFile = tmpValue;

  for (i=0; i<LOG_MAX_MODULE; ++i) {
    if ((tmpValue = getenv(logModuleVar[i])))
      self->logSeverity[i] = getLogSeverity(tmpValue);
  }

  if ((tmpValue = getenv("MDTX_MDTXUSER")))
    self->confLabel = tmpValue;
  if ((tmpValue = getenv("MDTX_DRY_RUN")))
    self->dryRun = !strncmp(tmpValue, "1", 2);
  if ((tmpValue = getenv("MDTX_NO_REGRESSION")))
    self->noRegression = !strncmp(tmpValue, "1", 2);
}

/*=======================================================================
 * Function   : setEnv
 * Description: int setEnv(char* programName, MdtxEnv *self)
 * Synopsis   : enable the logs, export the MDTX environment variables
 *              and initialize custumized malloc
 * Input      : programName = the name of the program; usually argv[0] 
 *              self = the MDTX environment variables
 * Output     : TRUE on success
 =======================================================================*/
int 
setEnv(char* programName, MdtxEnv *self)
{
  int rc = FALSE;
  int ko = FALSE;
  int i = 0;

  static char logModuleVar[7][32] = {
    "MDTX_LOG_SEVERITY_ALLOC",
    "MDTX_LOG_SEVERITY_SCRIPT",
    "MDTX_LOG_SEVERITY_MISC",
    "MDTX_LOG_SEVERITY_MEMORY",    "MDTX_LOG_SEVERITY_PARSER",
    "MDTX_LOG_SEVERITY_COMMON",
    "MDTX_LOG_SEVERITY_MAIN"
  };

  // no logging available yet here

  // unset previous log handler if it exists (on bad malloc init)
  self->logHandler = logClose(self->logHandler);

  // set the log handler
  if(!(self->logHandler = 
       logOpen(programName, self->logFacility, self->logSeverity, 
	       self->logFile))) {
    fprintf(stderr, "%s: cannot allocate the logHandler\n", programName);
    goto error;
  }

  logMisc(LOG_DEBUG, "set the environment variables");

  // export the environment (used by executed binaries or scripts)
  mdtxSetEnv("MDTX_LOG_FACILITY", self->logHandler->facility->name);
  mdtxSetEnv("MDTX_LOG_FILE", self->logFile?self->logFile:"");

  for (i=0; !ko && i<LOG_MAX_MODULE; ++i) {
    mdtxSetEnv(logModuleVar[i], self->logHandler->severity[i]->name);
  }

  mdtxSetEnv("MDTX_MDTXUSER", self->confLabel);
  mdtxSetEnv("MDTX_DRY_RUN", self->dryRun?"1":"0");
  mdtxSetEnv("LD_LIBRARY_PATH", "");
  mdtxSetEnv("IFS", "");

  // Because of "make distcheck", scripts use $srcdir to source each
  // others. Here, we reuse this automake variable so as scripts will
  // still be able to source each others after installation.
  mdtxSetEnv("srcdir", CONF_SCRIPTS);

  // customize malloc, record a callback to free memory if malloc fails
  if (!initMalloc(self->allocLimit*MEGA, self->allocDiseaseCallBack))
    goto error2;

  rc = TRUE;
 error2:
  if (!rc) {
    logMisc(LOG_ERR, "fails to set the environment");
  }
 error:
  return rc;
}

/*=======================================================================
 * Function   : execChild
 * Description: exec call providing the MDTX environment variables
 * Synopsis   : void execChild(char** argv)
 * Input      : argv = the command line to run
 *              doHideStderr = close stderr before exec
 * Output     : Never returns on success
 * Note       : fork keeps file descriptors
 =======================================================================*/
void 
execChild(char** argv, int doHideStderr)
{ 
  int rc = 0;
  int fd;

  logMisc(LOG_DEBUG, "execChild: %s (do%s hide stderr)",
	  argv[0], doHideStderr?"":"not");

  if (argv[0][0] != '/') {
    logMisc(LOG_ERR, "refuse to exec from a relative path");
    goto error;
  }
  
  // be quiet
  if ((fd = open("/dev/null", O_WRONLY)) == -1) {
    logMisc(LOG_ERR, "open fails on /dev/null: %s", strerror(errno));
    goto error;
  }

  // - close stdout by default
  if (env.logHandler->severity[LOG_SCRIPT]->code <= LOG_INFO) {
    logMisc(LOG_INFO, "close stdout from child (use '-sdebug:script' to undo)");
    dup2(fd, 1);
  }

  // - close stderr only when we want to hide stdout-like informations
  if (doHideStderr &&
      env.logHandler->severity[LOG_SCRIPT]->code <= LOG_NOTICE) {
    logMisc(LOG_INFO, "exceptionally close stderr from child "
	    "(use '-sinfo:script' to undo)");
    dup2(fd, 2);
  }
      
  close(fd);
  rc = execve(argv[0], argv, environ);
 error:
  if (rc == -1) {
    logMisc(LOG_ERR, "execve failed: ", strerror(errno));
  } else {
    logMisc(LOG_ERR, "execve returns: you should improve ram");
  }
}

/*=======================================================================
 * Function   : execScript
 * Description: system call providing the MDTX environment variables
 * Synopsis   : int execScript(char** argv, char* user, char* pwd)
 * Input      : argv = the command line to run
 *              user = user to become to before exec 
 *              pwd = directory to change to before exec
 *              doHideStderr = close stderr before exec
 * Output     : TRUE on success
 =======================================================================*/
int 
execScript(char** argv, char* user, char* pwd, int doHideStderr)
{
  int rc = FALSE;
  int rcChild = -1;
  pid_t childId = 0;
  pid_t pid = 0;
  int status = 0;
  int i = 0;
  
  if (argv[0] == 0 || *argv[0] == (char)0) {
    logMisc(LOG_ERR, "please provide a script to exec");
    goto error;
  }

  logMisc(LOG_INFO, "execScript%s%s%s%s%s: %s", 
	  user?" as ":"", user?user:"", 
	  pwd?" PWD=":"", pwd?pwd:"",
	  doHideStderr?" (no-stderr)":"",
	  argv[0]);
  
  while (argv[++i] && *argv[i] != (char)0) {
    logMisc(LOG_INFO, " arg%i: %s", i, argv[i]);
  }
  
  // fork: new process
  if ((childId = fork()) == -1) {
      logMisc(LOG_ERR, "fork failed: ", strerror(errno));
      goto error;
    }
  
  switch(childId) {
  case 0:
    // child

    // change user
    if (user && !becomeUser(user, FALSE)) goto error;

    // change working directory
    if (pwd && chdir(pwd)) {
      logMisc(LOG_ERR, "chdir fails: %s", strerror(errno));
      goto error;
    }

    logMisc(LOG_INFO, "--- exec system call begin ---");
    execChild(argv, doHideStderr);
    logMisc(LOG_ERR, "failed to exec %s", argv[0]);
    exit(EXIT_FAILURE);
    break;
 
  default:
   // father
    if ((pid = waitpid(childId, &status, 0)) == -1) {
      logMisc(LOG_ERR, "wait failed (%i): %s", errno, strerror(errno));
      goto error;
    }
    if (pid != childId) {
      logMisc(LOG_ERR, 
	      "wait exit with unexpected process %i (waiting for %i)", 
	      rc, childId);
      goto error;
    }
  }

  logMisc(LOG_INFO, "--- exec system call end ---");
  if (WIFEXITED(status)) rcChild = WEXITSTATUS(status);
  logMisc(LOG_INFO, "%s execution returns %i", argv[0], rcChild);
  if (rcChild == -1) {
    logMisc(LOG_INFO, "if -1 you may try to improve ram");
  }
  rc = (rcChild == EXIT_SUCCESS);
 error:
  if (!rc) {
    logMisc(LOG_ERR, "execScript fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

