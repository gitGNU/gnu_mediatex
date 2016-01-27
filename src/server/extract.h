/*=======================================================================
 * Version: $Id: extract.h,v 1.5 2015/08/23 23:39:17 nroche Exp $
 * Project: MediaTeX
 * Module : extract
 *
 * Manage local cache extraction

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

#ifndef MDTX_SERVER_EXTRACT_H
#define MDTX_SERVER_EXTRACT_H 1

#include "mediatex-types.h"

/* API */

#define MAX_REMOTE_COPIES 1

/*
typedef enum {
  X_MAIN, // do scp extraction if locally needed
  X_CGI,  // no scp extraction
  X_STEP  // only do scp extraction on gateway if remotely needed
} ExtractType;
*/

typedef enum {
  X_NO_REMOTE_COPY, // no scp extraction 
  X_DO_REMOTE_COPY, // do scp extraction (if locally needed)
  X_GW_REMOTE_COPY  // only do scp extraction from gateway (default)
} ExtractType;

typedef struct ExtractData {

  Collection* coll;
  ExtractType context; // manage scp rule
  RG* toKeeps;         // Archive*
  int found;
  Archive* target;

} ExtractData;

int mdtxCall(int nbArgs, ...);
int extractArchive(ExtractData* data, Archive* archive);
int extractArchives(Collection* coll);

#endif /* MDTX_SERVER_EXTRACT_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
