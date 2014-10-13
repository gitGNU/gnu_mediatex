/*=======================================================================
 * Version: $Id: commonHtml.h,v 1.1 2014/10/13 19:38:46 nroche Exp $
 * Project: MediaTeX
 * Module : server to LaTex
 *
 * Server to LaTex serializer

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

#ifndef WRAPPER_ADMSERVER_H
#define WRAPPER_ADMSERVER_H 1

#include "../memory/serverTree.h"
#include <avl.h>

int getItemUri(char* buf, int id, char* suffix);
int getListUri(char* buf, int id);
int htmlMakeDirs(char* path, int max);
int getRelativeUri(char* buf, char* prefix, char* suffix);
int getArchiveUri1(char* buf, char* path, int id);
int getArchiveUri2(char* buf, char* path, int id);
int getContentListUri(char* buf, char* path, int archId, int listId);
int getArchiveUri(char* buf, char* path, Archive* archive);
int getDocumentUri(char* buf, char* path, int id);
int serializeHtmlListBar(Collection* coll, FILE* fd, int n, int N);
int htmlAssoCarac(FILE* fd, AssoCarac* self);

int serializeHtmlCache(Collection* coll);

#endif /* WRAPPER_ADMSERVER_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
