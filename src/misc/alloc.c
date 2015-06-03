/*=======================================================================
 * Version: $Id: alloc.c,v 1.3 2015/06/03 14:03:44 nroche Exp $
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

#include "alloc.h"

#include <pthread.h>

// unregister macros defined in alloc.h so as to use true ones here
#undef malloc
#undef free

static int (*diseaseCallBack)(long) = (int (*)(long))0;
static size_t sumAllocated = 0;
static size_t maxAllocated = 0;
static size_t limAllocated = DEFAULT_MALLOC_LIMIT;
static size_t nbAlloc = 0;
static int isMutexInitialized = FALSE;
static pthread_mutex_t mallocMutex;

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
  int err = 0;

  if (!isMutexInitialized) {
    if ((err = pthread_mutex_init(&mallocMutex, (pthread_mutexattr_t*)0))) {
      logAlloc(LOG_INFO, "pthread_mutex_init: %s", strerror(err));
      goto error;
    }
    isMutexInitialized = TRUE;
  }
  if (niceLimit > 0) limAllocated = niceLimit;
  diseaseCallBack = callback;

 error:
  return isMutexInitialized;
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
  
  // try to free extra memory if needed
  if (size + sumAllocated > limAllocated) {
    if (diseaseCallBack) {
      logEmitFunc(DefaultLog, LOG_NOTICE,
		  "[alloc %s:%i] try to free some memory", file, line);
      diseaseCallBack(size);
    }
  }
  
  // try to allocate
  need = 0;
  do { 
    if ((rc = malloc(size))) break;
    logAlloc(LOG_WARNING, "malloc fails: %s", strerror(errno));
    need = (need==0)?1:(need<<1);
    prev = sumAllocated;
  } while (diseaseCallBack(need) && prev != sumAllocated);

  if (rc) {
    pthread_mutex_lock(&mallocMutex);
    sumAllocated += malloc_usable_size (rc);
    ++nbAlloc;
    if (sumAllocated > maxAllocated) maxAllocated = sumAllocated;
    pthread_mutex_unlock(&mallocMutex);

    if (env.debugAlloc) {
      if (DefaultLog) {
	logEmitFunc(DefaultLog, LOG_DEBUG,
		    "[alloc %s:%i] malloc n%i: %i (sum= %i / lim= %i)",
		    file, line, nbAlloc, malloc_usable_size (rc), 
		    sumAllocated, limAllocated);
      }
      else {
	fprintf(stderr, 
		"[alloc %s:%i] malloc n%i: %i (sum= %i / lim= %i)\n",
		file, line, nbAlloc, malloc_usable_size (rc), 
		sumAllocated, limAllocated);
      }
    }

  } 
  else {
    logAlloc(LOG_ERR, "mdtxMalloc fails (sum= %i / lim= %i)",
	    sumAllocated, limAllocated);
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
  
  pthread_mutex_lock(&mallocMutex);
  sumAllocated += malloc_usable_size (ptr);
  ++nbAlloc;
  if (sumAllocated > maxAllocated) maxAllocated = sumAllocated;
  pthread_mutex_unlock(&mallocMutex);

  if (env.debugAlloc) {
    if (DefaultLog) {
      logEmitFunc(DefaultLog, LOG_DEBUG,
		  "[alloc %s:%i] malloc n%i: %i (sum= %i / lim= %i)",
		  file, line, nbAlloc+1, malloc_usable_size (ptr), 
		  sumAllocated, limAllocated);
    }
    else {
      fprintf(stderr, "[alloc %s:%i] malloc n%i: %i (sum= %i / lim= %i)\n",
	      file, line, nbAlloc+1, malloc_usable_size (ptr), 
	      sumAllocated, limAllocated);
    }
  }
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
  if (!ptr) goto end;

  if (env.debugAlloc) {
    size_t size = 0;
    size = malloc_usable_size (ptr);
    if (DefaultLog) {
      logEmitFunc(DefaultLog, LOG_DEBUG, 
		  "[alloc %s:%i] free n%i: %i (sum= %i / lim= %i)",
		  file, line, nbAlloc, size, 
		  sumAllocated - size, limAllocated);
    }
    else {
      fprintf(stderr, "[alloc %s:%i] free n%i: %i (sum= %i / lim= %i)\n",
	      file, line, nbAlloc, size, 
	      sumAllocated - size, limAllocated);
    }
  }
  
  pthread_mutex_lock(&mallocMutex);
  sumAllocated -= malloc_usable_size (ptr);
  --nbAlloc;
  pthread_mutex_unlock(&mallocMutex);
 
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
  if (nbAlloc == 0 && sumAllocated == 0) goto end;

  if (DefaultLog) {
  logAlloc(LOG_WARNING, "Memory leaks: n%i, sum= %i", 
	  nbAlloc, sumAllocated);
  }
  else {
    fprintf(stderr, "Memory leaks: n%i, sum= %i\n", 
	    nbAlloc, sumAllocated);
  }

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
    logAlloc(LOG_ERR, "fopen fails: %s", strerror(errno));
    goto error;
  }

  // Just need the first num: vm size
  if (fscanf (file, "%li", &vm) < 1) {
    logAlloc(LOG_ERR, "fscanf fails: %s", strerror(errno));
    goto error;
  }

  if (fclose(file) != 0) {
    logAlloc(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error;
  }

  // this value match cat /proc/$$/status | grep VmSize
  rc = vm * getpagesize();
 error:
  if (rc == -1) logAlloc(LOG_ERR, "%s", "getVmSize fails");
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
memoryStatus(int priority)
{
  char vsz[30];
  char use[30];
  char max[30];
  char lim[30];
  
  sprintSize(vsz, (long long unsigned int)getVmSize());
  sprintSize(use, (long long unsigned int)sumAllocated);
  sprintSize(max, (long long unsigned int)maxAllocated);
  sprintSize(lim, (long long unsigned int)limAllocated);

  if (env.debugAlloc) {
    logAlloc(priority, "%s",
	    "Memory: VSZ ; SELF: actual, max reach, nice limit");
    logAlloc(priority, "%11s%15s%11s%12s", vsz, use, max, lim);
  }
}

/************************************************************************/

#ifdef utMAIN
#include "command.h"
GLOBAL_STRUCT_DEF;

#define malloc(s) mdtxMalloc(s, __FILE__, __LINE__)
#define free(p) mdtxFree(p, __FILE__, __LINE__)

#define BLOCK_SIZE 100
#define BLOCK_MAX  10

char* ptr[BLOCK_MAX];
long s = 0;


/*=======================================================================
 * Function   : disease
 * Description: Callback function call when malloc fails
 * Synopsis   : int disease(size_t size)
 * Input      : size_t size: amount of memory to try to free
 * Output     : TRUE if some memory was free
 =======================================================================*/
int disease(long size)
{
  int rc = FALSE;
  long goal = 0;
  int i=0;

  logAlloc(LOG_INFO, "disease callback: %i", size);

  goal = maxAllocated - size;
  if (goal < 0) goal = 0;

  while (i<BLOCK_MAX && sumAllocated > goal) {
    while (i<BLOCK_MAX && !ptr[i]) ++i;
    if (i<BLOCK_MAX) {
      logAlloc(LOG_INFO, "free slot %i", i);
      free(ptr[i]);
      ptr[i] = 0;
      rc = TRUE;
    }
    ++i;
  }

  // check if something was free
  if (!rc) {
    logAlloc(LOG_ERR, "%s", "fail to disease memory");
  }
  return rc;
}

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static 
void usage(char* programName)
{
  fprintf(stderr, "The usage for %s is:\n", programName);
  fprintf(stderr, "\t%s ", programName);
  fprintf(stderr, "{ -h | "
	  "[ -f facility ] [ -s severity ] [ -l logFileName ] }");
  fprintf(stderr, "\twhere:\n");
  fprintf(stderr, "\t\t-f   : use facility for logging\n");
  fprintf(stderr, "\t\t-s   : use severity for logging\n");
  fprintf(stderr, "\t\t-l   : log to logFile\n");

  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for alloc module
 * Synopsis   : ./utalloc
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  int i = 0;
  // ---
  int rc = 0;

  extern char* optarg;
  extern int optind;
  extern int opterr;
  extern int optopt;
  
  int cOption = EOF;

  char* programName = *argv;
  char* logFile = 0;
	
  int logFacility = -1;
  int logSeverity = -1;
	
  LogHandler* logHandler = 0;
	
  // set the log handler default values
  if ((logFacility = getLogFacility("file")) == -1) {
    fprintf(stderr, "%s: incorrect facility name '%s'\n", 
	    programName, optarg); goto optError;
  }
  if ((logSeverity = getLogSeverity("debug")) == -1) {
    fprintf(stderr, "%s: incorrect severity name '%s'\n", 
	    programName, optarg); goto optError;
  }

  while(TRUE) {
    cOption = getopt(argc, argv, ":f:s:l:h");
    
    if(cOption == EOF) {
      break;
    }
    
    switch(cOption) {

    case 'f':
      if(optarg == 0) {
	fprintf(stderr, "%s: nil argument for the facility name\n", 
		programName);
	rc = 2;
      }
      else {
	if(!strlen(optarg)) {
	  fprintf(stderr, "%s: empty argument for the facility name\n", 
		  programName);
	  rc = 2;
	}
	else {
	  logFacility = getLogFacility(optarg);
	  if(logFacility == -1) {
	    fprintf(stderr, "%s: incorrect facility name '%s'\n", 
		    programName, optarg);
	    rc = 2;
	  }
	}
      }
      break;
      
    case 's':
      if(optarg == 0) {
	fprintf(stderr, "%s: nil argument for the severity name\n", 
		programName);
	rc = 2;
      }
      else {
	if(!strlen(optarg)) {
	  fprintf(stderr, "%s: empty argument for the severity name\n", 
		  programName);
	  rc = 2;
	}
	else {
	  logSeverity = getLogSeverity(optarg);
	  if(logSeverity == -1) {
	    fprintf(stderr, "%s: incorrect severity name '%s'\n", 
		    programName, optarg);
	    rc = 2;
	  }
	}
      }
      break;
	
    case 'l':
      if(optarg == 0) {
	fprintf(stderr, "%s: nil argument for the log stream\n", 
		programName);
	rc = 2;
      }
      else {
	if(!strlen(optarg)) {
	  fprintf(stderr, "%s: empty argument for the log stream\n", 
		  programName);
	  rc = 2;
	}
	else {
	  logFile = (char*)malloc(sizeof(char) * strlen(optarg) + 1);
	  if(logFile != 0) {
	    strcpy(logFile, optarg);
	  }
	  else {
	    fprintf(stderr, 
		    "%s: cannot allocate memory for the log stream name\n",
		    programName);
	    rc = 2;
	  }
	}
      }
      break;
      
    case 'h':
      usage(programName);
	  rc = 4;
	  
	  break;
	  
    case ':':
	  usage(programName);
	  rc = 125;

	  break;

	case '?':
	  usage(programName);
	  rc = 126;

	  break;

	default:
	  usage(programName);
	  rc = 127;
			
	  break;
	}
    }

  if(rc) goto optError;

  // set the log handler
  if((logHandler = 
      logOpen(programName, logFacility, logSeverity, logFile)) 
     == 0) {
    fprintf(stderr, "%s: cannot allocate the logHandler\n", programName);
    goto optError;
  }
  logDefault(logHandler);

  /************************************************************************/
  /* Note:
     - logOpen take 16 (logHandler) + strlen(programName) bytes
     - I guess the syslog library take 1360 more bytes
   */
  
  initMalloc(2000, disease);

  for (i=1; i<BLOCK_MAX; ++i) {
    logAlloc(LOG_NOTICE, "allocating slot %i...", i);
    if (!(ptr[i] = malloc(BLOCK_SIZE*i))) goto error;
    memset(ptr[i], 42, BLOCK_SIZE*i);
    logAlloc(LOG_NOTICE, "...slot %i allocated", i);
    memoryStatus(LOG_DEBUG);
  }

  // Finished
  for (i=1; i<BLOCK_MAX; ++i) {
    if (ptr[i]) {
      logAlloc(LOG_NOTICE, "freeing slot %i...", i);
      free(ptr[i]);
      logAlloc(LOG_NOTICE, "...slot %i is free", i);
    }
  }

  memoryStatus(LOG_DEBUG);
  logAlloc(LOG_NOTICE, "sumAllocated = %i", sumAllocated);

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
