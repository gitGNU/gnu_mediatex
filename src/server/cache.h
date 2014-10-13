/*=======================================================================
 * Version: $Id: cache.h,v 1.1 2014/10/13 19:39:52 nroche Exp $
 * Project: MediaTeX
 * Module : cache
 *
 * Manage local cache directory and DB

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

#ifndef MDTX_SERVER_CACHE_H
#define MDTX_SERVER_CACHE_H 1

#include "../memory/confTree.h"
#include "threads.h"

/* API */

char* getAbsCachePath(Collection* coll, char* path);
char* getAbsRecordPath(Collection* coll, Record* record);
int callAccess(char* path);
int makeDir(char* base, char* path, mode_t mode);
int removeDir(char* base, char* path);
int extractCp(char* source, char* target);

int quickScan(Collection* collection);
int quickScanAll(void);
int cacheAlloc(Record** record, Collection* coll, Archive* archive);
int uploadFinaleArchive(RecordTree* recordTree, Connexion* connexion);

#endif /* MDTX_SERVER_CACHE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
