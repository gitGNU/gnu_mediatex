/*=======================================================================
 * Version: $Id: supportTree.h,v 1.1 2014/10/13 19:39:13 nroche Exp $
 * Project: MediaTeX
 * Module : archive tree
 *
 * Support producer interface

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

#ifndef MEMORY_ADMSUPP_H
#define MEMORY_ADMSUPP_H 1

#include "confTree.h"

#include <sys/types.h>

struct Support
{
  char     name[MAX_SIZE_NAME+1];

  Archive* archive;
  char     quickHash[MAX_SIZE_HASH+1];
  char     fullHash[MAX_SIZE_HASH+1];

  // off_t must be on 64bit to manage CDRom
  // (set -D_FILE_OFFSET_BITS=64 to CFLAGS)
  off_t    size; 

  char     status[MAX_SIZE_STAT+1];

  // time_t => 2011/09/05,14:11:45 char[20]
  time_t   firstSeen; 
  time_t   lastCheck; 
  time_t   lastSeen;

  // Collection sharing the suport
  RG* collections;

  // Computed
  float    score;
};

/* API */

int cmpSupport(const void *p1, const void *p2);
Support* createSupport(void);
Support* destroySupport(Support* self);
int serializeSupport(Support* self, FILE *output);
int serializeSupports();

Support* getSupport(char* label);
Support* addSupport(char* label);
int delSupport(Support* self);

#endif /* MEMORY_ADMSUPP_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
