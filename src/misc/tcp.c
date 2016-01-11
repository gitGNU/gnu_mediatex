/* ======================================================================= 
 * Version: $Id: tcp.c,v 1.7 2015/08/13 21:14:36 nroche Exp $
 * Project: 
 * Module : tcp socket

 * deal with tcp sockets

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
 ======================================================================= */

#include "mediatex-config.h"

/*=======================================================================
 * Function   : acceptTcpSocket
 * Description: TCP server that accept clients socket
 *
 * Synopsis   : int acceptTcpSocket(
 *                  (const struct sockaddr_in* address_listening, 
 *                   void (*server)(int))
 *
 * Input      : the listenning socket address 
 * Output     : 0 on error, 1 when success
 =======================================================================*/
int 
acceptTcpSocket(const struct sockaddr_in* address_listening, 
		int (*server)(int, struct sockaddr_in*))
{
  int rc = FALSE;
  int sock_listening;
  int sock_accepted;
  socklen_t length = sizeof(struct sockaddr_in);
  struct sockaddr_in address_accepted;
  int autorisation = 1;

  logMisc(LOG_DEBUG, "acceptTcpSocket");

  if ((sock_listening = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    logMisc(LOG_ERR, "socket: %s", strerror(errno));
    goto error;
  }

  // so as to re-use socket address
  if (setsockopt(sock_listening, SOL_SOCKET, SO_REUSEADDR, &autorisation, 
		 sizeof(int)) == -1){
      logMisc(LOG_ERR, "setsockopt: %s", strerror(errno));
      goto error;
    }
  
  while (bind(sock_listening, (struct sockaddr*) address_listening, 
	      sizeof(struct sockaddr_in)) < 0) {
    if (errno != EADDRINUSE) {
      logMisc(LOG_ERR, "bind: ", strerror(errno));
      goto error;
    }

    logMisc(LOG_WARNING, "address in use: exiting");
    goto error; // maybe to comment here in production mode
  }

  if (listen(sock_listening, 5)) {
    logMisc(LOG_ERR, "listen: %s", strerror(errno));
    goto error;
  }

  logMisc(LOG_NOTICE, "Listenning on %s:%u",
	  inet_ntoa(address_listening->sin_addr),
	  ntohs(address_listening->sin_port));

  // accept incoming sockets
  while (env.running) {
    if ((sock_accepted 
	 = accept(sock_listening,
		  (struct sockaddr *) &address_accepted, &length)) < 0) {
      logMisc(LOG_ERR, "accept: %s", strerror(errno));
      goto error;
    }
    
    // the server deal with the client
    rc = server(sock_accepted, &address_accepted);
  }

  rc = TRUE;
 error:
  //env.running = FALSE; // do not exit
  if(!rc) {
    logMisc(LOG_ERR, "acceptTcpSocket fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : connectTcpSocket
 * Description: connect a tcp client socket
 * Synopsis   : connectTcpSocket(const struct sockaddr_in* address_server)
 * Input      : the server address to connect
 * Output     : a descriptor on the opened socket
 *              -1 on error
 =======================================================================*/
int 
connectTcpSocket(const struct sockaddr_in* address_server)
{
  int rc = -1;
  
  if ((rc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    logMisc(LOG_ERR, "socket: %s", strerror(errno));
    goto error;
  }
  
  logMisc(LOG_INFO, "connecting to %s:%u",
	  inet_ntoa(address_server->sin_addr),
	  ntohs(address_server->sin_port));

  if (connect(rc, (struct sockaddr*) address_server, 
	      sizeof(struct sockaddr_in))) {
    logMisc(LOG_INFO, "connect fails: %s", strerror(errno));
    goto error;
  }

  return rc; // please don't forget to close it next
 error:
  close(rc);
  // not really an error as remote server may be offline
  //logMisc(LOG_ERR, "connectTcpSocket fails");
  return -1;
}

/*=======================================================================
 * Function   : tcpWrite
 * Description: write into a TCP socket
 * Synopsis   : int tcpWrite(int sd, char* buffer, size_t bufferSize)
 * Input      : sd:         socket descriptor
 *              buffer:     string to write
 *              bufferSize: number of char to write
 * Output     : TRUE on success
 =======================================================================*/
int 
tcpWrite(int sd, char* buffer, size_t bufferSize)
{
  int rc = FALSE;
  char *next = buffer;
  size_t remaining = bufferSize;
  ssize_t writen = -1;
  int errorNb = 0;

  while (remaining > 0){
    while ((writen = send(sd, next, remaining, 0)) == -1)
      {
	errorNb = errno;

	if (errorNb == EINTR) {
	  logMisc(LOG_WARNING, "writting socket interrupted by kernel: %s",
		  strerror(errorNb));
	  continue;
	}
	if (errorNb != EAGAIN) {
	  logMisc(LOG_ERR, "writting socket error: %s", strerror(errorNb));
	  goto error;
	}
	logMisc(LOG_WARNING, "writting socket keep EAGAIN error: %s", 
		strerror(errorNb));
      }
    remaining -= writen;
    next += writen;
    logMisc(LOG_DEBUG, "written %d ; remain %d", writen, remaining);
  }

  rc = (remaining == 0);
 error:
  return rc;

}

/*=======================================================================
 * Function   : tcpRead
 * Description: read into a TCP socket
 * Synopsis   : size_t tcpRead(int sd, char* buffer, size_t bufferSize)
 * Input      : sd:          socket descriptor
 *              buffer:      string to write
 *              bufferSize:  number of char to write
 * Output     : number of char written             
 =======================================================================*/
size_t
tcpRead(int sd, char* buffer, size_t bufferSize)
{
   size_t rc = FALSE;
   char *next = buffer;
   size_t remaining = bufferSize;
   ssize_t read = -1;
   int errorNb = -1;

   while ((read = recv(sd, next, remaining, 0)))
     { 
       if (read == -1) {
	 errorNb = errno;

	 if (errno != EINTR) {
	   logMisc(LOG_ERR, "reading socket error: %s", strerror(errorNb));
	   goto error;
	 }
	 logMisc(LOG_WARNING, "reading socket interrupted by kernel: %s", 
		 strerror(errorNb));
       }
       else {
	 rc += read;
	 next += read;
	 logMisc(LOG_DEBUG, "read %d ; remain %d", rc, bufferSize - rc);
       }
     }

 error:
   return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
