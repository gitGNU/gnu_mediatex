/*=======================================================================
 * Project: MediaTeX
 * Module : utfunc
 *
 * Functions only used by unit tests

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

#ifndef MDTX_SERVER_UTFUNC_H
#define MDTX_SERVER_UTFUNC_H 1

#include "mediatex-types.h"
#include "server/mediatex-server.h"

/* API */

int utCleanCaches();
int utCopyFileOnCache(Collection* coll, char* srcdir, char* file);

Record* utLocalRecord(Collection* coll, char* hash, off_t size, 
		      Type type, char* extra);
Record* utRemoteDemand(Collection* coll, Server* server);

int utAddFinalDemand(Collection* coll);
int utAddLocalDemand(Collection* coll, char* hash, off_t size, 
		     char* extra);

Connexion* utConnexion(Collection* coll, MessageType messageType, 
		       Server* from);
Connexion* utUploadMessage(Collection* coll, char* extra);
Connexion* utCgiMessage(Collection* coll, char* mail);
Connexion* utHaveMessage1(Collection* coll, char* extra);
Connexion* utHaveMessage2(Collection* coll, char* extra);

/*=======================================================================
 * Function   : Print test purpose or result
 * Description: Print the current archive tree
 * Synopsis   : void printResult(char* topo)
 * Input      : char* format: like for printf  
 *              char* topo : what to say about we print
 *              Collection* coll : cache to display if provided
 * Output     : N/A
 * Note       : if no collection is provided, we introduce.
 =======================================================================*/
#define utLog(format, topo, coll)					\
  if (coll == 0) {							\
    logMain(LOG_NOTICE, "%s",						\
	    "-------------------------------------------------------"); \
    logMain(LOG_NOTICE, format, (char*)topo);				\
    logMain(LOG_NOTICE, "%s",						\
	    "......................................................."); \
  }									\
  else {								\
    logMain(LOG_NOTICE, "%s",						\
	    "......................................................."); \
    logMain(LOG_NOTICE, format, (char*)topo);				\
    logMain(LOG_NOTICE, "%s",						\
	    "......................................................."); \
    logRecordTree(LOG_MAIN, LOG_NOTICE,					\
		  ((Collection*)coll)->cacheTree->recordTree, 0);	\
    logMain(LOG_NOTICE, "%s",						\
	    "-------------------------------------------------------"); \
  }

#endif /* MDTX_SERVER_UTFUNC_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
