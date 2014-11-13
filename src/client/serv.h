/*=======================================================================
 * Version: $Id: serv.h,v 1.2 2014/11/13 16:36:19 nroche Exp $
 * Project: MediaTeX
 * Module : wrapper/serv
 *
 * Manage servers.txt modifications

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

#ifndef MDTX_WRAPPER_UPGRADE_H
#define MDTX_WRAPPER_UPGRADE_H 1

// to use to ptrevent concurrent wrappers running together
int clientWriteLock(); 
int clientWriteUnlock();

// the 2 firsts may also be used for catalog.txt and extract.txt
int mdtxUpdate(char* label);
int mdtxCommit(char* label);
int mdtxUpgrade(char* label);

int addKey(char* label, char* path);
int delKey(char* label, char* path);

#endif /* MDTX_WRAPPER_UPGRADE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
