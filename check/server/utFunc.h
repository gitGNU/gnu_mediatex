/*=======================================================================
 * Version: $Id: utFunc.h,v 1.3 2015/07/28 11:45:43 nroche Exp $
 * Project: MediaTeX
 * Module : utfunc
 *
 * Functions only used by unit tests

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

#ifndef MDTX_SERVER_UTFUNC_H
#define MDTX_SERVER_UTFUNC_H 1

#include "mediatex-types.h"

/* API */

int utCleanCaches();
int utCopyFileOnCache(Collection* coll, char* srcdir, char* file);

int utToKeep(Collection* coll, char* hash, off_t size);
int utDemand(Collection* coll, char* hash, off_t size, char* mail);
RecordTree* ask4logo(Collection* collection, char* mail);
RecordTree* providePart1(Collection* coll, char* path);
RecordTree* providePart2(Collection* coll, char* path);

/*=======================================================================
 * Function   : Print test purpose or result
 * Description: Print the current archive tree
 * Synopsis   : void printResult(char* topo)
 * Input      : char* format: like for printf  
 *              char* topo : what to say about we print
 *              Collection* coll : cache to display
 * Output     : N/A
 * Note       : if no collection is provided, we introduce.
 =======================================================================*/
#define utLog(format, topo, coll)					\
  if (coll == 0) {							\
    logMain(LOG_NOTICE, "%s",						\
	    "----------------------------------------------------------"); \
    logMain(LOG_NOTICE, format, (char*)topo);				\
    logMain(LOG_NOTICE, "%s",						\
	    ".........................................................."); \
  }									\
  else {								\
    logMain(LOG_NOTICE, "%s",						\
	    ".........................................................."); \
    logMain(LOG_NOTICE, format, (char*)topo);				\
    logMain(LOG_NOTICE, "%s",						\
	    ".........................................................."); \
    serializeRecordTree(((Collection*)coll)->cacheTree->recordTree, 0, 0); \
    logMain(LOG_NOTICE, "%s",						\
	    "----------------------------------------------------------"); \
  }

#endif /* MDTX_SERVER_UTFUNC_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
