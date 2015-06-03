/*=======================================================================
 * Version: $Id: alloc.h,v 1.3 2015/06/03 14:03:44 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
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

#ifndef MISC_ALLOC_H
#define MISC_ALLOC_H 1

#include "log.h"
#include <malloc.h>

#define malloc(s) mdtxMalloc(s, __FILE__, __LINE__)
#define free(p) mdtxFree(p, __FILE__, __LINE__)
#define remind(p) mdtxFakeMalloc(p, __FILE__, __LINE__)

int initMalloc(size_t max, int (*callBack)(long));
void memoryStatus();
void exitMalloc();

void* mdtxMalloc(size_t size, char* file, int line);
void mdtxFakeMalloc(void* ptr, char* file, int line);
void mdtxFree(void* ptr, char* file, int line);

#endif /* MISC_ALLOC_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
