/*=======================================================================
 * Project: MediaTeX
 * Module : connect
 *
 * Open and write to servers socket

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

#include "mediatex-types.h"

#ifndef MDTX_COMMON_CONNECT_H
#define MDTX_COMMON_CONNECT_H 1

int buildServerAddress(Server* server);
int connectServer(Server* server);
int upgradeServer(int socket, RecordTree* tree, char* fingerPrint);

#endif /* MDTX_COMMON_CONNECT_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
