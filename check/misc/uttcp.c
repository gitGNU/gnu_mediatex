/* ======================================================================= 
 * Version: $Id: uttcp.c,v 1.4 2015/08/13 21:14:30 nroche Exp $
 * Project: 
 * Module : tcp socket

 * deal with tcp sockets

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

#include "mediatex.h"

//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
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
  char* addr = 0;

  // rq: getpeername can retrieve struct sockaddr* from sock
  addr = getHostNameByAddr(&address_accepted->sin_addr);
  logMain(LOG_INFO, "Accepting connexion from %s (%s)",
	  inet_ntoa(address_accepted->sin_addr),
	  //ntohs(address_accepted->sin_port),
	  addr);
  free(addr);

  /* read a word an reply by an anagrame */
  n = tcpRead(sock, buffer, 98);
  buffer[n] = (char)0;
  logMain(LOG_NOTICE, "server read>  %s", buffer);

  for (i=0; i<n/2; ++i){
    buffer[99] = buffer[i];
    buffer[i]   = buffer[n-1-i];
    buffer[n-1-i] = buffer[99];
  }

  buffer[n] = (char)0;
  if (!tcpWrite(sock, buffer, n)) {
    logMain(LOG_ERR, "server error writing");
  }
  else {
    logMain(LOG_NOTICE, "server write> %s", buffer);
  }

  /* end */
  close(sock);

  addr = getHostNameByAddr(&address_accepted->sin_addr);
  logMain(LOG_INFO, "Connexion closed with %s (%s)",
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

  logMain(LOG_NOTICE, "client write> %s", query);
  if (!tcpWrite(sock_connected, query, strlen(query))) {
    logMain(LOG_ERR, "client error writing");
    goto error;
  }
  
  if (shutdown(sock_connected, SHUT_WR)) {
    logMain(LOG_ERR, "shutdown: %s", strerror(errno));
  }

  n = tcpRead(sock_connected, buffer, 99);
  buffer[n] = (char)0;
  logMain(LOG_NOTICE, "client read>  %s", buffer);

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
    logMain(LOG_ERR, "cannot convert DEFAULT_PORT into service");
    goto error;
  }
  
  // Client
  if (isClient) {
    usleep(500);
    logMain(LOG_NOTICE,  "client mode");
    
    if (!buildSocketAddress(&address, serverToConnect, "tcp", service)) {
      logMain(LOG_ERR, "error while building socket address");
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
    logMain(LOG_NOTICE,  "server mode");
    
    if (!buildSocketAddress(&address, (char*)0, "tcp", service)) {
      logMain(LOG_ERR, "error while building socket address");
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

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
