/*=======================================================================
 * Version: $Id: utFunc.h,v 1.2 2014/11/13 16:36:34 nroche Exp $
 * Project: MediaTeX
 * Module : utFunc
 *
 * unit test functions for memory modules

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

#ifndef MEMORY_UTFUNC_H
#define MEMORY_UTFUNC_H 1

#include "confTree.h"

int createExempleConfiguration();
int createExempleSupportTree();
int createExempleCatalogTree(Collection* coll);
int createExempleExtractTree(Collection* coll);
int createExempleServerTree(Collection* coll);
int createExempleRecordTree(Collection* coll);

#endif /* MEMORY_UTFUNC_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
