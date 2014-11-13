/*=======================================================================
 * Version: $Id: misc.h,v 1.2 2014/11/13 16:36:19 nroche Exp $
 * Project: MediaTeX
 * Module : wrapper/make
 *
 * Front-end for the html sub modules

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

#ifndef MDTX_WRAPPER_MAKE_H
#define MDTX_WRAPPER_MAKE_H 1

/* API */

int mdtxInit();
int mdtxRemove();
int mdtxPurge();
int mdtxMake(char* label);
int mdtxClean(char* label);
int mdtxAddUser(char* user);
int mdtxDelUser(char* user);
int mdtxBind();
int mdtxUnbind();
int mdtxSu(char* label);
int mdtxScp(char* label, char* fingerPrint, char* target);
int mdtxUploadFile(char* label, char* path);

#endif /* MDTX_WRAPPER_MAKE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
