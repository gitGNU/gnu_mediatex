/*=======================================================================
 * Project: MediaTeX
 * Module : checksums
 *
 * md5sum computation

 MediaTex is an Electronic Records Management System
 Copyright (C) 2012  2017 Nicolas Roche

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

#ifndef MDTX_MISC_PERM_H
#define MDTX_MISC_PERM_H 1

#include "mediatex.h"
#include <time.h>

typedef struct Perm {
  char user[32];
  char group[32];
  mode_t mode;
  char acl[128];
} Perm;
  
time_t currentTime();
int checkDirectoryPerm(char* coll, char* path, int index);

// only used by server (mainly extract.c)
int callAccess(char* path, int* isThere);
int checkDirectory(char* path, int *isDirectory);
int makeDir(char* base, char* path, mode_t mode);
int removeDir(char* base, char* path);
int getUnusedPath(char* path, int lenFilename) ;
int buildAbsoluteTargetPath(char** newAbsolutePath, 
			    char* prefix, char* postfix);

#endif /* MDTX_MISC_PERM_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
