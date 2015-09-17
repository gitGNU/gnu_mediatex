/*=======================================================================
 * Version: $Id: md5sum.h,v 1.5 2015/09/17 18:53:48 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
 *
 * md5sum computation

 MediaTex is an Electronic Records Management System
 Copyright (C) 2012  Nicolas Roche

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

#ifndef MDTX_MISC_CHECKSUM_H
#define MDTX_MISC_CHECKSUM_H 1

#include "mediatex-types.h"

// Operation supported by API:
typedef enum CheckOpp { 
  CHECK_CACHE_ID,       // full computation, no path resolution, no progbar
  CHECK_SUPP_ID,        // quick check, path resolution, progbar
  CHECK_SUPP_ADD,       // full computation, path resolution, progbar
  CHECK_SUPP_CHECK      // full check, path resolution, progbar
} CheckOpp;

// only used by CHECK_SUPP_CHECK
typedef enum CheckRc { 
  CHECK_SUCCESS = 0, 
  CHECK_ERROR = 1,      // report errors (not bad checks)
  CHECK_FALSE_SIZE = 2,
  CHECK_FALSE_QUICK = 4, 
  CHECK_FALSE_FULL = 8,
  CHECK_SYSTEM_MASK = 14
} CheckRc;   

typedef struct CheckData {
  char *path;           // path file to work in
  CheckOpp opp;         // operation to do
  off_t size;           // internal data (for progbar)
  char quickMd5sum[MAX_SIZE_MD5 + 1];
  char fullMd5sum[MAX_SIZE_MD5 + 1];
  char quickShasum[MAX_SIZE_SHA + 1];
  char fullShasum[MAX_SIZE_SHA + 1];
  CheckRc rc;           // only used by CHECK_SUPP_CHECK
} CheckData;

typedef struct MdtxProgBar {
  ProgBar bar;
  char* label;
  off_t max;
  off_t cur;
} MdtxProgBar;

int startProgBar(char* label);
void stopProgBar();

int doChecksum(CheckData* data);

#endif /* MDTX_MISC_CHECKSUM_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
