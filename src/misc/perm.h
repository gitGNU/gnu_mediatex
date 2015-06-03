/*=======================================================================
 * Version: $Id: perm.h,v 1.3 2015/06/03 14:03:46 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
 *
 * md5sum computation

 MediaTex is an Electronic Records Management System
 Copyright (C) 2012  Nicolas Roche

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

#ifndef MISC_PERM_H
#define MISC_PERM_H 1

#include "../mediatex.h"

#include <time.h>

time_t currentTime();
int checkDirectory(char* path, char* user, char* group, mode_t mode);

#endif /* MISC_PERM_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
