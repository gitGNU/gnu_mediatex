/*=======================================================================
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

#include "mediatex-config.h"

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
    logMemory(LOG_WARNING, "create null String");
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

  if(self) {
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

  if(destination) {
    free(destination);
    destination = 0;
  }

  if(source) {
    if ((destination 
	 = (char*)malloc(sizeof(char) * (strlen(source) + 1)))
	== 0) {
      logMemory(LOG_ERR, "malloc: cannot allocate String");
    }
    
    if(destination) {
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

  if(destination) {
    free(destination);
    destination = 0;
  }

  if(source) {
    if ((destination 
	 = (char*)malloc(sizeof(char) * (size + 1))) 
	== 0) {
      logMemory(LOG_ERR, "%", "malloc: cannot allocate String");
    }
    
    if(destination) {
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

  if(suffix) {
    int size = strlen(suffix);
    
    if(prefix) {
      size += strlen(prefix);
    }
    
    if ((rc 
	 = (char*)malloc(sizeof(char) * (size + 1))) 
	== 0) {
      logMemory(LOG_ERR, "malloc: cannot allocate String");
    }
    
    if(rc) {
      /* on Solaris 10 u2 sparc, malloc doesn't sets memory to
	 nil */
      memset(rc, 0, (size + 1));
      
      if(prefix) {
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

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
