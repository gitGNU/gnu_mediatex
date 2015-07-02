/* ======================================================================= 
 * Version: $Id: address.h,v 1.5 2015/07/02 12:14:07 nroche Exp $
 * Project: 
 * Module : socket

 * affect socket address

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
 ======================================================================= */

#ifndef MDTX_MISC_ADDRESS_H
#define MDTX_MISC_ADDRESS_H 1

#include <netinet/in.h>
#include <arpa/inet.h>  // for inet_ntoa

char* getHostNameByAddr(struct in_addr* inAddr);

int getIpFromHostname(struct in_addr *ipv4, const char* hostname);

int buildSocketAddressEasy(struct sockaddr_in* rc, 
			   unsigned long int host, 
			   short int port);

int buildSocketAddress(struct sockaddr_in* rc, 
		       const char* host, 
		       const char* protocol, 
		       const char* service);

#endif /* MDTX_MISC_ADDRESS_H*/
