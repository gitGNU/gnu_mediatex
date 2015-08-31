/*=======================================================================
 * Version: $Id: archive.h,v 1.10 2015/08/31 00:14:52 nroche Exp $
 * Project: MediaTeX
 * Module : archive tree
 *
 * Archive producer interface

 MediaTex is an Electronic Archives Management System
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

#ifndef MDTX_MEMORY_ARCHIVE_H
#define MDTX_MEMORY_ARCHIVE_H 1

#include "mediatex-types.h"

// type write into Record struct
typedef enum {UNUSED = 0, USED, WANTED, ALLOCATED, AVAILABLE, TOKEEP,
	      ASTATE_MAX} AState;

struct Archive
{
  // not easy to parse into uchar[16]
  char  hash[MAX_SIZE_HASH+1]; 
  int   id;  // id used to serialize html pages
  int   tag; // dedicated for external use

  // off_t must be 64bit to manage CDRom size
  // (_FILE_OFFSET_BITS set to 64 by mediatex.h)
  off_t size;

  // serverTree related data
  RG*   images;
  float imageScore;

  // extractTree related data
  RG*        fromContainers; // (FromAsso*)
  Container* toContainer;    // only one: (choose a rule TGZ or TAR+GZ)
  float      extractScore;   // computed value used by cache
  time_t     uploadTime;     // uploaded archive (from INC container)

  // documentTree related data
  RG* documents;
  RG* assoCaracs;

  // recordTree related data
  RG* records; // related record to destroy if we remove this archive

  // cacheTree related to data
  AState  state;
  RG*     demands;        // Record*
  RG*     remoteSupplies; // Record*
  RG*     finalSupplies; // Record*
  Record* localSupply;
  int     nbKeep;
  time_t  backupDate;     // date to set after last unkeep
};

/* API */
char* strAState(AState state);
int cmpArchive(const void *p1, const void *p2);
int cmpArchive2(const void *p1, const void *p2);
int cmpArchiveSize(const void *p1, const void *p2);
int cmpArchiveScore(const void *p1, const void *p2);
Archive* getArchive(Collection* coll, char* hash, off_t size);
Archive* addArchive(Collection* coll, char* hash, off_t size);
int delArchive(Collection* coll, Archive* archive);

/* Private */
Archive* createArchive(void);
Archive* destroyArchive(Archive* self);
int diseaseArchive(Collection* coll, Archive* arch);
int diseaseArchives(Collection* coll);
int isIncoming(Collection* coll, Archive* self);
int hasExtractRule(Archive* self);

#endif /* MDTX_MEMORY_ARCHIVE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
