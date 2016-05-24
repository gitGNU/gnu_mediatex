/*=======================================================================
 * Project: MediaTeX
 * Module : ardsm
 *
 * Abstract Ring Data Structure Management implementation.
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
 * Author     : Peter FELECAN, Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for ardsm module. Tests RG generic
 *              implementation by acquiring string content from the
 *              stdandard input.
 * Synopsis   : utardsm
 * Input      : stdin
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char E[] = "ABCDE";
  char* E1[] = {&E[0], &E[1],        &E[3], &E[4]};
  char* E2[] = {       &E[1], &E[2], &E[3]};
  int i = 0;
  char *it = (char *)0;
  RG* cont = 0;
  RG* ring = 0;
  RG* ring2 = 0;
  FILE* hin = (FILE*)stdin;
  FILE* hout = (FILE*)stdout;
  char buffer[BUFSIZ];
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
  env = envUnitTest;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {

    case 'i':
      if(isEmptyString(optarg)) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input stream\n",
		programName);
	rc = 2;
      }
      else {
	if ((hin = fopen(optarg, "r")) == 0) {
	  fprintf(stderr, 
		  "cannot open '%s' for input, "
		  "will try to revert to the standard input\n", optarg);
	  rc = 3;
	}
      }
      break;
		
    case 'o':
      if(isEmptyString(optarg)) {
	fprintf(stderr, 
		"%s: nil or empty argument for the output stream\n",
		programName);
	rc = 2;
      }
      else {
	if ((hin = fopen(optarg, "r")) == 0) {
	  fprintf(stderr, 
		  "cannot open '%s' for input, "
		  "will try to revert to the standard output\n", optarg);
	  rc = 3;
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
  // working with pointers (don't care about item's content)
  if (!(ring = createRing())) goto error;
  if (!(ring2 = createRing())) goto error;
  for (i=0; i<4; ++i) rgInsert(ring, E1[i]);
  for (i=0; i<3; ++i) rgInsert(ring2, E2[i]);

  if (!rgSort(ring, cmpPtr)) goto error;
  fprintf(hout, "E1: ");
  while ((it = rgNext(ring))) fprintf(hout, "%c", *it);
  fprintf(hout, "\n");

  if (!rgSort(ring2, cmpPtr)) goto error;
  fprintf(hout, "E2: ");
  while ((it = rgNext(ring2))) fprintf(hout, "%c", *it);
  fprintf(hout, "\n");
  
  if (!(cont = rgInter(ring, ring2))) goto error;
  fprintf(hout, "E1 /\\ E2: ");
  while ((it = rgNext(cont))) fprintf(hout, "%c", *it);
  fprintf(hout, "\n");
  cont = destroyOnlyRing(cont);

  if (!(cont = rgUnion(ring, ring2))) goto error;
  fprintf(hout, "E1 \\/ E2: ");
  while ((it = rgNext(cont))) fprintf(hout, "%c", *it);
  fprintf(hout, "\n");
  cont = destroyOnlyRing(cont);

  if (!(cont = rgMinus(ring, ring2))) goto error;
  fprintf(hout, "E1 - E2: ");
  while ((it = rgNext(cont))) fprintf(hout, "%c", *it);
  fprintf(hout, "\n");
  cont = destroyOnlyRing(cont);

  ring = destroyOnlyRing(ring);
  ring2 = destroyOnlyRing(ring2);
  /*------------------------------------------------------------------*/
  if ((cont = createRing()) == 0) goto error;
  if ((ring2 = createRing()) == 0) goto error;
  rgInit(cont);	/*	: ring initialisation (not needed) */
  fprintf(hout, 
	  "Enter items, one per line.\nEnd the list by EOF (ctl-d)\n");
		
  while (fgets(buffer, BUFSIZ, hin)) {
    if (!strcmp(buffer, "quit\n")) break;

    /*	content acquisition :	*/

    /* Note: do not use malloc here as in unit test we do not 
       use the mediatex malloc allocator. Because mediatex will
       free this string automatically we use an internal allocation
       so as to not disturb memory leaks accounting. */
    //it = (char *)malloc(sizeof(char) * (strlen(buffer) + 1))
    //strcpy(it, buffer);
    it = createString(buffer);

    rgInsert(cont, (void *)it);
    rgHeadInsert(ring2, (void *)it);
  }
  
  rgRewind(cont);
  if ((ring = copyRing(ring, cont,
  		       (void*(*)(void*)) destroyString,
  		       (void*(*)(void*, const void*)) copyString))
      == 0) goto error;

  while ((it = (char *)rgNext(ring)) != (char *)0) {
    /*	forward :	*/
    fprintf(hout, "fw : %s", it);
  }
  
  while ((it = (char *)rgPrevious(ring)) != (char *)0) {
    /*	backward :	*/
    fprintf(hout, "bk : %s", it);
  }

  while ((it = (char *)rgNext(ring2)) != (char *)0) {
    /*	forward :	*/
    fprintf(hout, "bk2 : %s", it);
  }
  
  /* sort */
  fprintf(hout, "sorting ring ...\n");
  if (rgSort(ring, cmpString) == FALSE) goto error;

  fprintf(hout, "first ring (your inputs)\n");
  rgRewind(cont);
  while ((it = (char *)rgNext(cont)) != (char *)0) {
    /*	forward :	*/
    fprintf(hout, "fw : %s", it);
  }

  fprintf(hout, "ring sorted\n");
  rgRewind(ring);
  while ((it = (char *)rgNext(ring)) != (char *)0) {
    /*	forward :	*/
    fprintf(hout, "fw : %s", it);
  }

  ring = destroyRing(ring, (void*(*)(void*)) destroyString);
  cont = destroyRing(cont, (void*(*)(void*)) destroyString);
  ring2 = destroyOnlyRing(ring2);	
  fflush(hout);
  rc = TRUE;
  /************************************************************************/

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
