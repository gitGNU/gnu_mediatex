/* ======================================================================= 
 * Version: $Id: utaddress.c,v 1.1 2015/07/01 10:49:54 nroche Exp $
 * Project: 
 * Module : socket address

 * affect socket address
 * note: 
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

#include "mediatex.h"

//#include <netdb.h>

//#include <netinet/tcp.h> /* for TCP_NODELAY */
//#include <sys/socket.h>
//#include <arpa/inet.h>
GLOBAL_STRUCT_DEF;

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

  miscOptions();
  //fprintf(stderr, "\t\t---\n");

  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * Description: Unit test for udp module
 * Synopsis   : utudp
 * Input      : -c option for client (else act as a server)
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  struct sockaddr_in address;
  struct in_addr ipv4;
  char* text = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    //{"input-file", required_argument, 0, 'i'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!getIpFromHostname(&ipv4, "localhost")) goto error;;
  printf("IP of localhost is: %s\n", inet_ntoa(ipv4)); 
  
  if (!buildSocketAddressEasy(&address, 0x7f000001, 7)) {
    logEmit(LOG_ERR, "%s", "error while building socket address (1)");
    goto error;
  } 
 
  if ((text = getHostNameByAddr(&address.sin_addr)) == 0)
    goto error;

  printf("host name of 0x7f000001 is: %s\n", text);
  free(text);

  if (!buildSocketAddress(&address, "localhost", "udp", "echo")) {
    logEmit(LOG_ERR, "%s", "error while building socket address (2)");
    goto error;
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
