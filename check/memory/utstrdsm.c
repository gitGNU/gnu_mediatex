/*=======================================================================
 * Version: $Id: utstrdsm.c,v 1.1 2015/07/01 10:49:46 nroche Exp $
 * Project: MediaTeX
 * Module : strdsm
 *
 * STRing Data Structure Management implementation
 * This file was originally written by Peter Felecan under GNU GPL

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Felecan Peter

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
  memoryUsage(programName);
  fprintf(stderr, " [ -i input ] [ -o output ]");

  memoryOptions();
  fprintf(stderr, "  ---\n");
  fprintf(stderr, "  -i, --input\t\tinput stream\n");
  fprintf(stderr, "  -o, --output\t\toutput stream\n");
  return;
}

/*=======================================================================
 * Function   : main
 * Author     : Peter FELECAN, Nicolas Roche
 * Description: Unit test for strdsm module.
 * Synopsis   : utstrdsm
 * Input      : stdin
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  FILE* hin = (FILE*)stdin;
  FILE* hout = (FILE*)stdout;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MEMORY_SHORT_OPTIONS"i:o:";
  struct option longOptions[] = {
    MEMORY_LONG_OPTIONS,
    {"input", required_argument, 0, 'i'},
    {"output", required_argument, 0, 'o'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.debugMemory = TRUE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {

    case 'i':
      if(isEmptyString(optarg)) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input stream\n",
		programName);
	rc = EINVAL;
      }
      else {
	if ((hin = fopen(optarg, "r")) == 0) {
	  fprintf(stderr, 
		  "cannot open '%s' for input, "
		  "will try to revert to the standard input\n", optarg);
	  rc = ENOENT;
	}
      }
      break;
		
    case 'o':
      if(isEmptyString(optarg)) {
	fprintf(stderr, 
		"%s: nil or empty argument for the output stream\n",
		programName);
	rc = EINVAL;
      }
      else {
	if ((hin = fopen(optarg, "r")) == 0) {
	  fprintf(stderr, 
		  "cannot open '%s' for input, "
		  "will try to revert to the standard output\n", optarg);
	  rc = ENOENT;
	}
      }
      break;

    GET_MEMORY_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }
  
  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;
  
  /************************************************************************/
  char buffer[BUFSIZ];
  char* string = 0;
  char* copy = 0;
  
  char* prefix = createString("prefix");
  char* suffix = createString("/suffix");

  rc = FALSE;
  prefix = catString(prefix, "/suffix");
  fprintf(hout, "concatenate 1/1 %s\n", prefix);
  
  prefix = copyString(prefix, "prefix/");
  prefix = catString(prefix, (char*)0);
  fprintf(hout, "concatenate 1/0 %s\n", prefix);
  
  prefix = destroyString(prefix);
  prefix = catString(prefix, suffix);
  fprintf(hout, "concatenate 0/1 %s\n", prefix);
   
  fprintf(hout, 
	  "Enter items, one per line.\nEnd the list by EOF (ctl-d)\n");
		
  while(fgets(buffer, BUFSIZ, hin) != 0) {	
    if (!strcmp(buffer, "quit\n")) break;
    
    fprintf(hout, "content = %s", buffer);

    fprintf(hout, "created = ");
    if ((string = createString(buffer)) == 0) goto error;
    fprintf(hout, "%s", string);
    
    fprintf(hout, "copied  = ");
    if ((copy = copyString(copy, string)) == 0) goto error;
    fprintf(hout, "%s", copy);
    
    string = destroyString(string);
    copy = destroyString(copy);
  }

  clearerr(hin);
		
  fprintf(hout, "Enter items, one per line.\nEnd the list by EOF (ctl-d)\n"
	  "Only the first 4 characters are used for the creation\n"
	  "Only the first 2 characters are copied\n");
		
  while(fgets(buffer, BUFSIZ, hin) != 0)	{

    fprintf(hout, "content = %s", buffer);
    
    fprintf(hout, "created = ");
    if ((string = createSizedString(4, buffer)) == 0) goto error;
    fprintf(hout, "%s\n", string);

    fprintf(hout, "copied  = ");
    if ((copy = copySizedString(2, copy, string)) == 0) goto error;
    fprintf(hout, "%s\n", copy);
    
    string = destroyString(string);
    copy = destroyString(copy);
  }

  destroyString(suffix);
  destroyString(prefix);
  fflush(hout);
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
