/*=======================================================================
 * Version: $Id: cypher.h,v 1.4 2015/06/30 17:37:31 nroche Exp $
 * Project: MediaTeX
 * Module : cypher
 *
 * aes encryption

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 Nicolas Roche

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

#ifndef MDTX_MISC_CYPHER_H
#define MDTX_MISC_CYPHER_H 1

#include "mediatex.h"
#include <openssl/aes.h>

typedef enum {NO_WAY, ENCRYPT, DECRYPT} MDTX_AES_WAY;

typedef struct AESData {
  AES_KEY aesKey;
  unsigned char mBlock[MAX_SIZE_AES+1];
  unsigned char cBlock[MAX_SIZE_AES+1];

  MDTX_AES_WAY way;
  int index;

  // API
  int doCypher;  // if FALSE we juste copy the data (for headers)
  int fd;  /* output file descriptor to use to read/write 
	      depending on way selected :
	      - ENCRYPT: aesPrint and aesFlush will output in fd
	      - DECRYPT: aesInit will read from fr */		 
} AESData;

int aesInit(AESData* data, char key[MAX_SIZE_AES+1], MDTX_AES_WAY way);
int aesPrint(AESData* data, const char* format, ...);
int aesFlush(AESData* data);
int aesInput(AESData* data, char* buf, int *result, int maxsize);

#endif /* MDTX_MISC_CYPHER_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
