/*=======================================================================
 * Project: MediaTeX
 * Module : conf
 *
 * Manage mediatex.conf configuration file

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

#ifndef MDTX_CLIENT_CONF_H
#define MDTX_CLIENT_CONF_H 1

#include "mediatex-types.h"

/* API */
int upgradeWrapper(int doCollection, Collection* coll);
int mdtxAddCollection(Collection* coll);
int mdtxDelCollection(char* label);
int mdtxListCollection(int onlyMasterColl);
int mdtxShareSupport(char* sLabel, char* cLabel);
int mdtxWithdrawSupport(char* sLabel, char* cLabel);
int mdtxAddNatClient(char* cLabel, char* proxy);
int mdtxAddNatServer(char* cLabel, char* proxy);
int mdtxDelNatClient(char* cLabel, char* proxy);
int mdtxDelNatServer(char* cLabel, char* proxy);

#endif /* MDTX_CLIENT_CONF_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
