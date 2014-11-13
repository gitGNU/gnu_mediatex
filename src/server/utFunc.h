/*=======================================================================
 * Version: $Id: utFunc.h,v 1.2 2014/11/13 16:37:12 nroche Exp $
 * Project: MediaTeX
 * Module : utfunc
 *
 * Function use by unit tests

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

#ifndef MDTX_SERVER_UTFUNC_H
#define MDTX_SERVER_UTFUNC_H 1

#include "../memory/confTree.h"

/* API */

int utCleanCaches();
int utCopyFileOnCache(Collection* coll, char* srcdir, char* file);

int utToKeep(Collection* coll, char* hash, off_t size);
int utDemand(Collection* coll, char* hash, off_t size, char* mail);
RecordTree* ask4logo(Collection* collection, char* mail);
RecordTree* providePart1(Collection* coll, char* path);
RecordTree* providePart2(Collection* coll, char* path);

void utLog(char* format, void* topo, Collection* coll);

#endif /* MDTX_SERVER_UTFUNC_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
