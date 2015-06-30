/*=======================================================================
 * Version: $Id: cvsPrint.h,v 1.4 2015/06/30 17:37:29 nroche Exp $
 * Project: MediaTeX
 * Module : cvs print
 *
 * cvs files producer interface

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

#ifndef MISC_MEMORY_CVSPRINT_H
#define MISC_MEMORY_CVSPRINT_H 1

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

#endif /* MISC_MEMORY_CVSPRINT_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
