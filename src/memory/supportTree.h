/*=======================================================================
 * Project: MediaTeX
 * Module : archive tree
 *
 * Support producer interface

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche
 
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

#ifndef MDTX_MEMORY_SUPP_H
#define MDTX_MEMORY_SUPP_H 1

#include "mediatex-types.h"

struct Support
{
  char     name[MAX_SIZE_STRING+1];

  char     quickMd5sum[MAX_SIZE_MD5+1];
  char     fullMd5sum[MAX_SIZE_MD5+1];
  char     quickShasum[MAX_SIZE_SHA+1];
  char     fullShasum[MAX_SIZE_SHA+1];

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

int isSupportFile(Support* self);

#endif /* MDTX_MEMORY_SUPP_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
