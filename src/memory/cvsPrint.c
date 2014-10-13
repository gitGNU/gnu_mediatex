/*=======================================================================
 * Version: $Id: cvsPrint.c,v 1.1 2014/10/13 19:39:09 nroche Exp $
 * Project: MediaTeX
 * Module : md5sumTree
 *
 * cvs files producer interface

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Nicolas Roche
 
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

#include "../mediatex.h"
#include "../misc/log.h"
#include "../misc/locks.h"
#include "strdsm.h"
#include "cvsPrint.h"

#ifdef utMAIN
#undef MEMORY_CVSPRINT_MAX
#define MEMORY_CVSPRINT_MAX 10
#endif

/*=======================================================================
 * Function   : cvsCloseFile
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : TRUE on success
 =======================================================================*/
int cvsCloseFile(CvsFile* fd)
{
  int rc = FALSE;

  if (!fd) goto error;
  logEmit(LOG_DEBUG, "cvsCloseFile %s", fd->path);
  if (!(fd->fd)) goto end;

  fprintf(fd->fd, "\n# Local Variables:\n"
	  "# mode: conf\n"
	  "# mode: font-lock\n"
	  "# End:\n");

  if (fd->fd == stdout) {
    fflush(stdout);
    goto end;
  }

  if (!unLock(fileno(fd->fd))) rc = FALSE;
  if (fclose(fd->fd)) {
    logEmit(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error;
  }

  fd->fd = NULL;
 end:
  rc = TRUE;
error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "cvsCloseFile fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : cvsOpenFile
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : TRUE on success
 =======================================================================*/
int cvsOpenFile(CvsFile* fd)
{
  int rc = FALSE;
  char* path = NULL;
  int l = 0;
  int i = 0;

  if (!fd) goto error;
  if (isEmptyString(fd->path)) goto error; 
  logEmit(LOG_DEBUG, "cvsOpenFile %s %i", fd->path, fd->nb);

  l = strlen(fd->path);
  if (!(path = createString(fd->path))
      || !(path = catString(path, "00.txt"))) goto error;

  // first call:  unlink existing files
  if (fd->nb == 0 && fd->fd != stdout) {
    // empty part files
    do {
      if (!sprintf(path+l, "%02i.txt", i)) goto error;
      if (access(path, R_OK) != 0) break;
      if (!env.dryRun) {
	logEmit(LOG_INFO, "empty %s", path);
	if ((fd->fd = fopen(path, "w")) == NULL) {
	  logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno));
	  goto error;
	}
	if (fclose(fd->fd)) {
	  logEmit(LOG_ERR, "fclose fails: %s", strerror(errno));
	  goto error;
	}
	fd->fd = NULL;
      }
    }
    while (++i < 100);
    
    // unlink last addon
    if (!sprintf(path+l, "%s", "NN.txt")) goto error;
    if (access(path, R_OK) == 0) {
      logEmit(LOG_INFO, "unlink %s", path);
      if (unlink(path) == -1) {
	logEmit(LOG_ERR, "unlink fails %s:", strerror(errno));
	goto error;
      }
    }
  }

  // not first call
  if (fd->nb > 0) {
    if (!cvsCloseFile(fd)) goto error;
  }

  // open file
  if (!sprintf(path+l, "%02i.txt", fd->nb)) goto error;
  logEmit(LOG_INFO, "serialize into %s", path);
  if (fd->fd == stdout) goto end;
  if ((fd->fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno));
    goto error;
  }

  if (!lock(fileno(fd->fd), F_WRLCK)) goto error;
 end:
  ++fd->nb;
  fd->offset = 0;
  rc = TRUE;
error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "cvsOpenFile fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : cvsPrint
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : TRUE on success
 =======================================================================*/
int cvsPrint(CvsFile* fd, const char* format, ...)
{
  int rc = FALSE;
  va_list args;

  if (!fd) goto error;

  if (fd->doCut && fd->offset > MEMORY_CVSPRINT_MAX) {
    if (!cvsOpenFile(fd)) goto error;
  }

  va_start(args, format);
  fd->offset += vfprintf(fd->fd, format, args);
  va_end(args);
  rc = TRUE;
error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "cvsPrint fails");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
GLOBAL_STRUCT_DEF;

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
  memoryUsage(programName);

  memoryOptions();
  //fprintf(stderr, "\t\t---\n");
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for confTree module.
 * Synopsis   : utconfTree
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  CvsFile fd = {"filename", 0, stdout, FALSE, 0};
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MEMORY_SHORT_OPTIONS;
  struct option longOptions[] = {
    MEMORY_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MEMORY_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!cvsOpenFile(&fd)) goto error;
  fd.doCut = FALSE;
  if (!cvsPrint(&fd, "%s", "the lines should not be cut\n")) goto error;
  if (!cvsPrint(&fd, "%s", "put 2nd line in same file\n")) goto error;
  fd.doCut = TRUE;
  if (!cvsPrint(&fd, "%s", "3rd line ...\n")) goto error;
  if (!cvsPrint(&fd, "%s", "4rth line ...\n")) goto error;
  if (!cvsCloseFile(&fd)) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

