/*=======================================================================
 * Version: $Id: extractTree.h,v 1.7 2015/08/31 00:14:52 nroche Exp $
 * Project: MediaTeX
 * Module : extraction tree
 *
 * Extraction producer interface

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 Nicolas Roche
 
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

#ifndef MDTX_MEMORY_EXTRACT_H
#define MDTX_MEMORY_EXTRACT_H 1

#include "mediatex-types.h"

// Container types
typedef enum {UNDEF=0, 
	      INC,                   // for uploads not already burned
	      ISO,                   // CD image
	      CAT,                   // generic way to do multi-volume
	      TGZ, TBZ, AFIO,        // GNU/Linux agregation + compression
	      TAR, CPIO,             // GNU/Linux agregation only
	      GZIP, BZIP,            // GNU/Linux compression only  
	      ZIP, RAR,              // windows agregation + compression
	      ETYPE_MAX} EType;

// note: native multi-volume is only managed for rar archives
//       (this should be done later for the others too)
//       however, split/cat provide a generic way to do that for all

// Record from container association
struct FromAsso {
  Archive*   archive;    // id
  Container* container;  // id
  char*      path;            
};

struct Container {
  // the 2 entries bellow are the primary key
  EType    type;               // container type: IMG, CAT, TGZ, ISO...
  Archive* parent;             // same as the first entry in bellow ring

  RG*      parents;            // ex: RAR source files (Archives)
  AVLTree* childs;             // ex: RAR target files (FromAsso)

  float    score;              // use by mdtx-make
};

struct ExtractTree {
  AVLTree* containers;
  Container* incoming;   // special container with no parent

  float score;           // global score for the collection
};


char* strEType(EType self);
EType getEType(char* label);

FromAsso* createFromAsso(void);
FromAsso* destroyFromAsso(FromAsso* self);
int cmpFromAssoAvl(const void *p1, const void *p2);

Container* createContainer(void);
Container* destroyContainer(Container* self);
int cmpContainerAvl(const void *p1, const void *p2);
int serializeContainer(Collection* coll, Container* self, CvsFile* fd);

int serializeExtractRecord(Archive* self, CvsFile* fd);

ExtractTree* createExtractTree(void);
ExtractTree* destroyExtractTree(ExtractTree* self);
int serializeExtractTree(Collection* coll, CvsFile* fd);

/* API */

int addFromArchive(Collection* coll, Container* container, 
		   Archive* archive);
int delFromArchive(Collection* coll, Container* container, 
		   Archive* archive);

FromAsso* addFromAsso(Collection* coll, Archive* archive, 
		      Container* container, char* path);
int delFromAsso(Collection* coll, FromAsso* self);

Container* addContainer(Collection* coll, EType type, Archive* parent);
int delContainer(Collection* coll, Container* self);

int diseaseExtractTree(Collection* coll);

#endif /* MDTX_MEMORY_EXTRACT_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
