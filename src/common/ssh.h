/*=======================================================================
 * Version: $Id: ssh.h,v 1.1 2014/10/13 19:39:00 nroche Exp $
 * Project: MediaTeX
 * Module : wrapper/conf
 *
 * SSH producer interface

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

#ifndef MDTX_WRAPPER_SSH_H
#define MDTX_WRAPPER_SSH_H 1

/* API */
/*
int serializeAuthKeys(Collection* coll);
int serializeSshConfig(Collection* coll);
int serializeKnownHosts(Collection* coll);
*/
int upgradeSshConfiguration(Collection* coll);

#endif /* MDTX_WRAPPER_SSH_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
