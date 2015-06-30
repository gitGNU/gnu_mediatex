/*=======================================================================
 * Version: $Id: shm.c,v 1.4 2015/06/30 17:37:34 nroche Exp $
 * Project: MediaTeX
 * Module : shm
 *
 * Share memory manager

 * Note: it should be nicer to have an initialize function ?
 * Note: IPC système V need to be compilated into the linux kernel
 if not, error ENOSYS is raised

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
#include <sys/ipc.h> // ftok
#include <sys/sem.h> // semop
#include <sys/shm.h> // shmget

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
  void* buffer = 0;

  logEmit(LOG_DEBUG, "%s", "shmWrite");

  if (pathFile == 0 || *pathFile == (char)0) {
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
  if ((buffer = shmat(shm, 0, 0)) == 0) {
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
  void* buffer = 0;

  logEmit(LOG_DEBUG, "%s", "shmRead");

  if (pathFile == 0 || *pathFile == (char)0) {
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
  if ((buffer = shmat(shm, 0, SHM_RDONLY)) == 0) {
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

  if (pathFile == 0 || *pathFile == (char)0) {
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
  if (shmctl(shm, IPC_RMID, 0) == -1) {
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

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
