/*=======================================================================
 * Version: $Id: supp.h,v 1.5 2015/08/27 10:51:52 nroche Exp $
 * Project: MediaTeX
 * Module : supp
 *
 * Manage local supports

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

#ifndef MDTX_CLIENT_SUPP_H
#define MDTX_CLIENT_SUPP_H 1

#include "mediatex-types.h"

int checkValidity(Support* supp, int* obsolete);
int mdtxMount(char* iso, char* target);
int mdtxUmount(char* target);

/* API */

int mdtxLsSupport();
int mdtxUpdateSupport(char* label, char* status);
int mdtxAddSupport(char* label, char* path);
int mdtxAddFile(char* path);
int mdtxHaveSupport(char* label, char* path);
int mdtxDelSupport(char* label);

#endif /* MDTX_CLIENT_SUPP_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
