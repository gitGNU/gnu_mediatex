/*=======================================================================
 * Project: MediaTeX
 * Module : shm
 *
 * Test for share memory manager

 * Note: it should be nicer to have an initialize function ?
 * Note: IPC système V need to be compilated into the linux kernel
 if not, error ENOSYS is raised

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

#define SHM_BUFF_SIZE 4

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : N/A
 =======================================================================*/
void 
exempleForRead(void *buffer, int shmSize, void* arg) {
  (void) arg;
  (void) shmSize;
  fprintf(stdout, "-> %s\n", (char*)buffer);
}

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : N/A
 =======================================================================*/
void 
exempleForWrite(void *buffer, int shmSize, void *arg) {
  (void) arg;
  // show actual content
  fprintf(stdout, "-> %s\n", (char*)buffer);
  // Modify share memory content
  fprintf(stdout, "<- ");
  fgets((char*)buffer, shmSize+1, stdin);
  fprintf(stdout, "%s\n", (char*)buffer);
}

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
  fprintf(stderr, "\n\t\t{ -R | -W | -C }");

  miscOptions();
  fprintf(stderr, "  ---\n"
	  "  -r, --read\t\tread the share memory\n"
	  "  -w, --write\t\twrite the share memory\n"
	  "  -c, --clean\t\tclean the share memory\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * Description: Unit test for shm module.
 * Synopsis   : utshm
 * Input      : -R | -W | -C 
 * Output     : N/A
 =======================================================================*/
int main(int argc, char** argv)
{
  typedef enum {UNDEF, READER, WRITER, CLEANER} Role;
  Role role = UNDEF;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"rwc";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"read", no_argument, 0, 'r'},
    {"write", no_argument, 0, 'w'},
    {"clean", no_argument, 0, 'c'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {

    case 'r':
      if (role != UNDEF) {
	usage(programName);
	goto error;
      }
      role = READER;
      break;

    case 'w':
      if (role != UNDEF) {
	usage(programName);
	goto error;
      }
      role = WRITER;
      break;

    case 'c':
      if (role != UNDEF) {
	usage(programName);
	goto error;
      }
      role = CLEANER;
      break;

      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }
  
  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;
  
  /************************************************************************/
  switch (role) {
  case WRITER:
    rc = shmWrite(argv[0], SHM_BUFF_SIZE, exempleForWrite, 0);
    break;
  case READER:
    rc = shmRead(argv[0], SHM_BUFF_SIZE, exempleForRead, 0);
    break;
  case CLEANER:
    rc = shmFree(argv[0], SHM_BUFF_SIZE);
    break;
  default:
    usage(programName);
  }
  /************************************************************************/

 error:
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
