/*=======================================================================
 * Project: MediaTeX
 * Module : extract
 *
 * Manage local cache extraction

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

#ifndef MDTX_SERVER_EXTRACT_H
#define MDTX_SERVER_EXTRACT_H 1

#include "mediatex-types.h"

#define MAX_REMOTE_COPIES 1 // maximum number of scp's threads

typedef enum {
  X_NO_REMOTE_COPY, // no scp extraction (all except extract)
  X_DO_REMOTE_COPY, // do scp extraction (only extract)
  X_GW_REMOTE_COPY  // only do scp extraction from gateway (only extract)
} ExtractScpType;

typedef enum {
  X_NO_LOCAL_COPY, // no cp extraction (only cgi server)
  X_DO_LOCAL_COPY, // others
} ExtractCpType;

typedef struct ExtractData {

  Collection* coll;
  ExtractScpType scpContext; // manage scp rule
  ExtractCpType  cpContext;  // manage cp rule
  RG* toKeeps;         // Archive*
  int found;

} ExtractData;

/* API */

int mdtxCall(int nbArgs, ...);
int extractArchive(ExtractData* data, Archive* archive, int doCp);
int extractDelToKeeps(Collection* coll, RG* toKeeps);
int extractArchives(Collection* coll);

#endif /* MDTX_SERVER_EXTRACT_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
