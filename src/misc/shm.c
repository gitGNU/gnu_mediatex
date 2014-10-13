/*=======================================================================
 * Version: $Id: shm.c,v 1.1 2014/10/13 19:39:34 nroche Exp $
 * Project: MediaTeX
 * Module : shm
 *
 * Share memory manager

 * Note: it should be nicer to have an initialize function ?
 * Note: IPC système V need to be compilated into the linux kernel
 if not, error ENOSYS is raised

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
#include "log.h"
#include "shm.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

typedef union semun {
  int val;
  struct semid_ds *buffer;
  unsigned short int *table;
} semun_t;

/*=======================================================================
 * Function   : shmWrite
 * Description: Allocate shm and perform callback function on it.
 * Synopsis   : int shmWrite(char* pathFile, int shmSize,
 *                       void (*callback)(void*, int, void*), void* arg)
 * Input      : char* filePath: needed to create IPC key
 *              int shmSize: the size of the shm
 *              void (*callback)(void*, int, void*): callback function
 *              void* arg: argument to pass to the callback function
 * Output     : TRUE on success
 =======================================================================*/
int 
shmWrite(char* pathFile, int shmSize, 
	 void (*callback)(void*, int, void*), void* arg)
{
  int rc = FALSE;
  key_t key;
  int sem;
  int shm;
  struct sembuf sembuf;
  semun_t u_semun;
  unsigned short table[1];
  void* buffer = NULL;

  logEmit(LOG_DEBUG, "%s", "shmWrite");

  if (pathFile == NULL || *pathFile == (char)0) {
    logEmit(LOG_ERR, "%s", "need a path file's path to create shm key"); 
    goto error;
  }
  
  // Create a new IPC key base on an i-node + minor peripherical
  // number and a project number
  if ((key = ftok(pathFile, MISC_SHM_PROJECT_ID)) == -1) {
    logEmit(LOG_ERR, "ftok fails to create IPC key: %s", strerror(errno)); 
    logEmit(LOG_ERR, "ftok using path: %s", pathFile); 
    goto error;
  }

  // Create a new share memory segmment matched with the key 
  // or use the pre-existing one
  if ((shm = shmget(key, shmSize, IPC_CREAT | 0600)) == -1) {
    logEmit(LOG_ERR, "semget fails to create share memory: %s", 
	    strerror(errno)); 
    goto error;
  }

  // Attach the share memory segment to the space memory of the processus
  if ((buffer = shmat(shm, NULL, 0)) == NULL) {
    logEmit(LOG_ERR, "shmat fails to attach share memory: %s", 
	    strerror(errno)); 
    goto error;
  }

  // Try to match an existing semaphore with the key
  if ((sem = semget(key, 1, 0)) == -1) {

    // Create a new semaphore matched with the key
    if ((sem = semget(key, 1, IPC_CREAT | IPC_EXCL | 0600)) == -1) {
      logEmit(LOG_ERR, "semget fails to create new semaphore: %s", 
	      strerror(errno)); 
      goto error;
    }
  
    // Initialize content of share memory
    memset(buffer, 0, shmSize);

    // Initialize the semaphore(s) values
    table[0] = 1;
    u_semun.table = table;
    if (semctl(sem, 0, SETALL, u_semun) <0) {
      logEmit(LOG_ERR, "semctl fails to initialize semaphore value: %s", 
	      strerror(errno)); 
      goto error;
    }
  }

  // P (may I ?)
  sembuf.sem_num = 0;
  sembuf.sem_op = -1;
  sembuf.sem_flg = SEM_UNDO; // for crash recovery
  if (semop(sem, &sembuf, 1) < 0) {
    logEmit(LOG_ERR, "semop fails asking for semaphore: %s", 
	    strerror(errno)); 
    goto error;
  }

  // Modify share memory content
  callback(buffer, shmSize, arg);

  // V (Please)
  sembuf.sem_op = 1;
  if (semop(sem, &sembuf, 1) < 0) {
    logEmit(LOG_ERR, "semop fails returning back semaphore: %s", 
	    strerror(errno)); 
    goto error;
  }

  // Detach the share memory segment to the space memory of the processus
  if (shmdt(buffer)) {
    logEmit(LOG_ERR, "shdt fails to detach share memory: %s", 
	    strerror(errno)); 
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to write share memory"); 
  }
  return rc;
}

/*=======================================================================
 * Function   : shmRead
 * Description: int shmRead(char* pathFile, int shmSize,
 *                       void (*callback)(void*, int, void*), void* arg)
 * Synopsis   : Perform callback function on shm.
 * Input      : char* filePath: needed to create IPC key
 *              int shmSize: the size of the shm
 *              void (*callback)(void*, int, void*): callback function
 *              void* arg: argument to pass to the callback function
 * Output     : TRUE on success
 =======================================================================*/
int 
shmRead(char* pathFile, int shmSize,
	void (*callback)(void*, int, void*), void* arg)
{
  int rc = FALSE;
  key_t key;
  int sem;
  int shm;
  struct sembuf sembuf;
  void* buffer = NULL;

  logEmit(LOG_DEBUG, "%s", "shmRead");

  if (pathFile == NULL || *pathFile == (char)0) {
    logEmit(LOG_ERR, "%s", "need a path file's path to create shm key"); 
    goto error;
  }
  
  // Create a new IPC key base on an i-node + minor peripherical 
  // number and a project number
  if ((key = ftok(pathFile, MISC_SHM_PROJECT_ID)) == -1) {
    logEmit(LOG_ERR, "ftok fails to create IPC key: %s", strerror(errno)); 
    logEmit(LOG_NOTICE, "ftok using path: %s", pathFile); 
    goto error;
  }
  
  // Try to match an existing semaphore with the key
  if ((sem = semget(key, 1, 0)) == -1) {
    logEmit(LOG_ERR, "semget fails matching 0x%x IPC key: %s", 
	    key, strerror(errno)); 
    logEmit(LOG_NOTICE, "%s", "(semaphore as gone)");
    goto error;
  }

  // Try to match an existing share memory segment with the key
  if ((shm = shmget(key, 0, 0)) == -1) {
    logEmit(LOG_ERR, "shmget fails matching  0x%x IPC key: %s", 
	    key, strerror(errno)); 
    logEmit(LOG_NOTICE, "%s", "(share memory as gone)");
    goto error;
  }
  
  // Attach the share memory segment to the space memory of the processus
  if ((buffer = shmat(shm, NULL, SHM_RDONLY)) == NULL) {
    logEmit(LOG_ERR, "shmat fails to attach share memory: %s", 
	    strerror(errno)); 
    goto error;
  }

  // P (may I ?)
  sembuf.sem_num = 0;
  sembuf.sem_op = -1;
  sembuf.sem_flg = 0;
  if (semop(sem, &sembuf, 1) < 0) {
    logEmit(LOG_ERR, "semop fails asking for semaphore: %s", 
	    strerror(errno)); 
    goto error;
  }

  // Work on the share memory 
  callback(buffer, shmSize, arg);

  // V (Please)
  sembuf.sem_op = 1;
  if (semop(sem, &sembuf, 1) < 0) {
    logEmit(LOG_ERR, "semop fails returning back semaphore: %s", 
	    strerror(errno)); 
    goto error;
  }

  // Detach the share memory segment to the space memory of the processus
  if (shmdt(buffer)) {
    logEmit(LOG_ERR, "shdt fails to detach share memory: %s", 
	    strerror(errno)); 
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to read share memory"); 
  }
  return rc;
}

/*=======================================================================
 * Function   : shmFree
 * Description: Free shm
 * Synopsis   : int shmFree(char* pathFile, int shmSize)
 * Input      : char* filePath: needed to create IPC key
 *              int shmSize: the size of the shm
 * Output     : TRUE on success
 =======================================================================*/
int 
shmFree(char* pathFile, int shmSize)
{
  int rc = FALSE;
  key_t key;
  int sem = 0;
  int shm = 0;
  union semun arg;

  logEmit(LOG_DEBUG, "%s", "shmFree");

  if (pathFile == NULL || *pathFile == (char)0) {
    logEmit(LOG_ERR, "%s", "need a path file's path to create shm key"); 
    goto error;
  }
  
  // Create a new IPC key base on an i-node + minor peripherical 
  // number and a project number
  if ((key = ftok(pathFile, MISC_SHM_PROJECT_ID)) == -1) {
    logEmit(LOG_ERR, "ftok fails to create IPC key: %s", strerror(errno)); 
    logEmit(LOG_ERR, "ftok using path: %s", pathFile); 
    goto error;
  }
  
  // Try to match an existing semaphore with the key
  if ((sem = semget(key, 1, 0)) == -1) {
    logEmit(LOG_ERR, "semget fails matching IPC key: %s", strerror(errno)); 
    goto error;
  }

  // Try to match an existing share memory segment with the key
  if ((shm = shmget(key, shmSize, 0)) == -1) {
    logEmit(LOG_ERR, "shmget fails matching IPC key: %s", strerror(errno)); 
    goto error;
  }

  // Free share memory IPC ressource
  if (shmctl(shm, IPC_RMID, NULL) == -1) {
    logEmit(LOG_ERR, "shmctl fails: %s", strerror(errno)); 
    goto error;
  }

  // Free semaphore IPC ressource
  arg.val = 0;
  if (semctl(sem, 0, IPC_RMID, arg) == -1) {
    logEmit(LOG_ERR, "semctl fails: %s", strerror(errno)); 
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to free share memory"); 
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "command.h"
GLOBAL_STRUCT_DEF;
#define MDTX_SHM_BUFF_SIZE 4

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
    {"read", no_argument, NULL, 'r'},
    {"write", no_argument, NULL, 'w'},
    {"clean", no_argument, NULL, 'c'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
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
    rc = shmWrite(argv[0], MDTX_SHM_BUFF_SIZE, exempleForWrite, NULL);
    break;
  case READER:
    rc = shmRead(argv[0], MDTX_SHM_BUFF_SIZE, exempleForRead, NULL);
    break;
  case CLEANER:
    rc = shmFree(argv[0], MDTX_SHM_BUFF_SIZE);
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
 
#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
