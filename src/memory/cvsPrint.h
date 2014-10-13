/*=======================================================================
 * Version: $Id: cvsPrint.h,v 1.1 2014/10/13 19:39:09 nroche Exp $
 * Project: MediaTeX
 * Module : archive tree
 *
 * cvs files producer interface

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

#ifndef MEMORY_ADMCVSPRINT_H
#define MEMORY_ADMCVSPRINT_H 1

/* API */

typedef struct CvsFile
{
  char     *path;
  int      nb;
  FILE     *fd;
  int      doCut;
  long int offset;
} CvsFile;

int cvsOpenFile(CvsFile* fd);
int cvsPrint(CvsFile* fd, const char* format, ...);
int cvsCloseFile(CvsFile* fd);

#endif /* MEMORY_CVSPRINT_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
