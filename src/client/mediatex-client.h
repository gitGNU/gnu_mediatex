/*=======================================================================
 * Version: $Id: mediatex-client.h,v 1.2 2015/08/10 11:07:45 nroche Exp $
 * Project: MediaTex
 * Module : headers
 *
 * Client headers

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

#ifndef MDTX_CLIENT_H
#define MDTX_CLIENT_H 1

#include "client/serv.h"
#include "client/conf.h"
#include "client/supp.h"
#include "client/motd.h"
#include "client/commonHtml.h"
#include "client/catalogHtml.h"
#include "client/extractHtml.h"
#include "client/misc.h"
#include "client/upload.h"

// parsers
int getCommandLine(int argc, char** argv, int optind);
int parseShellQuery(int argc, char** argv, int optind);

#endif /* MDTX_CLIENT_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
