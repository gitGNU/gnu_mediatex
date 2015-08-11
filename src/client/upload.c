
/*=======================================================================
 * Version: $Id: upload.c,v 1.4 2015/08/11 18:14:23 nroche Exp $
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
 * Description: parse uploaded catalog file
 * Synopsis   : static int mdtxUploadCatalog(
 *                                        Collection* upload, char* path)
 * Input      : Collection* upload: collection used to parse upload
 *              char* path: input catalog file to parse
 * Output     : TRUE on success
 =======================================================================*/
static int 
mdtxUploadCatalog(Collection* upload, char* path)
{ 
  int rc = FALSE;

  checkCollection(upload);
  checkLabel(path);
  logMain(LOG_DEBUG, "mdtxUploadCatalog");
  
  if (!(upload->catalogTree = createCatalogTree())) goto error;
  if (!(upload->catalogDB = createString(path))) goto error;

  if (!parseCatalogFile(upload, path)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUploadCatalog fails");
  }
  upload->catalogDB = destroyString(upload->catalogDB);
  return rc;
}

/*=======================================================================
 * Function   : mdtxUploadCatalog
 * Description: concatenate uploaded catalog metadata
 * Synopsis   : static int mdtxConcatCatalog(
                                    Collection* coll, Collection *upload)
 * Input      : Collection* coll: target
 *              Collection* upload: source
 * Output     : TRUE on success
 =======================================================================*/
static int 
mdtxConcatCatalog(Collection* coll, Collection *upload)
{ 
  int rc = FALSE;
  CvsFile fd = {0, 0, 0, FALSE, 0, cvsCatOpen, cvsCatPrint};

  logMain(LOG_DEBUG, "mdtxConcatCatalog");

  upload->catalogDB = coll->catalogDB;
  if (!(serializeCatalogTree(upload, &fd))) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxConcatCatalog fails");
  }
  upload->catalogDB = 0;
  return rc;
}


/*=======================================================================
 * Function   : mdtxUploadExtract
 * Description: parse uploaded extract file
 * Synopsis   : static int mdtxUploadExtract(
 *                                        Collection* upload, char* path)
 * Input      : Collection* upload: collection used to parse upload
 *              char* path: input extract file to parse
 * Output     : TRUE on success
 =======================================================================*/
static int 
mdtxUploadExtract(Collection* upload, char* path)
{ 
  int rc = FALSE;

  checkCollection(upload);
  checkLabel(path);
  logMain(LOG_DEBUG, "mdtxUploadExtract");
  
  if (!(upload->extractTree = createExtractTree())) goto error;
  if (!(upload->extractDB = createString(path))) goto error;

  if (!parseExtractFile(upload, path)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUploadExtract fails");
  }
  upload->extractDB = destroyString(upload->extractDB);
  return rc;
}


/*=======================================================================
 * Function   : mdtxUploadExtract
 * Description: concatenate uploaded extract metadata
 * Synopsis   : static int mdtxConcatExtract(
                                    Collection* coll, Collection *upload)
 * Input      : Collection* coll: target
 *              Collection* upload: source
 * Output     : TRUE on success
 =======================================================================*/
static int 
mdtxConcatExtract(Collection* coll, Collection *upload)
{ 
  int rc = FALSE;
  CvsFile fd = {0, 0, 0, FALSE, 0, cvsCatOpen, cvsCatPrint};

  logMain(LOG_DEBUG, "mdtxConcatExtract");

  upload->extractDB = coll->extractDB;
  if (!(serializeExtractTree(upload, &fd))) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxConcatExtract fails");
  }
  upload->extractDB = 0;
  return rc;
}


/*=======================================================================
 * Function   : mdtxUpload
 * Description: 
 * Synopsis   : 
 * Input      : char* label: related collection
 *              char* catalog: catalog metadata file to upload
 *              char* extract: extract metadata file to upload
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxUpload(char* label, char* catalog, char* extract)
{ 
  int rc = FALSE;
  Collection *coll = 0;
  Collection *upload = 0;

  logMain(LOG_DEBUG, "mdtxUpload");

  if (!allowedUser(env.confLabel)) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;

  // parser uploaded metadata files into a new collection
  if (!(upload = addCollection("_upload_"))) goto error;
  if (!(upload->masterLabel = createString(coll->masterUser))) goto error;
  if (!(upload->user = createString(coll->user))) goto error;
  strcpy(upload->masterHost, "localhost");

  if (catalog) {
    if (!mdtxUploadCatalog(upload, catalog)) goto error;
  }
  if (extract) {
    if (!mdtxUploadExtract(upload, extract)) goto error;
  }

  // TODO some checks
  
  // concatenate metadata to the true collection
  upload->memoryState |= EXPANDED;
  if (catalog) {
    if (!mdtxConcatCatalog(coll, upload)) goto error;
  }
  if (extract) {
    if (!mdtxConcatExtract(coll, upload)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "mdtxUpload fails");
  }
  delCollection(upload);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
