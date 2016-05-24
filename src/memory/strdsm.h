/*=======================================================================
 * Project: MediaTeX
 * Module : strdsm
 *
 * STRing Data Structure Management interface
 * This file was originally written by Peter Felecan under GNU GPL

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Felecan Peter

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

#ifndef MDTX_MEMORY_STRDSM_H
#define MDTX_MEMORY_STRDSM_H 1

char* createString(const char* content);
char* createSizedString(size_t size, const char* content);
char* copyString(char* destination, const char* source);
char* copySizedString(size_t size, char* destination, const char* source);
char* catString(char* prefix, const char* suffix);
char* destroyString(char* self);

inline int isEmptyString(const char* content);
int cmpString(const void *p1, const void *p2);

#endif /* MDTX_MEMORY_STRDSM_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
