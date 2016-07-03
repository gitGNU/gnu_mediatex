/*=======================================================================
 * Project: MediaTeX
 * Module : setuid
 *
 * setuid API

 This file comes from:
 http://www.gnu.org/software/libc/manual/html_node/Setuid-Program-Example.html

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

#ifndef MDTX_MISC_SETUID_H
#define MDTX_MISC_SETUID_H 

#include "mediatex-types.h"
#include <pwd.h>
#include <grp.h>

int getPasswdLine (char* label, uid_t uid, 
		   struct passwd* pw, char** buffer);
int getGroupLine (char* label, gid_t gid, 
		  struct group* gr, char** buffer);

int undoSeteuid (void);
int allowedUser (char* label, int* isAllowed, int optUid);
int becomeUser (char* label, int doCheck);
int logoutUser (int uid);

#endif /* MDTX_MISC_SETUID_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

