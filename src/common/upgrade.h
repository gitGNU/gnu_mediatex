/*=======================================================================
 * Version: $Id: upgrade.h,v 1.5 2015/09/13 23:47:35 nroche Exp $
 * Project: MediaTeX
 * Module : upgrade
 *
 * Manage servers.txt upgrade from mediatex.conf and supports.txt

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

#ifndef MDTX_COMMON_UPGRADE_H
#define MDTX_COMMON_UPGRADE_H 1

#include "mediatex-types.h"

int doCheckSupport(Support *supp, char* path);
int scoreSupport(Support* supp, ScoreParam *p);
int scoreLocalImages(Collection* coll);
int upgradeCollection(Collection* collection);

#endif /* MDTX_COMMON_UPGRADE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
