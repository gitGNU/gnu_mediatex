/*=======================================================================
 * Version: $Id: tcp.h,v 1.4 2015/06/30 17:37:35 nroche Exp $
 * Project: mediatex
 * Module : tcp
 *
 * socket tcp

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

#ifndef MDTX_MISC_TCP_H
#define MDTX_MISC_TCP_H 1

#include "address.h"

int acceptTcpSocket(const struct sockaddr_in* address_listening, 
		    int (*server)(int, struct sockaddr_in*));
int connectTcpSocket(const struct sockaddr_in* address_server);
int tcpWrite(int sd, char* buffer, size_t bufferSize);
size_t tcpRead(int sd, char* buffer, size_t bufferSize);

#endif /* MDTX_MISC_TCP_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
