/*=======================================================================
 * Version: $Id: openClose.h,v 1.3 2015/06/03 14:03:34 nroche Exp $
 * Project: MediaTeX
 * Module : bus/openClose
 
 * Manage data files

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

#ifndef MDTX_BUS_OPEN_CLOSE_H
#define MDTX_BUS_OPEN_CLOSE_H 1

int callUpdate(char* user);
int callCommit(char* user, char* signature1, char* signature2);

int loadConfiguration(int confFiles); // logical OR on confFiles
int loadRecords(Collection* coll);
int saveRecords(Collection* coll);

int loadCollection(Collection* coll, int collFiles);
int wasModifiedCollection(Collection* coll, int collFiles);
int releaseCollection(Collection* coll, int collFiles);

int saveConfiguration();
int saveCollection(Collection* coll, int collFiles);
int clientSaveAll();
int serverSaveAll();

int diseaseCollection(Collection* coll, int collFiles);
int clientDiseaseAll();
int serverDiseaseAll();

Collection* mdtxGetCollection(char* label);
Support* mdtxGetSupport(char* label);

int clientLoop(int (*callback)(char*));
int serverLoop(int (*callback)(Collection*));

#endif /* MDTX_BUS_OPEN_CLOSE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
