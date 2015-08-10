
/*=======================================================================
 * Version: $Id: upload.c,v 1.1 2015/08/10 11:07:45 nroche Exp $
 * Project: MediaTeX
 * Module : upload
 *
 * Manage upload

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

#include "mediatex-config.h"
#include "client/mediatex-client.h"


/*=======================================================================
 * Function   : mdtxUpload
 * Description: 
 * Synopsis   : 
 * Input      : 
 * Output     : 
 =======================================================================*/
int 
mdtxUpload(Collection* coll)
{ 
  int rc = FALSE;
  Configuration* conf = 0;

  checkCollection(coll);
  checkLabel(coll->label);
  logMain(LOG_DEBUG, "mdtxUpload");
  if (!(conf = getConfiguration())) goto error;
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUpload fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
