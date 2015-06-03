/*=======================================================================
 * Version: $Id: have.h,v 1.3 2015/06/03 14:03:56 nroche Exp $
 * Project: MediaTeX
 * Module : server/have
 *
 * Manage extraction from removable device

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

#ifndef MDTX_SERVER_HAVE_H
#define MDTX_SERVER_HAVE_H 1

#include "../memory/recordTree.h"
#include "threads.h"

/* API */

int extractFinaleArchives(RecordTree* recordTree, Connexion* connexion);

#endif /* MDTX_SERVER_HAVE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
