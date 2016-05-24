/*=======================================================================
 * Project: MediaTeX
 * Module : checksums
 *
 * modified malloc

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

#ifndef MDTX_MISC_ALLOC_H
#define MDTX_MISC_ALLOC_H 1

#include <malloc.h>

// replace malloc functions by our ones
#define malloc(s) mdtxMalloc(s, __FILE__, __LINE__)
#define free(p) mdtxFree(p, __FILE__, __LINE__)
#define remind(p) mdtxFakeMalloc(p, __FILE__, __LINE__)

typedef struct Alloc {
  int (*diseaseCallBack)(long); // = (int (*)(long))NULL
  size_t sumAllocated;          // = 0
  size_t maxAllocated;          // = 0
  size_t limAllocated;          // = DEFAULT_MALLOC_LIMIT
  size_t nbAlloc;               // = 0
  pthread_mutex_t mallocMutex;
} Alloc;

int initMalloc(size_t max, int (*callBack)(long));
void memoryStatus(int priority, char* file, int line);
void exitMalloc();

void* mdtxMalloc(size_t size, char* file, int line);
void mdtxFakeMalloc(void* ptr, char* file, int line);
void mdtxFree(void* ptr, char* file, int line);

#endif /* MDTX_MISC_ALLOC_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
