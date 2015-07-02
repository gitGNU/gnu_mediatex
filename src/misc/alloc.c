/*=======================================================================
 * Version: $Id: alloc.c,v 1.5 2015/07/02 12:14:07 nroche Exp $
 * Project: MediaTeX
 * Module : alloc
 *
 * modified malloc

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

#include "mediatex.h"
#include "alloc.h"

/* Note:
   - logOpen take 16 (logHandler) + strlen(programName) bytes
   - I guess the syslog library take 1360 more bytes
*/

// unregister macros defined in alloc.h so as to use true ones here
#undef malloc
#undef free

/*=======================================================================
 * Function   : initMalloc
 * Description: Initialize internal mutex
 * Synopsis   : static initMalloc()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
initMalloc(size_t niceLimit, int (*callback)(long))
{
  int rc = FALSE;
  Alloc* alloc;
  int err = 0;

  if (!(env.logHandler)) {
    fprintf(stderr, "please initialize logger before malloc\n");
    goto error;
  }

  if ((alloc = env.alloc)) {
    logEmit(LOG_INFO, "%s", "malloc was already initialized");
    goto setNewValues;
  }

  if (!(alloc = malloc(sizeof(Alloc)))) {
    logEmit(LOG_ERR, "%s", "malloc fails to allocate Alloc struct");
    goto error;
  }
  memset(alloc, 0, sizeof(Alloc));
  alloc->limAllocated = DEFAULT_MALLOC_LIMIT;

  if ((err = pthread_mutex_init(&alloc->mallocMutex, 
				(pthread_mutexattr_t*)0))) {
    logEmit(LOG_ERR, "pthread_mutex_init: %s", strerror(err));
    goto error;
  }

  env.alloc = alloc;
 setNewValues:
  if (niceLimit > 0) alloc->limAllocated = niceLimit;
  alloc->diseaseCallBack = callback;
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "initMalloc fails");
    if (alloc) free(alloc);
  }
  return rc;
}


/*=======================================================================
 * Function   : mdtxMalloc
 * Description: malloc asking for memory to be free if needed
 * Synopsis   : void* mdtxMalloc(size_t size)
 * Input      : size_t size: same as malloc
 * Output     : TRUE on success
 =======================================================================*/
void* mdtxMalloc(size_t size, char* file, int line)
{
  void* rc = 0;
  size_t need = 0;
  size_t prev = 0;
  size_t size2 = 0;
  Alloc* alloc = env.alloc;

  if (!alloc) {
    setEnv("(setEnv not yet called)", &env);
    logAlloc(LOG_INFO, file, line, "malloc inerly initialized");
    alloc = env.alloc;
  }

  // try to free extra memory if needed
  if (size + alloc->sumAllocated > alloc->limAllocated) {
    if (alloc->diseaseCallBack) {
      logAlloc(LOG_NOTICE, file, line,
	       "try to free some memory");
      alloc->diseaseCallBack(size);
    }
  }
  
  // try to allocate
  need = 0;
  do { 
    if ((rc = malloc(size))) break;
    logAlloc(LOG_WARNING, file, line, 
	     "malloc fails: %s", strerror(errno));
    need = (need==0)?1:(need<<1);
    prev = alloc->sumAllocated;
  } while (alloc->diseaseCallBack(need) && prev != alloc->sumAllocated);

  if (rc) {
    size2 = malloc_usable_size (rc);
    pthread_mutex_lock(&alloc->mallocMutex);
    alloc->sumAllocated += size2;
    ++alloc->nbAlloc;
    if (alloc->sumAllocated > alloc->maxAllocated) {
      alloc->maxAllocated = alloc->sumAllocated;
    }
    pthread_mutex_unlock(&alloc->mallocMutex);

    logAlloc(LOG_DEBUG, file, line,
	     "malloc n%i: %i (sum= %i / lim= %i)",
	     alloc->nbAlloc, size2, 
	     alloc->sumAllocated, alloc->limAllocated);
  } 
  else {
    logAlloc(LOG_ERR, file, line, 
	     "mdtxMalloc fails (sum= %i / lim= %i)",
	     alloc->sumAllocated, alloc->limAllocated);
  }
  
  return rc;
}


/*=======================================================================
 * Function   : mdtxFakeMalloc
 * Description: remind a system function alloc memory (to be free next)
 * Synopsis   : void* mdtxFakeMalloc(size_t size)
 * Input      : void* ptr: memory allocated
 * Output     : TRUE on success
 * Note       : Needed to take accounting of when we free variable
 *              allocated for us by external library (getcwd and scandir)
 =======================================================================*/
void mdtxFakeMalloc(void* ptr, char* file, int line)
{
  size_t size = 0;
  Alloc* alloc = env.alloc;

  if (!alloc) {
    setEnv("(setEnv not yet called)", &env);
    logAlloc(LOG_INFO, file, line, "malloc inerly initialized");
    alloc = env.alloc;
  }

  size = malloc_usable_size (ptr);

  pthread_mutex_lock(&alloc->mallocMutex);
  alloc->sumAllocated += size;
  ++alloc->nbAlloc;
  if (alloc->sumAllocated > alloc->maxAllocated) {
    alloc->maxAllocated = alloc->sumAllocated;
  }
  pthread_mutex_unlock(&alloc->mallocMutex);

  logAlloc(LOG_DEBUG, file, line,
	   "malloc n%i: %i (sum= %i / lim= %i)",
	   alloc->nbAlloc+1, size, 
	   alloc->sumAllocated, alloc->limAllocated);
 
  return;
}


/*=======================================================================
 * Function   : mdtxFree
 * Description: free udating number of used memory
 * Synopsis   : void mdtxFree(void* ptr)
 * Input      : void* ptr: memory allocated to free
 * Output     : N/A
 =======================================================================*/
void mdtxFree(void* ptr, char* file, int line)
{
  Alloc* alloc = env.alloc;
  size_t size = 0;

  if (!alloc) {
    setEnv("(setEnv not yet called)", &env);
    logAlloc(LOG_INFO, file, line, "malloc inerly initialized");
    alloc = env.alloc;
  }

  if (!ptr) goto end; // NULL pointer
  size = malloc_usable_size (ptr);

  logAlloc(LOG_DEBUG, file, line,
	   "free n%i: %i (sum= %i / lim= %i)",
	   alloc->nbAlloc, size,
	   alloc->sumAllocated - size, alloc->limAllocated);
  
  pthread_mutex_lock(&alloc->mallocMutex);
  alloc->sumAllocated -= size;
  --alloc->nbAlloc;
  pthread_mutex_unlock(&alloc->mallocMutex);
 
  free(ptr);
 end:
  return;
}

/*=======================================================================
 * Function   : exitMalloc
 * Description: Initialize internal mutex
 * Synopsis   : void exitMalloc()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
void
exitMalloc()
{
  Alloc* alloc = env.alloc;

  if (!(env.logHandler)) {
    fprintf(stderr, "please initialize logger before exiting malloc\n");
    goto end;
  }

  if (!alloc) {
    logEmit(LOG_WARNING, "%s", "malloc is not initialized");
    goto end;
  }

  if (alloc->nbAlloc != 0 || alloc->sumAllocated != 0) {
    logEmit(LOG_WARNING, "Memory leaks: n%i, sum= %i", 
	    alloc->nbAlloc, alloc->sumAllocated);
  }

  free(env.alloc);
  env.alloc = 0;
 end:
  return;
}


/*=======================================================================
 * Function   : getVmSize
 * Description: Get the RAM size we are using
 * Synopsis   : long getVmSize()
 * Input      : N/A
 * Output     : VSZ value from 'ps un' or -1 on error 
 * Note       :
 Ugh, getrusage doesn't work well on GNU/Linux.  
 Try grabbing info directly from the /proc pseudo-filesystem.  
 Reading from /proc/self/statm gives info on your own process, 
 as one line of numbers that are: virtual mem program size, 
 resident set size, shared pages, text/code, data/stack, library, 
 dirty pages.  
 The mem sizes should all be multiplied by the page size.
 =>
 echo "$(cat /proc/self/statm | cut -f1 -d" ") * 
 $(getconf PAGE_SIZE) / 1024" | bc
 =======================================================================*/
size_t getVmSize() 
{
  size_t rc  = -1;
  FILE* file = 0;
  long vm = 0;

  if ((file = fopen("/proc/self/statm", "r")) == 0) {
    logEmit(LOG_ERR, "fopen fails: %s", strerror(errno));
    goto error;
  }

  // Just need the first num: vm size
  if (fscanf (file, "%li", &vm) < 1) {
    logEmit(LOG_ERR, "fscanf fails: %s", strerror(errno));
    goto error;
  }

  if (fclose(file) != 0) {
    logEmit(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error;
  }

  // this value match cat /proc/$$/status | grep VmSize
  rc = vm * getpagesize();
 error:
  if (rc == -1) logEmit(LOG_ERR, "%s", "getVmSize fails");
  return rc;
}

/*=======================================================================
 * Function   : memoryStatus
 * Description: log the memory status
 * Synopsis   : void memoryStatus
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
void
memoryStatus(int priority, char* file, int line)
{
  char vsz[30];
  char use[30];
  char max[30];
  char lim[30];
  Alloc* alloc = env.alloc;
  
  if (!alloc) {
    logEmit(LOG_WARNING, "%s", "malloc is not initialized");
    goto error;
  }

  sprintSize(vsz, (long long unsigned int)getVmSize());
  sprintSize(use, (long long unsigned int)alloc->sumAllocated);
  sprintSize(max, (long long unsigned int)alloc->maxAllocated);
  sprintSize(lim, (long long unsigned int)alloc->limAllocated);

  logAlloc(priority, file, line, 
	   "Memory: VSZ ; SELF: actual, max reach, nice limit");
  logAlloc(priority, file, line, 
	   "%11s%15s%11s%12s", vsz, use, max, lim);

 error:
  return;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
