/* ======================================================================= 
 * Version: $Id: utaddress.c,v 1.6 2015/08/07 17:50:24 nroche Exp $
 * Project: Mediatex
 * Module : socket address

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
extern void mdtxFree(void* ptr, char* file, int line);
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
  fprintf(stderr, "\n\t\t[ -H hostname ]");

  miscOptions();
  fprintf(stderr, "  ---\n" 
	  "  -H, --host\thostname or IP address\n");
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
  char* localhost = "localhost";
  char* inputHost = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS "H:";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"host", required_argument, 0, 'H'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'H':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the hostname\n",
		programName);
	rc = EINVAL;
	break;
      }
      if (!(inputHost = createString(optarg))) {
	fprintf(stderr, "cannot malloc the input hostname: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      break;

      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/

  // test on 127.0.0.1
  if (!buildSocketAddressEasy(&address, 0x7f000001, 7)) {
    logMain(LOG_ERR, "error while building socket address (1)");
    goto error;
  } 
  if ((text = getHostNameByAddr(&address.sin_addr)) == 0)
    goto error;
  
  printf("host name of 0x7f000001 is: %s\n", text);
  mdtxFree(text, __FILE__, __LINE__);

  // test in localhost (default) or input host parameter
  if (!inputHost) inputHost = localhost;
  if (!getIpFromHostname(&ipv4, inputHost)) goto error;;
  printf("IP of %s is: %s\n", inputHost, inet_ntoa(ipv4)); 

  if (!buildSocketAddress(&address, inputHost, "udp", "echo")) {
    logMain(LOG_ERR, "error while building socket address (2)");
    goto error;
  }
  
  if ((text = getHostNameByAddr(&address.sin_addr)) == 0)
    goto error;

  printf("host name of %s is: %s\n", inet_ntoa(ipv4), text);
  mdtxFree(text, __FILE__, __LINE__);
  /************************************************************************/

  rc = TRUE;
 error:
  if (inputHost != localhost) destroyString(inputHost);
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
