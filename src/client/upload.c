
/*=======================================================================
 * Version: $Id: upload.c,v 1.3 2015/08/11 11:59:33 nroche Exp $
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

  checkCollection(coll);
  checkLabel(coll->label);
  logMain(LOG_DEBUG, "mdtxUploadCatalog");
  
  if (!(coll->catalogTree = createCatalogTree())) goto error;
  if (!(coll->catalogDB = createString(path))) goto error;

  if (!parseCatalogFile(coll, path)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUploadCatalog fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : mdtxUploadCatalog
 * Description: 
 * Synopsis   : 
 * Input      : Collection* coll: collection used to parse input file
 *              char* path: input catalog file to parse
 * Output     : TRUE on success
 =======================================================================*/
static int 
mdtxConcatCatalog(Collection* coll)
{ 
  int rc = FALSE;
  //Configuration* conf = 0;

  checkCollection(coll);
  logMain(LOG_DEBUG, "mdtxConcatCatalog");
  //if (!(conf = getConfiguration())) goto error;

  //data.dryRun = env.dryRun;
  //env.dryRun = FALSE;

  
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
    logMain(LOG_ERR, "mdtxConcatCatalog fails");
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
  //Configuration* conf = 0;
  Collection *coll = 0;
  Collection *upload = 0;

  logMain(LOG_DEBUG, "mdtxUpload");

  if (!allowedUser(env.confLabel)) goto error;
  //if (!(conf = getConfiguration())) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;

  // build a temporary collection to parse uploaded metadata
  if (!(upload = addCollection("__upload"))) goto error;
  strcpy(upload->label, "__upload");
  if (!(upload->masterLabel = createString(env.confLabel))) goto error;
  strcpy(upload->masterHost, "localhost");
  //upload->fileState[iEXTR] = DISEASED;
  //upload->fileState[iCTLG] = DISEASED;
  upload->memoryState |= EXPANDED;

  // parse uploaded catalog  metadata
  if (catalog) {
    if (!mdtxUploadCatalog(upload, catalog)) goto error;
  }
  
  if (catalog) {
    if (!mdtxConcatCatalog(coll)) goto error;
  }

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
