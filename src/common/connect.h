/*=======================================================================
 * Version: $Id: connect.h,v 1.2 2014/11/13 16:36:23 nroche Exp $
 * Project: MediaTeX
 * Module : server/connect
 *
 * Manage socket connexion to the server and sending archiveTree

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

#include "../memory/recordTree.h"

#ifndef MDTX_BUS_CONNECT_H
#define MDTX_BUS_CONNECT_H 1

int buildServerAddress(Server* server);
int connectServer(Server* server);
int upgradeServer(int socket, RecordTree* tree, char* fingerPrint);

#endif /* MDTX_BUS_CONNECT_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
