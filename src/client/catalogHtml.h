/*=======================================================================
 * Version: $Id: catalogHtml.h,v 1.1 2014/10/13 19:38:46 nroche Exp $
 * Project: MediaTeX
 * Module : archive catalog's latex serializer
 *
 * Catalog latex serializer interface

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

#ifndef WRAPPER_ADMCATALOG_H
#define WRAPPER_ADMCATALOG_H 1

#include  "../memory/catalogTree.h"

/* API */

int getHumanUri(char* buf, char* path, int id);
int getRoleListUri(char* buf, char* path, int roleId, int listId);
int getCateListUri(char* buf, char* path, int cateId, int listId);
int serializeHtmlIndex(Collection* coll);

#endif /* WRAPPER_ADMCATALOG_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
