/*=======================================================================
 * Version: $Id: notify.h,v 1.1 2014/10/13 19:39:55 nroche Exp $
 * Project: MediaTeX
 * Module : server/notify
 *
 * Manage notify for remote servers: send local md5sums list

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

#ifndef MDTX_SERVER_NOTIFY_H
#define MDTX_SERVER_NOTIFY_H 1

#include "../memory/recordTree.h"
#include "threads.h"

extern int running; // from threads.c

typedef struct NotifyData {

  Collection* coll;
  RG* toNotify; // records
  int found;

} NotifyData;

int sendRemoteNotify(Collection* coll); 
//int sendAllRemoteNotify(); 

// TO TEST IN MAIN
int acceptRemoteNotify(RecordTree* tree, Connexion* connexion);

#endif /* MDTX_SERVER_NOTIFY_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
