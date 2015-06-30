/*=======================================================================
 * Version: $Id: command.c,v 1.5 2015/06/30 17:37:31 nroche Exp $
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
	  "\n" PACKAGE_STRING " Copyright (C) 2014 <Nicolas Roche>\n"
	  "\n<http://www.gnu.org/licenses/gpl.html>\n"
	  "This program comes with ABSOLUTELY NO WARRANTY\n"
	  "This is free software, and you are welcome to redistribute it\n"
	  "\nPlease report bugs to: <" PACKAGE_BUGREPORT ">.\n"
	  PACKAGE_NAME " home page: <http://www.nongnu.org/mediatex>.\n"
	  "General help using GNU software: "
	  "<http://www.gnu.org/gethelp/>\n\n");
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
  fprintf(stderr, "The usage for %s is:\n", basename(programName));
  fprintf(stderr, "%s\t{ -h | -v |\n", basename(programName));
  fprintf(stderr, "\t\t[ -f facility ] [ -s severity ] [ -l logFile ]");
  fprintf(stderr, "\n\t\t[-a memoryLimit ] [ -n ] [ -A ] [ -S ]");
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
  fprintf(stderr, "\n\t\t[ -M ]");
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
  fprintf(stderr, "\n\t\t[ -L ] [ -P ]");
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
  fprintf(stderr, "\n\t\t[ -O ] [ -c confFile ]");
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
	  " }\nwhere:\n"
	  "  -h, --help\t\tdisplay this message\n"
	  "  -v, --version\t\tdisplay the " PACKAGE_NAME 
	  " software version\n"
	  "  -f, --facility\tuse facility for logging\n"
	  "\t\t\tsee syslog(3) : mainly 'file' or 'local2' here\n"
	  "  -s, --severity\tuse severity for logging\n"
	  "\t\t\tamongs 'err' 'warning' 'notice' 'info' 'debug'\n"
	  "  -l, --log-file\tlog to logFile\n"
	  "\t\t\tneed to be coupled with the 'file' facility\n"
	  "  -a, --memory-limit\tlimit for malloc in Mo\n"
	  "  -n, --dry-run\t\tdo a dry run\n"
	  "  -A, --debug-alloc\tenable logs from malloc/free\n"
	  "  -S, --debug-script\tdo not hide outpouts from scripts\n");
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
  fprintf(stderr,
	  "  -M, --debug-memory\tenable logs from the memory objects\n");
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
	  "  -L, --debug-lexer\tdebug information from lexers\n"
	  "  -P, --debug-parser\tenable logs from the parsers\n");
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
	  "  -C, --debug-common\tenable logs from the common code\n"
	  "  -c, --conf-label\tto overhide default configuration file"
	  " mdtx.conf\n");
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

  // no logging available here
 
  // check the environment variables
  if ((tmpValue = getenv("MDTX_LOG_FILE")) != 0)
    self->logFile = tmpValue;
  if ((tmpValue = getenv("MDTX_LOG_FACILITY")) != 0)
    self->logFacility = tmpValue;
  if ((tmpValue = getenv("MDTX_LOG_SEVERITY")) != 0)
    self->logSeverity = tmpValue;
  if ((tmpValue = getenv("MDTX_MDTXUSER")) != 0)
    self->confLabel = tmpValue;
  if ((tmpValue = getenv("MDTX_DRY_RUN")) != 0)
    self->dryRun = !strncmp(tmpValue, "1", 2);
  if ((tmpValue = getenv("MDTX_NO_REGRESSION")) != 0)
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
  int logFacility = -1;
  int logSeverity = -1;
  LogHandler* logHandler = 0;

  // no logging available yet here

  // unset previous log handler if it exists (on bad malloc init)
  self->logHandler = logClose(self->logHandler);

  // set the log handler
  if ((logFacility = getLogFacility(self->logFacility)) == -1) {
    fprintf(stderr, "%s: incorrect facility name '%s'\n", 
	    programName, optarg); goto error;
  }
  if ((logSeverity = getLogSeverity(self->logSeverity)) == -1) {
    fprintf(stderr, "%s: incorrect severity name '%s'\n", 
	    programName, optarg); goto error;
  }
  if(!(self->logHandler = 
       logOpen(programName, logFacility, logSeverity, self->logFile))) {
    fprintf(stderr, "%s: cannot allocate the logHandler\n", programName);
    goto error;
  }

  logEmit(LOG_DEBUG, "%s", "set the environment variables");

  // export the environment
  if (setenv("MDTX_LOG_FILE", self->logFile, 1) == -1
      || setenv("MDTX_LOG_FACILITY", self->logFacility, 1) == -1
      || setenv("MDTX_LOG_SEVERITY", self->logSeverity, 1) == -1
      || setenv("MDTX_MDTXUSER", self->confLabel, 1) == -1
      || setenv("MDTX_DRY_RUN", self->dryRun?"1":"0", 1) == -1
      || setenv("LD_LIBRARY_PATH", "", 1) == -1
      || setenv("IFS", "", 1) == -1
      ) {
    logEmit(LOG_ERR, "setenv variables failed: ", strerror(errno));
    goto error2;
  }

  // Because of "make distcheck", scripts use $srcdir to source each
  // others. Here, we reuse this automake variable so as scripts will
  // still be able to source each others after installation.
  if (setenv("srcdir", CONF_SCRIPTS , 1) == -1)
    goto error2;

  // custumize malloc, record a callback to free memory if malloc fails
  if (!initMalloc(self->allocLimit*MEGA, self->allocDiseaseCallBack))
    goto error;

  rc = TRUE;
 error2:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to set the environment variables");
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
 =======================================================================*/
void 
execChild(char** argv, int doHideStderr)
{ 
  int rc = 0;
  int fd;

  if (argv[0][0] != '/') {
    logEmit(LOG_ERR, "%s", "refuse to exec from a relative path");
    goto error;
  }

  // be quiet: close stdout
  if (!env.debugScript) {
    if ((fd = open("/dev/null", O_WRONLY)) != -1) {
      dup2(fd, 1);
      if (doHideStderr) dup2(fd, 2); // close stderr
      close(fd);
    }
  }

  rc = execve(argv[0], argv, environ);
 error:
  if (rc == -1) {
    logEmit(LOG_ERR, "execve failed: ", strerror(errno));
  } else {
    logEmit(LOG_ERR, "%s", "execve returns: you should improve ram");
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
    logEmit(LOG_ERR, "%s", "please provide a script to exec");
    goto error;
  }

  logEmit(LOG_INFO, "execScript%s%s%s%s: %s", 
	  user?" as ":"", user?user:"", 
	  pwd?" PWD=":"", pwd?pwd:"",
	  argv[0]);

  while (argv[++i] != 0 && *argv[i] != (char)0) {
    logEmit(LOG_INFO, " arg%i: %s", i, argv[i]);
  }

  // fork: new process
  if ((childId = fork()) == -1) {
      logEmit(LOG_ERR, "fork failed: ", strerror(errno));
      goto error;
    }
  
  switch(childId) {
  case 0:
    // child

    // change user
    if (user && !becomeUser(user, FALSE)) goto error;

    // change working directory
    if (pwd && chdir(pwd)) {
      logEmit(LOG_ERR, "chdir fails: %s", strerror(errno));
      goto error;
    }

    execChild(argv, doHideStderr);
    logEmit(LOG_ERR, "failed to exec %s", argv[0]);
    exit(EXIT_FAILURE);
    break;
 
  default:
   // father
    if ((pid = waitpid(childId, &status, 0)) == -1) {
      logEmit(LOG_ERR, "wait failed (%i): %s", errno, strerror(errno));
      goto error;
    }
    if (pid != childId) {
      logEmit(LOG_ERR, 
	      "wait exit with unexpected process %i (waiting for %i)", 
	      rc, childId);
      goto error;
    }
  }

  if (WIFEXITED(status)) rcChild = WEXITSTATUS(status);
  logEmit(LOG_INFO, "%s execution returns %i", argv[0], rcChild);
  if (rcChild == -1) {
    logEmit(LOG_INFO, "%s", "if -1 you may try to improve ram");
  }
  rc = (rcChild == EXIT_SUCCESS);
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "execScript fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

