/*=======================================================================
 * Project: MediaTeX
 * Module : catalogHtml
 *
 * HTML catalog serializer

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

#ifndef MDTX_CLIENT_ADMCATALOG_H
#define MDTX_CLIENT_ADMCATALOG_H 1

#include  "mediatex-types.h"

/* API */

int getHumanUri(char* buf, char* path, int id);
int getRoleListUri(char* buf, char* path, int roleId, int listId);
int getCateListUri(char* buf, char* path, int cateId, int listId);
int serializeHtmlIndex(Collection* coll);

#endif /* MDTX_CLIENT_ADMCATALOG_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
