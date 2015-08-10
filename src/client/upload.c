
/*=======================================================================
 * Version: $Id: upload.c,v 1.2 2015/08/10 12:24:27 nroche Exp $
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
 * Function   : mdtxUploadCatalog
 * Description: 
 * Synopsis   : 
 * Input      : Collection* coll: collection used to parse input file
 *              char* path: input catalog file to parse
 * Output     : TRUE on success
 =======================================================================*/
static int 
mdtxUploadCatalog(Collection* coll, char* path)
{ 
  int rc = FALSE;
  Configuration* conf = 0;

  checkCollection(coll);
  checkLabel(coll->label);
  logMain(LOG_DEBUG, "mdtxUploadCatalog");
  if (!(conf = getConfiguration())) goto error;
  
  if (!(coll->catalogTree = createCatalogTree())) goto error;
  if (!(coll->catalogDB = createString(path))) goto error;
  if (!loadCollection(coll, CTLG)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUploadCatalog fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : mdtxUpload
 * Description: 
 * Synopsis   : 
 * Input      : char* label: related collection
 *              char* catalog: catalog metadata file to upload
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxUpload(char* label, char* catalog)
{ 
  int rc = FALSE;
  Configuration* conf = 0;
  Collection *upload = 0;

  checkLabel(label);
  checkLabel(catalog);
  logMain(LOG_DEBUG, "mdtxUpload");
  if (!(conf = getConfiguration())) goto error;
  
  //data.dryRun = env.dryRun;
  //env.dryRun = FALSE;
  if (!(upload = addCollection("__upload"))) goto error;
  if (!mdtxUploadCatalog(upload, catalog)) goto error;
  
  // serialize the resulting mediatex metadata using basename prefix
  /* if (sprintf(tmp, "stage3/%s_ext", data.basename) <= 0) goto error; */
  /* if (!(data.coll->extractDB = createString(tmp))) goto error; */
  /* if (sprintf(tmp, "stage3/%s_cat", data.basename) <= 0) goto error; */
  /* if (!(data.coll->catalogDB = createString(tmp))) goto error; */
  /* data.coll->memoryState |= EXPANDED; */
  /* if (!(data.coll->user = createString("mdtx"))) goto error; */
  /* env.noRegression = TRUE; */
  /* if (!(serializeExtractTree(data.coll))) goto error; */
  /* if (!(serializeCatalogTree(data.coll))) goto error; */


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
