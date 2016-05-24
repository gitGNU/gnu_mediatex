/*=======================================================================
 * Project: CGIVARS
 * Module : mediatex
 *
 * Get cgi variables

 * Written by:	Ilya G. Goldberg <igg@nih.gov>   11/2003
 *
 *  Copyright (C) 2003 Open Microscopy Environment
 *      Massachusetts Institute of Technology,
 *      National Institutes of Health,
 *      University of Dundee
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 2.1 of the License, or (at your option) any later version.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 *
=======================================================================*/

#ifndef MDTX_MISC_CGI_CGIVARS_H
#define MDTX_MISC_CGI_CGIVARS_H 1

char **getcgivars();
void freecgivars(char **cgivars);

#endif /* MDTX_MISC_CGI__CGIVARS_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */


