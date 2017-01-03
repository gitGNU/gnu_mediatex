/*=======================================================================
 * Project: MediaTex
 * Module : unit tests
 *
 * test for getcgivars

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche

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

#include "mediatex.h"

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

  return;
}


/*=======================================================================
 * Function   : main 
 * Description: Standard "hello, world" program, 
 *              that also shows all CGI input
 * Synopsis   : ./utgetcgivars
 * Input      : HTTP global variables, ex: 
 *              REQUEST_METHOD=GET
 *              QUERY_STRING="hash=123&size=12"
 *              SCRIPT_FILENAME=/cgi-bin/hello.cgi
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char **cgivars; 
  int i;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS;
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  /** Print the CGI response header, required for all HTML output.**/
  /** Note the extra \n, to send the blank line.                  **/
  printf("Content-type: text/html\n\n") ;
    
  /** Finally, print out the complete HTML response page.         **/
  printf("<html>\n") ;
  printf("<head><title>CGI Results</title></head>\n") ;
  printf("<body>\n") ;
  printf("<h1>Hello, world.</h1>\n") ;
  printf("Your CGI input variables were:\n") ;
  printf("<ul>\n") ;

  /** First, get the CGI variables into a list of strings         **/
  cgivars= getcgivars() ;

  if (cgivars == (char**)0) {
      fprintf(stdout, "usage: you must call this CGI script via apache\n");
      goto error;
    }
    
  /** Print the CGI variables sent by the user.  Note the list of **/
  /**   variables alternates names and values, and ends in 0.  **/
  for (i=0; cgivars[i]; i+= 2)
    printf("<li>[%s] = [%s]\n", cgivars[i], cgivars[i+1]) ;
        
  printf("</ul>\n") ;
  printf("</body>\n") ;
  printf("</html>\n") ;

  /** Free anything that needs to be freed **/
  freecgivars(cgivars);
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
