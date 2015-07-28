/* ======================================================================= 
 * Version: $Id: address.c,v 1.9 2015/07/28 11:45:46 nroche Exp $
 * Project: Mediatex
 * Module : socket address
 *
 * The  gethostbyname*()  and  gethostbyaddr*()  functions  are  obsolete.
 * Applications should use getaddrinfo(3) and getnameinfo(3) instead.

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

#include "mediatex-config.h"

#include <netdb.h> // gethostbyaddr_r

#include <sys/socket.h>  //
#include <netinet/tcp.h> // inet_ntoa
#include <arpa/inet.h>   //

// It seems that gethostby*_r functions are obsolete too:
//  https://sourceware.org/bugzilla/show_bug.cgi?id=515
//  using 256 we get: gethostbyname_r fails: (-1) Resolver internal error
#define GETHOSTBY_BUFFER_SIZE 1024


/*=======================================================================
 * Function   : getHostNameByAddr
 * Description: Retrieve the host name from the IP address
 * Synopsis   : getHostNameByAddr(struct in_addr* inAddr)
 * Input      : struct in_addr* inAddr
 * Output     : an allocated string for the host name
 * Note       : Need an entry in /etc/hosts for local machine.
 *              Maybe need /etc/host.conf: order hosts,bind
 *              We should prefer to retrieve IP from hostname if possible
 =======================================================================*/
char*
getHostNameByAddr(struct in_addr* inAddr)
{
  char* rc = 0;
  struct hostent sHost;
  struct hostent *result = 0;
  char buf[GETHOSTBY_BUFFER_SIZE];
  int h_errnop = 0;

  if ((gethostbyaddr_r(inAddr, sizeof(struct in_addr), AF_INET,
		       &sHost, buf, GETHOSTBY_BUFFER_SIZE, &result, &h_errnop)) 
      || !result) {
    logMisc(LOG_NOTICE, 
	    "gethostbyaddr_r: cannot retrieve host name for %s: %s",
	    inet_ntoa(*inAddr), hstrerror(h_errnop));

    switch (h_errnop) {
    case HOST_NOT_FOUND:
      logMisc(LOG_INFO, "gethostbyaddr: %s", "HOST_NOT_FOUND");
      
      // do not found an hostname: use IP instead
      if ((rc = (char*)malloc(16)) == 0) {
	logMisc(LOG_ERR, "%s", "malloc cannot allocate string for IP");
	goto error;
      }
      memset(rc, 0, 16);
      strcpy(rc, inet_ntoa(*inAddr));
      goto end;
    case NO_ADDRESS:
      logMisc(LOG_ERR, "gethostbyaddr: %s", "NO_ADDRESS");
      goto error;
    case NO_RECOVERY:
      logMisc(LOG_ERR, "gethostbyaddr: %s", "NO_RECOVERY");
      goto error;
    case TRY_AGAIN:
      logMisc(LOG_ERR, "gethostbyaddr: %s", "TRY_AGAIN");
      goto error;
    case ERANGE:
      logMisc(LOG_ERR, "gethostbyaddr: %s", "ERANGE");
      goto error;
    }
    goto error;
  }

  // normal case (found a hostname)
  if ((rc = (char*)malloc(strlen(sHost.h_name)+1)) == 0) {
    logMisc(LOG_ERR, "malloc cannot allocate string of len %i+1",
	    strlen(sHost.h_name));
    goto error;
  }
  strcpy(rc, sHost.h_name);
 end:
 error:
  return rc;
}


/*=======================================================================
 * Function   : getIpFromHostname
 * Author     : Nicolas ROCHE
 * Description: Retrieve IP address from host name 
 * Synopsis   : 
 * Input      : char* hostname = the hostname
 *              struct in_addr *ipv4 = structure to fill
 * Output     : TRUE on success
 =======================================================================*/
int
getIpFromHostname(struct in_addr *ipv4, const char* hostname)
{
  int rc = FALSE;
  int a,b,c,d;
  struct hostent sHost;
  struct hostent *result = 0;
  char buf[GETHOSTBY_BUFFER_SIZE];
  int h_errnop = 0;

  logMisc(LOG_DEBUG, "getIpFromHostname %s", hostname);
  memset(ipv4, 0, sizeof(struct in_addr));

  // It should be an ipv4 address or a host name.
  if (sscanf(hostname, "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
    if (inet_aton(hostname, ipv4) == 0) {
      logMisc(LOG_ERR, "inet_aton: ", strerror(errno));
      goto error;
    }
  }
  else {
    if (gethostbyname_r(hostname, &sHost, buf, GETHOSTBY_BUFFER_SIZE,
			&result, &h_errnop)
	|| !result) {
      logMisc(LOG_ERR, "gethostbyname_r %s: %s", 
	      hostname, hstrerror(h_errnop));
      goto error;
    }
    *ipv4 = *((struct in_addr *) sHost.h_addr);
  }

  rc = TRUE;
 error:
  if(!rc) {
    logMisc(LOG_ERR, "%s", "getIpFromHostname fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : buildSocketAddressEasy 
 * Author     : Nicolas ROCHE
 * Description: Quickly build a socket address
 * Synopsis   : int buildSocketAddressEasy(struct sockaddr_in* rc, 
 *                            unsigned long int host, short int port)
 * Input      : unsigned long int host: the IP using network format
 *              short int port:         the port
 * Output     : struct sockaddr_in* rc: the resulting socket address
                True on success
 =======================================================================*/
int
buildSocketAddressEasy(struct sockaddr_in* rc, 
		       unsigned long int host, 
		       short int port)
{
  memset(rc, 0, sizeof (struct sockaddr_in));

  rc->sin_family      = AF_INET;
  rc->sin_port        = htons(port);
  rc->sin_addr.s_addr = htonl(host);

  logMisc(LOG_INFO, "build socket address %i.%i.%i.%i:%i", 
	  (ntohl(rc->sin_addr.s_addr) & 0xFF000000) >> 24,
	  (ntohl(rc->sin_addr.s_addr) & 0x00FF0000) >> 16,
	  (ntohl(rc->sin_addr.s_addr) & 0x0000FF00) >> 8,
	  ntohl(rc->sin_addr.s_addr)  & 0x000000FF,
	  ntohs(rc->sin_port));

  return TRUE;
}


/*=======================================================================
 * Function   : buildSocketAddress
 * Author     : Nicolas ROCHE
 * Description: Build a socket address
 * Synopsis   : int buildSocketAddress(struct sockaddr_in* address, 
 *		   const char* host, const char* protocol, 
 *                 const char* service)
 * Input      : const char* host:     the hostname
 *              const char* protocol: the protocol (tcp or udb)
 *              const char* service:  the service
 * Output     : struct sockaddr_in* address: the resulting socket address
                True on success
 * TODO       : The  gethostbyname*() and gethostbyaddr*() functions 
 *              are obsolete. Applications should use getaddrinfo(3) 
 *              and getnameinfo(3) instead.
 =======================================================================*/
int
buildSocketAddress(struct sockaddr_in* address, 
		   const char* host, 
		   const char* protocol, 
		   const char* service)
{
  int rc = FALSE;
  struct protoent *sProtocol = (struct protoent *)0;
  struct servent  *sService  = (struct servent  *)0;
  int port = -1;
  struct in_addr ipv4;

  logMisc(LOG_DEBUG, "%s", "buildSocketAddress");

  // Convert protocol parameter (we need it for service conversion)
  if ((sProtocol = getprotobyname(protocol)) == (struct protoent *)0) {
    logMisc(LOG_ERR, "getprotoyname: %s", strerror(errno));
    perror("getprotobyname");
    goto error;
  }
  
  // Convert service parameter. (we need to build socket address)
  // It should be a number or a string.
  if (service == 0 || *service == (char)0) {
    logMisc(LOG_WARNING, "no port defined for %s address", host);
    port = htons(CONF_PORT);
  }
  else if (sscanf(service, "%d", &port) == 1) {
    port = htons(port);
  }
  else {
    if ((sService = getservbyname(service, sProtocol->p_name)) 
	== (struct servent*)0) {
      logMisc(LOG_ERR, "getservbyname: cannot find '%s' in /etc/services", 
	      service);
      goto error;
    }
    port = sService->s_port;
  }

  // Convert host parameter
  if (host == 0 || *host == (char)0) {
    ipv4.s_addr = htonl(INADDR_ANY); // this is ok for a server address
  }
  else {
    if (!getIpFromHostname(&ipv4, host)) goto error;
  }

  // Build the socket address
  buildSocketAddressEasy(address, ntohl(ipv4.s_addr), ntohs(port));

  rc = TRUE;
 error:
  if(!rc) {
    logMisc(LOG_ERR, "%s", "buildSocketAddress fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
