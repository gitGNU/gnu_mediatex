/*=======================================================================
 * Version: $Id: md5sum.h,v 1.4 2015/06/30 17:37:33 nroche Exp $
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

#ifndef MDTX_MISC_MD5SUM_H
#define MDTX_MISC_MD5SUM_H 1

#include "mediatex-types.h"

// Operation supported by API:
typedef enum Md5Opp { 
  MD5_CACHE_ID,       // full computation, no path resolution, no progbar
  MD5_SUPP_ID,        // quick check, path resolution, progbar
  MD5_SUPP_ADD,       // full computation, path resolution, progbar
  MD5_SUPP_CHECK      // full check, path resolution, progbar
} Md5Opp;

// only used by MD5_SUPP_CHECK
typedef enum Md5Rc { 
  MD5_SUCCESS = 0, 
  MD5_ERROR = 1,       // report errors (not bad checks)
  MD5_FALSE_SIZE = 2,
  MD5_FALSE_QUICK = 4, 
  MD5_FALSE_FULL = 8,
  MD5_SYSTEM_MASK = 14
} Md5Rc;   

typedef struct Md5Data {
  char *path;          // path file to work in
  Md5Opp opp;          // operation to do
  off_t size;          // internal data (for progbar)
  char quickMd5sum[MAX_SIZE_HASH + 1];
  char fullMd5sum[MAX_SIZE_HASH + 1];
  Md5Rc rc;            // only used by MD5_SUPP_CHECK
} Md5Data;

typedef struct MdtxProgBar {
  ProgBar bar;
  char* label;
  off_t max;
  off_t cur;
} MdtxProgBar;

int startProgBar(char* label);
void stopProgBar();

int doMd5sum(Md5Data* data);

#endif /* MDTX_MISC_MD5SUM_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
