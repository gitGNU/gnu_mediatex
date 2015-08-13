/* ======================================================================= 
 * Version: $Id: udp.c,v 1.5 2015/08/13 21:14:36 nroche Exp $
 * Project: 
 * Module : udp socket

 * deal with udp sockets
 * Note: this file is yet not used by the project

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

#include "../mediatex.h"
#include "log.h"
#include "udp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int errorNb;

/*=======================================================================
 * Function   : bindUdpSocket
 * Description: UDP server that accept clients socket
 * Synopsis   : int bindUdpSocket(
                             const struct sockaddr_in* address_listening)
 * Input      : the listenning socket address 
 * Output     : the udp listenning socket or -1 on error
 =======================================================================*/
int 
bindUdpSocket(const struct sockaddr_in* address_listening)
{
  int rc = -1;

  if ((rc = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    logEmit(LOG_ERR, "socket: %s", strerror(errno));
    goto error;
  }

  if (bind(rc, (struct sockaddr*) address_listening, 
	   sizeof(struct sockaddr_in)) < 0) {
    logEmit(LOG_ERR, "bind: ", strerror(errno));
    goto error;
  }

  logEmit(LOG_INFO, "Listenning on %s:%u",
	  inet_ntoa(address_listening->sin_addr),
	  ntohs(address_listening->sin_port));

  return rc;
 error:
  return -1;
}

/*=======================================================================
 * Function   : openUdpSocket
 * Description: open a udp client socket
 * Synopsis   : int openUdpSocket()
 * Input      : N/A
 * Output     : a descriptor on the opened socket, -1 on error
 =======================================================================*/
int 
openUdpSocket()
{
  int rc = -1;
  
  if ((rc = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    logEmit(LOG_ERR, "socket: %s", strerror(errno));
    goto error;
  }

  return rc; // please don't forget to close it next
 error:
  close(rc);
  return -1;
}

/*=======================================================================
 * Function   : connectUdpSocket
 * Description: open and connect a udp client socket
 *
 *   $ man connect:
 *
 *   Si la socket sockfd est du type SOCK_DGRAM, alors serv_addr est
 *   l'adresse à laquelle les datagrammes seront envoyés par défaut,
 *   et la seule adresse depuis laquelle ils seront reçus. 
 *
 * Synopsis   : int connectUdpSocket(const struct sockaddr_in* address_server)
 * Input      : address_server
 * Output     : a descriptor on the opened and connected socket, -1 on error
 =======================================================================*/
int 
connectUdpSocket(const struct sockaddr_in* address_server)
{
  int rc = -1;
  
  if ((rc = openUdpSocket()) < 0) {
    goto error;
  }

  logEmit(LOG_INFO, "Connecting to %s:%u",
	  inet_ntoa(address_server->sin_addr),
	  ntohs(address_server->sin_port));

  if (connect(rc, (struct sockaddr*) address_server, 
	      sizeof(struct sockaddr_in))) {
    logEmit(LOG_INFO, "connect fails: %s", strerror(errno));
    goto error;
  }

  return rc; // please don't forget to close it next
 error:
  close(rc);
  // not really an error as remote server may be offline
  //logEmit(LOG_ERR, "connectUdpSocket fails");
  return -1;
}

/*=======================================================================
 * Function   : udpWrite
 * Description: write into an UDP socket
 * Synopsis   : int udpWrite(int sd, struct sockaddr_in* addressTo,
 *  	                     char* buffer, size_t bufferSize)
 * Input      : sd:         socket descriptor
 *              addressTo:  address where to write
 *              buffer:     string to write
 *              bufferSize: number of char to write
 * Output     : TRUE on success
 =======================================================================*/
int
udpWrite(int sd, struct sockaddr_in* addressTo,
	   char* buffer, size_t bufferSize)
{
  int rc = FALSE;
  char *next = buffer;
  size_t remaining = bufferSize;
  ssize_t writen = -1;
  int errorNb = 0;

  while ((writen = 
	  sendto(sd, next, remaining, 0, 
		 (struct sockaddr*)addressTo, sizeof(struct sockaddr_in)))
	 == -1)
    {
      errorNb = errno;
      
      if (errorNb == EINTR) {
	logEmit(LOG_WARNING, "writting socket interrupted by kernel: %s", 
		strerror(errorNb));
	continue;
      }
      if (errorNb != EAGAIN) {
	logEmit(LOG_ERR, "writting socket error: %s", strerror(errorNb));
	goto error;
      }
      logEmit(LOG_WARNING, "writting socket keep EAGAIN error: %s", 
	      strerror(errorNb));
    }
  remaining -= writen;
  next += writen;
  logEmit(LOG_DEBUG, "written %d ; remain %d", writen, remaining);

  rc = (remaining == 0);
 error:
  return rc;
}

/*=======================================================================
 * Function   : udpRead
 * Description: read into an UDP socket
 *
 * $ man recvfrom:
 * 
 * Ces trois routines renvoient la longueur du message si  elles  réussis‐
 * sent. Si un message est trop long pour tenir dans le tampon, les octets
 * supplémentaires peuvent être  abandonnés  suivant  le  type  de  socket
 * utilisé.
 *
 * Synopsis   : size_t udpRead(int sd, struct sockaddr_in* addressFrom,
 *  	                     char* buffer, size_t bufferSize)
 * Input      : sd:          socket descriptor
 *              buffer:      string to write
 *              bufferSize:  number of char to write
 * Output     : addressFrom: address where we have reade
 *              rc: number of char written             
 =======================================================================*/
size_t
udpRead(int sd, struct sockaddr_in* addressFrom,
	char* buffer, size_t bufferSize)
{
  static struct sockaddr_in address;
  ssize_t rc = 0;

  // warning! we MUST initialize length
  socklen_t length = sizeof(struct sockaddr_in); 

  // with UDP, if reading buffer is too short then the resting data is lost
  while ((rc = recvfrom(sd, buffer, bufferSize, 0,
			  (struct sockaddr*)&address, &length))
	 == -1){
    errorNb = errno;
  
    if (errno != EINTR) {
      logEmit(LOG_ERR, "reading socket error: %s", strerror(errorNb));
      goto error;
    }
    logEmit(LOG_WARNING, "reading socket interrupted by kernel: %s", 
	    strerror(errorNb));
  }
 
  logEmit(LOG_DEBUG, "read %d ; remain %d", rc, bufferSize - rc);
 error:
  if (addressFrom != (struct sockaddr_in*)0)
    *addressFrom = address;

  return rc;
}


/************************************************************************/

#ifdef utMAIN
#include "command.h"
GLOBAL_STRUCT_DEF;

/*=======================================================================
 * Function   : udpServerExemple
 * Description: deal with a udp client socket
 * Synopsis   : void udpServerExemple(int sock)
 * Input      : the client socket
 * Output     : 0 on error, 1 when success
 =======================================================================*/
int
udpServerExemple(int sock_server)
{
  int rc = TRUE;
  char buffer[100];
  int n = 0;  
  struct sockaddr_in addressClient;

  while (env.running) 
    {
      n = udpRead(sock_server, &addressClient, buffer, 100); 

      buffer[n] = (char)0;	
      logEmit(LOG_NOTICE, "server read %i char> %s (from %s:%i)", 
	      n,
	      buffer,
	      inet_ntoa(addressClient.sin_addr),
	      ntohs(addressClient.sin_port));

      if (n>0) env.running = FALSE;
    }
  
  /* end */
  close(sock_server);
  return rc;
}

/*=======================================================================
 * Function   : udpClientExemple
 * Description: talk to a server
 * Synopsis   : void udpServerExemple(int sock_connected)
 * Input      : the client socket
 * Output     : 0 on error, 1 when success
 =======================================================================*/
int
udpClientExemple(int sock_client, struct sockaddr_in* address_server)
{
  int rc = TRUE;
  char* query = "Hello from UDP";

  logEmit(LOG_NOTICE, "client write>      %s [to %s:%i]", 
	  query,
	  inet_ntoa(address_server->sin_addr),
	  ntohs(address_server->sin_port));

  if (!udpWrite(sock_client, address_server, query, strlen(query))) {
      logEmit(LOG_ERR, "udpWrite failed");
    }

  /* Udp client may connect (who know) */
  if (shutdown(sock_client, SHUT_WR)) {
    errorNb = errno;
    logEmit(LOG_ERR, "shutdown: %s", strerror(errorNb));
  }
    
  /* end */
  close(sock_client);
  return rc;
}

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually argv[0].
 * Output     : N/A
 =======================================================================*/
static void 
usage(char* programName)
{
  miscUsage(programName);
  fprintf(stderr, "\n\t\t[ -c ]");

  miscOptions();
  fprintf(stderr, "  ---\n"
  	  "  -c, --client\tact as a client (default as a server)\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for udp module
 * Synopsis   : utudp
 * Input      : -c option for client (else act as a server)
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  int isClient = FALSE;
  char service[10];
  struct sockaddr_in address;
  int sock = -1;
  const int portToUse = 65000;
  const char* serverToConnect = "localhost";
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"c";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"client", required_argument, 0, 'c'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'c':
      isClient = TRUE;
      break;
      
      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }
  
  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;
  
  /************************************************************************/
  /* convert port into char* */
  if (sprintf(service, "%i", portToUse) < 0) {
    logEmit(LOG_ERR, "cannot convert DEFAULT_PORT into service");
    goto error;
  }
  
  // Client
  if (isClient) {
    usleep(500);
    logEmit(LOG_NOTICE,  "client mode");
    
    if (!buildSocketAddress(&address, serverToConnect, "udp", service)) {
      logEmit(LOG_ERR, "error while building socket address");
      goto error;
    }
    
    // launch the client
    if ((sock = connectUdpSocket(&address)) == -1) {// ne change rien ici
      goto error;
    }
    rc = udpClientExemple(sock, &address);
    usleep(100);
  }
  
  // Server
  else {
    logEmit(LOG_NOTICE,  "server mode");
    
    if (!buildSocketAddress(&address, (char*)0, "udp", service)) {
      logEmit(LOG_ERR, "error while building socket address");
      goto error;
    }
    
    // launch the server
    if ((sock = bindUdpSocket(&address)) == -1) {
      goto error;
    }
    udpServerExemple(sock);
  }
  /************************************************************************/

  rc = TRUE;
 error:
  ENDINGS;
  rc=!rc;
 optError:
  usleep(200);
  exit(rc);
}

#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
