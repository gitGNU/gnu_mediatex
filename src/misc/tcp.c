/* ======================================================================= 
 * Version: $Id: tcp.c,v 1.2 2014/11/13 16:36:48 nroche Exp $
 * Project: 
 * Module : tcp socket

 * deal with tcp sockets

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
 ======================================================================= */

#include "../mediatex.h"
#include "log.h"
#include "tcp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

  logEmit(LOG_DEBUG, "%s", "acceptTcpSocket");

  if ((sock_listening = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    logEmit(LOG_ERR, "socket: %s", strerror(errno));
    goto error;
  }

  // so as to re-use socket address
  if (setsockopt(sock_listening, SOL_SOCKET, SO_REUSEADDR, &autorisation, 
		 sizeof(int)) == -1){
      logEmit(LOG_ERR, "setsockopt: %s", strerror(errno));
      goto error;
    }
  
  while (bind(sock_listening, (struct sockaddr*) address_listening, 
	      sizeof(struct sockaddr_in)) < 0) {
    if (errno != EADDRINUSE) {
      logEmit(LOG_ERR, "bind: ", strerror(errno));
      goto error;
    }

    logEmit(LOG_WARNING, "%s", "address in use: exiting");
    goto error; // maybe to comment here in production mode
  }

  if (listen(sock_listening, 5) != 0) {
    logEmit(LOG_ERR, "listen: %s", strerror(errno));
    goto error;
  }

  logEmit(LOG_NOTICE, "Listenning on %s:%u",
	  inet_ntoa(address_listening->sin_addr),
	  ntohs(address_listening->sin_port));

  // accept incoming sockets
  while (env.running) {
    if ((sock_accepted 
	 = accept(sock_listening,
		  (struct sockaddr *) &address_accepted, &length)) < 0) {
      logEmit(LOG_ERR, "accept: %s", strerror(errno));
      goto error;
    }
    
    // the server deal with the client
    rc = server(sock_accepted, &address_accepted);
  }

  rc = TRUE;
 error:
  //env.running = FALSE; // do not exit
  if(!rc) {
    logEmit(LOG_ERR, "%s", "acceptTcpSocket fails");
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
    logEmit(LOG_ERR, "socket: %s", strerror(errno));
    goto error;
  }
  
  logEmit(LOG_INFO, "connecting to %s:%u",
	  inet_ntoa(address_server->sin_addr),
	  ntohs(address_server->sin_port));

  if (connect(rc, (struct sockaddr*) address_server, 
	      sizeof(struct sockaddr_in)) != 0) {
    logEmit(LOG_INFO, "connect fails: %s", strerror(errno));
    goto error;
  }

  return rc; // please don't forget to close it next
 error:
  close(rc);
  // not really an error as remote server may be offline
  //logEmit(LOG_ERR, "%s", "connectTcpSocket fails");
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

   while ((read = recv(sd, next, remaining, 0)) != 0)
     { 
       if (read == -1) {
	 errorNb = errno;

	 if (errno != EINTR) {
	   logEmit(LOG_ERR, "reading socket error: %s", strerror(errorNb));
	   goto error;
	 }
	 logEmit(LOG_WARNING, "reading socket interrupted by kernel: %s", 
		 strerror(errorNb));
       }
       else {
	 rc += read;
	 next += read;
	 logEmit(LOG_DEBUG, "read %d ; remain %d", rc, bufferSize - rc);
       }
     }

 error:
   return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "command.h"
GLOBAL_STRUCT_DEF;

/*=======================================================================
 * Function   : tcpServerExemple
 * Description: deal with a tcp client socket
 * Synopsis   : void tcpServerExemple(int sock)
 * Input      : the client socket
 * Output     : 0 on error, 1 when success
 =======================================================================*/
int
tcpServerExemple(int sock, struct sockaddr_in* address_accepted)
{
  int rc = TRUE;
  char buffer[100];
  int i,n = 0;  
  char* addr = NULL;

  // rq: getpeername can retrieve struct sockaddr* from sock
  addr = getHostNameByAddr(&address_accepted->sin_addr);
  logEmit(LOG_INFO, "Accepting connexion from %s (%s)",
	  inet_ntoa(address_accepted->sin_addr),
	  //ntohs(address_accepted->sin_port),
	  addr);
  free(addr);

  /* read a word an reply by an anagrame */
  n = tcpRead(sock, buffer, 98);
  buffer[n] = (char)0;
  logEmit(LOG_NOTICE, "server read>  %s", buffer);

  for (i=0; i<n/2; ++i){
    buffer[99] = buffer[i];
    buffer[i]   = buffer[n-1-i];
    buffer[n-1-i] = buffer[99];
  }

  buffer[n] = (char)0;
  if (!tcpWrite(sock, buffer, n)) {
    logEmit(LOG_ERR, "%s", "server error writing");
  }
  else {
    logEmit(LOG_NOTICE, "server write> %s", buffer);
  }

  /* end */
  close(sock);

  addr = getHostNameByAddr(&address_accepted->sin_addr);
  logEmit(LOG_INFO, "Connexion closed with %s (%s)",
	  inet_ntoa(address_accepted->sin_addr),
	  //ntohs(address_accepted->sin_port),
	  addr);
  free(addr);

  env.running = FALSE;
  return rc;
}

/*=======================================================================
 * Function   : tcpClientExemple
 * Description: talk to a server
 * Synopsis   : void tcpServerExemple(int sock_connected)
 * Input      : the client socket
 * Output     : 0 on error, 1 when success
 =======================================================================*/
int
tcpClientExemple(int sock_connected)
{
  int rc = TRUE;
  char* query = ".PCT morf olleH";
  char buffer[100];
  int n = 0;

  logEmit(LOG_NOTICE, "client write> %s", query);
  if (!tcpWrite(sock_connected, query, strlen(query))) {
    logEmit(LOG_ERR, "%s", "client error writing");
    goto error;
  }
  
  if (shutdown(sock_connected, SHUT_WR) != 0) {
    logEmit(LOG_ERR, "shutdown: %s", strerror(errno));
  }

  n = tcpRead(sock_connected, buffer, 99);
  buffer[n] = (char)0;
  logEmit(LOG_NOTICE, "client read>  %s", buffer);

 error:
  close(sock_connected);
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
 * Description: Unit test for tcp module
 * Synopsis   : uttcp
 * Input      : -c option for client (else act as a server)
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  int isClient = FALSE;
  char service[10];
  struct sockaddr_in address;
  int socket = -1;
  const int portToUse = 65000;
  const char* serverToConnect = "localhost";  
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"c";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"client", required_argument, NULL, 'c'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
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
    logEmit(LOG_ERR, "%s", "cannot convert DEFAULT_PORT into service");
    goto error;
  }
  
  // Client
  if (isClient) {
    usleep(500);
    logEmit(LOG_NOTICE, "%s",  "client mode");
    
    if (!buildSocketAddress(&address, serverToConnect, "tcp", service)) {
      logEmit(LOG_ERR, "%s", "error while building socket address");
      goto error;
    }
    
    if ((socket = connectTcpSocket(&address)) == -1) {
      goto error;
    }
    
    // launch the client
    if (!tcpClientExemple(socket)) goto error;
    usleep(100);
  }
  
  // Server
  else {
    logEmit(LOG_NOTICE, "%s",  "server mode");
    
    if (!buildSocketAddress(&address, (char*)0, "tcp", service)) {
      logEmit(LOG_ERR, "%s", "error while building socket address");
      goto error;
    }
    
    // launch the server
    if (!acceptTcpSocket(&address, tcpServerExemple)) goto error;
  }
  /************************************************************************/

  rc = TRUE;
 error:
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
