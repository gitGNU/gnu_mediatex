/*=======================================================================
 * Version: $Id: device.h,v 1.2 2014/11/13 16:36:39 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
 *
 * Device informations

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

#ifndef MISC_CHECKSUMS_DEVICE_H
#define MISC_CHECKSUMS_DEVICE_H 1

#include <sys/types.h>   // open
#include <sys/stat.h>    // open
#include <fcntl.h>       // open

char* absolutePath(char* path);
int getDevice(char* inPath, char** outPath);
int isBlockDevice(char* path, int* isB);
int getIsoSize(int fd, off_t *size, 
	       unsigned long int *count, unsigned short int *bs);

#endif /* MISC_CHECKSUMS_DEVICE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
