/*=======================================================================
 * Version: $Id: extract.h,v 1.1 2014/10/13 19:39:54 nroche Exp $
 * Project: MediaTeX
 * Module : server/extract
 *
 * Manage local cache extraction

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Nicolas Roche
 
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

#include "../memory/confTree.h"
#include "../memory/recordTree.h"

/* API */


typedef enum {X_MAIN, X_CGI, X_STEP} ExtractType;

typedef struct ExtractData {

  Collection* coll;
  ExtractType context; // UNDEF for global extraction
  RG* toKeeps;  // Archive*
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
