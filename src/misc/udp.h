/*=======================================================================
 * Project: mediatex
 * Module : udp
 *
 * socket udp
 * Note: this file is yet not used by the project

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

#ifndef MISC_SOCKET_UDP_H
#define MISC_SOCKET__UDP_H 1

#include "address.h"

int bindUdpSocket(const struct sockaddr_in* address_listening);
int openUdpSocket();
int udpWrite(int sd, struct sockaddr_in* addressTo, char* buffer, size_t bufferSize);
size_t udpRead(int sd, struct sockaddr_in* addressFrom,	char* buffer, size_t bufferSize);

#endif /* MISC_SOCKET__UDP_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
