/*=======================================================================
 * Version: $Id: motd.h,v 1.4 2015/06/30 17:37:25 nroche Exp $
 * Project: MediaTeX
 * Module : motd
 *
 * Manage message of the day

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

#ifndef MDTX_CLIENT_MOTD_H
#define MDTX_CLIENT_MOTD_H 1

#include "mediatex-types.h"

typedef struct MotdData {

  Collection* coll;
  RG* outArchives;
  int found;

} MotdData;

/* API */
int updateMotd();

#endif /* MDTX_CLIENT_MOTD_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
