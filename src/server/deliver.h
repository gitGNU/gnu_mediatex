/*=======================================================================
 * Version: $Id: deliver.h,v 1.2 2014/11/13 16:37:08 nroche Exp $
 * Project: MediaTeX
 * Module : wrapper/deliver
 *
 * Manage delivering mail

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

#ifndef MDTX_WRAPPER_DELIVER_H
#define MDTX_WRAPPER_DELIVER_H 1

//#include "../mediatex.h"

/* API */

int deliverMail(Collection* coll, Archive* record);
int deliverMails(Collection* coll);

#endif /* MDTX_WRAPPER_DELIVER_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
