/*=======================================================================
 * Version: $Id: strdsm.c,v 1.3 2015/06/03 14:03:40 nroche Exp $
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

#include <string.h>

#include "strdsm.h"
#include "../misc/alloc.h"
#include "../misc/log.h"

/*=======================================================================
 * Function   : createString (strdsm) [MediaTeX]
 * Description: Create a string with a given content
 * Synopsis   : char* createString(char* content)
 * Input      : char* content = the content of the string to create.
 * Output     : Address of the created string or nil if the creation
 *              fails.
 =======================================================================*/
char* 
createString(const char* content)
{
  char* rc = 0;

  if (content == 0) {
    logMemory(LOG_WARNING, "%s", "create null String");
  }

  if ((rc = copyString(rc, content)) == 0) {
    logMemory(LOG_ERR, "malloc: cannot create String: \"%s\"", content);
  }

  return(rc);
}

/*=======================================================================
 * Function   : createSizedString (strdsm) [MediaTeX]
 * Description: Create a string with a given size content.
 * Synopsis   : char* createSizedString(char* content)
 * Input      : size_t size = size of the string;
 *              char* content = the content of the string to create;
 *              only the first size characters are used to initialize
 *              the string.
 * Output     : Address of the created string or nil if the creation
 *              fails.
 =======================================================================*/
char* 
createSizedString(size_t size, const char* content)
{
  char* rc = 0;

  if (content == 0) {
    logMemory(LOG_WARNING, "create null sized String: length=%i", size);
  }

  if ((rc = copySizedString(size, rc, content)) == 0) {
    logMemory(LOG_ERR, "malloc: cannot create sized String: \"%s\" length=%i",
	    content, size);
  }
	
  return rc;
}

/*=======================================================================
 * Function   : destroyString (strdsm) [MediaTeX]
 * Description: Destroy a string.
 * Synopsis   : char* destroyString(char* self)
 * Input      : char* self = address of the string to destroy.
 * Output     : Nil address of a string.
 =======================================================================*/
char* 
destroyString(char* self)
{
  char* rc = 0;

  if(self != 0) {
    free(self);
  }
	
  return(rc);
}

/*=======================================================================
 * Function   : copyString (strdsm) [MediaTeX]
 * Description: Copy a source string into a destination string. The
 *              destination string's memory, if any, is released and a
 *              new allocation is used.
 * Synopsis   : char* copyString(char* destination, char* source)
 * Input      : char* destination = the destination string
 *              char* source = the source string
 * Output     : Address of the destination string. It changes at each
 *              call.
 =======================================================================*/
char* 
copyString(char* destination, const char* source)
{
  char* rc = 0;

  if(destination != 0) {
    free(destination);
    destination = 0;
  }

  if(source != 0) {
    if ((destination 
	 = (char*)malloc(sizeof(char) * (strlen(source) + 1)))
	== 0) {
      logMemory(LOG_ERR, "%s", "malloc: cannot allocate String");
    }
    
    if(destination != 0) {
      strcpy(destination, source);
    }
  }

  rc = destination;	
  return(rc);
}

/*=======================================================================
 * Function   : copySizedStringString (strdsm) [MediaTeX]
 * Description: Copy a source string into a destination string. The
 *              destination string's memory, if any, is released and a
 *              new allocation is used. Only the first size characters
 *              are copied.
 * Synopsis   : char* copySizedStringString(size_t size, 
 *                                      char* destination, char* source)
 * Input      : size_t size = the destination string's size
 *              char* destination = the destination string
 *              char* source = the source string
 * Output     : Address of the destination string. It changes at each
 *              call.
 =======================================================================*/
char* 
copySizedString(size_t size, char* destination, const char* source)
{
  char* rc = 0;

  if(destination != 0) {
    free(destination);
    destination = 0;
  }

  if(source != 0) {
    if ((destination 
	 = (char*)malloc(sizeof(char) * (size + 1))) 
	== 0) {
      logMemory(LOG_ERR, "%", "malloc: cannot allocate String");
    }
    
    if(destination != 0) {
      size_t eos = strlen(source);
      
      eos = ((eos <= size) ? eos : size);
      strncpy(destination, source, size);
      *(destination + eos) = '\0';
    }
  }

  rc = destination;	
  return(rc);
}

/*=======================================================================
 * Function   : catString (strdsm) [MediaTeX]
 * Description: Concatenate a suffix to a prefix string.
 * Synopsis   : char* catString(char* prefix, const char* suffix)
 * Input      : char* prefix = the concatenation's prefix
 *              char* suffix = the concatenation's suffix
 * Output     : The address of the concatenated string. Note that the
 *              concatenated string is the reallocated prefix.
 =======================================================================*/
char* 
catString(char* prefix, const char* suffix)
{
  char* rc = prefix;

  if(suffix != 0) {
    int size = strlen(suffix);
    
    if(prefix != 0) {
      size += strlen(prefix);
    }
    
    if ((rc 
	 = (char*)malloc(sizeof(char) * (size + 1))) 
	== 0) {
      logMemory(LOG_ERR, "%s", "malloc: cannot allocate String");
    }
    
    if(rc != 0) {
      /* on Solaris 10 u2 sparc, malloc doesn't sets memory to
	 nil */
      memset(rc, 0, (size + 1));
      
      if(prefix != 0) {
	strcat(rc, prefix);
	prefix = destroyString(prefix);
      }
      
      strcat(rc, suffix);
    }
  }
	
  return(rc);
}

/*=======================================================================
 * Function   : isEmptyString
 * Author     : Nicolas ROCHE
 * modif      :
 * Description: test if a string is empty
 * Synopsis   : int isEmptyString(const char* content);
 * Input      : content: string to test
 * Output     : TRUE if empty
 =======================================================================*/
inline int isEmptyString(const char* content)
{
  return (content == 0 || *content == (char)0)?TRUE:FALSE;
}

/*=======================================================================
 * Function   : cmpString
 * Author     : Nicolas ROCHE
 * modif      :
 * Description: compare 2 strings
 * Synopsis   : int cmpString(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2 : pointers on objects
 * Output     : like strcmp
 =======================================================================*/
int 
cmpString(const void *p1, const void *p2)
{
  /* p1 and p2 are pointers on &items
   * and items are suposed to be char* 
   */
  
  char* w1 = *((char**)p1);
  char* w2 = *((char**)p2);
  
  return strcmp(w1, w2);
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
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
 
#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
