
/*=======================================================================
 * Version: $Id: upload.c,v 1.5 2015/08/12 12:07:27 nroche Exp $
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
 * Function   : uploadCatalog
 * Description: parse uploaded catalog file
 * Synopsis   : static int uploadCatalog(Collection* upload, char* path)
 * Input      : Collection* upload: collection used to parse upload
 *              char* path: input catalog file to parse
 * Output     : TRUE on success
 =======================================================================*/
static int 
uploadCatalog(Collection* upload, char* path)
{ 
  int rc = FALSE;

  checkCollection(upload);
  checkLabel(path);
  logMain(LOG_DEBUG, "uploadCatalog");
  
  if (!(upload->catalogTree = createCatalogTree())) goto error;
  if (!(upload->catalogDB = createString(path))) goto error;

  if (!parseCatalogFile(upload, path)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "uploadCatalog fails");
  }
  upload->catalogDB = destroyString(upload->catalogDB);
  return rc;
}

/*=======================================================================
 * Function   : uploadExtract
 * Description: parse uploaded extract file
 * Synopsis   : static int uploadExtract(Collection* upload, char* path)
 * Input      : Collection* upload: collection used to parse upload
 *              char* path: input extract file to parse
 * Output     : TRUE on success
 =======================================================================*/
static int 
uploadExtract(Collection* upload, char* path)
{ 
  int rc = FALSE;

  checkCollection(upload);
  checkLabel(path);
  logMain(LOG_DEBUG, "uploadExtract");
  
  if (!(upload->extractTree = createExtractTree())) goto error;
  if (!(upload->extractDB = createString(path))) goto error;

  if (!parseExtractFile(upload, path)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "uploadExtract fails");
  }
  upload->extractDB = destroyString(upload->extractDB);
  return rc;
}

/*=======================================================================
 * Function   : uploadContent
 * Description: add uploaded content file to extract metadata
 * Synopsis   : static Archive* uploadContent(
 *                                        Collection* upload, char* path)
 * Input      : Collection* upload: collection used to parse upload
 *              char* path: input file to upload
 * Output     : Archive object on success ; NULL on error
 =======================================================================*/
static Archive* 
uploadContent(Collection* upload, char* path)
{ 
  Archive* rc = 0;
  struct stat statBuffer;
  Md5Data md5; 
  Archive* archive = 0;
  time_t time = 0;
  struct tm date;
  char dateString[32];
  Container* container = 0;

  checkCollection(upload);
  checkLabel(path);
  logMain(LOG_DEBUG, "uploadContent");

  if (!upload->extractTree) {
    if (!(upload->extractTree = createExtractTree())) goto error;
  }
  
  // get file attributes (size)
  if (stat(path, &statBuffer)) {
    logMain(LOG_ERR, "status error on %s: %s", path, strerror(errno));
    goto error;
  }

  // compute hash
  memset(&md5, 0, sizeof(Md5Data));
  md5.path = path;
  md5.size = statBuffer.st_size;
  md5.opp = MD5_CACHE_ID;
  if (!doMd5sum(&md5)) goto error;

  // archive to upload
  if (!(archive = addArchive(upload, md5.fullMd5sum, statBuffer.st_size)))
    goto error;

  // add extraction rule
  if (!(time = currentTime())) goto error;
  if (localtime_r(&time, &date) == (struct tm*)0) {
    logMemory(LOG_ERR, "localtime_r returns on error");
    goto error;
  }

  sprintf(dateString, "%04i-%02i-%02i,%02i:%02i:%02i", 
	  date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	  date.tm_hour, date.tm_min, date.tm_sec);

  if (!(container = upload->extractTree->incoming)) goto error;
  if (!(addFromAsso(upload, archive, container, dateString))) 
    goto error;

  rc = archive;
  archive = 0;
 error:
  if (!rc) {
    logMain(LOG_ERR, "uploadExtract fails");
  }
  delArchive(upload, archive);
  return rc;
}


/*=======================================================================
 * Function   : isCatalogRefbyExtract
 * Description: check all archive from catalog are provided by extract
 * Synopsis   : int 
                
 * Input      : Collection* upload
 *              
 * Output     : TRUE on success
 =======================================================================*/
int 
isCatalogRefbyExtract(Collection *upload)
{ 
  int rc = FALSE;
  CatalogTree* self = 0;
  Document* doc = 0;
  Archive* archive = 0;
  AVLNode *node = 0;
  RGIT* curr = 0;

  checkCollection(upload);
  logMain(LOG_DEBUG, "isCatalogRefbyExtract");
  if (!(self = upload->catalogTree)) goto error;
  
  if (avl_count(self->documents)) {
    for(node = self->documents->head; node; node = node->next) {
      doc = (Document*)node->item;
      
      if (!isEmptyRing(doc->archives)) {
	while ((archive = rgNext_r(doc->archives, &curr)) != 0) {
	  
	  if (!archive->fromContainers->nbItems) {
	    logMain(LOG_ERR, 
		    "Archive %s%lli from catalog has no extraction rule",
		    archive->hash, archive->size);
	    goto error;
	  }
	}
      }
    }
  }
    
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "isCatalogRefbyExtract fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : checkFileRefbyExtract
 * Description: check extract provide a container describing the data
 * Synopsis   : int 
                
 * Input      : Collection* upload
 *              
 * Output     : TRUE on success
 =======================================================================*/
int 
checkFileRefbyExtract(Collection *upload, Archive* archive)
{ 
  int rc = FALSE;

  checkCollection(upload);
  logMain(LOG_DEBUG, "checkFileRefbyExtract");
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "checkFileRefbyExtract fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : checkFileRefbyCatalog
 * Description: check catalog describe the data
 * Synopsis   : int 
                
 * Input      : Collection* upload
 *              
 * Output     : TRUE on success
 =======================================================================*/
int 
checkFileRefbyCatalog(Collection *upload, Archive* archive)
{ 
  int rc = FALSE;

  checkCollection(upload);
  logMain(LOG_DEBUG, "checkFileRefbyCatalog");
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "checkFileRefbyCatalog fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : concatCatalog
 * Description: concatenate uploaded catalog metadata
 * Synopsis   : static int 
 *                  concatCatalog(Collection* coll, Collection *upload)
 * Input      : Collection* coll: target
 *              Collection* upload: source
 * Output     : TRUE on success
 =======================================================================*/
static int 
concatCatalog(Collection* coll, Collection *upload)
{ 
  int rc = FALSE;
  CvsFile fd = {0, 0, 0, FALSE, 0, cvsCatOpen, cvsCatPrint};

  logMain(LOG_DEBUG, "concatCatalog");

  upload->catalogDB = coll->catalogDB;
  if (!(serializeCatalogTree(upload, &fd))) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "concatCatalog fails");
  }
  upload->catalogDB = 0;
  return rc;
}

/*=======================================================================
 * Function   : concatExtract
 * Description: concatenate uploaded extract metadata
 * Synopsis   : static int 
 *                  concatExtract(Collection* coll, Collection *upload)
 * Input      : Collection* coll: target
 *              Collection* upload: source
 * Output     : TRUE on success
 =======================================================================*/
static int 
concatExtract(Collection* coll, Collection *upload)
{ 
  int rc = FALSE;
  CvsFile fd = {0, 0, 0, FALSE, 0, cvsCatOpen, cvsCatPrint};

  logMain(LOG_DEBUG, "concatExtract");

  upload->extractDB = coll->extractDB;
  if (!(serializeExtractTree(upload, &fd))) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "concatExtract fails");
  }
  upload->extractDB = 0;
  return rc;
}


/*=======================================================================
 * Function   : mdtxUpload
 * Description: upload manager
 * Synopsis   : int mdtxUpload(char* label, char* catalog, char* extract, 
 *                             char* file, char* targetPath)
 * Input      : char* label: related collection
 *              char* catalog: catalog metadata file to upload
 *              char* extract: extract metadata file to upload
 *              char* file: data to upload
 *              char* targetPath: where to copy data into the cache
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxUpload(char* label, char* catalog, char* extract, char* file, 
	   char* targetPath)
{ 
  int rc = FALSE;
  Collection *coll = 0;
  Collection *upload = 0;
  Archive* archive = 0;

  logMain(LOG_DEBUG, "mdtxUpload");

  if (!allowedUser(env.confLabel)) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;

  // build a new collection to parse upload contents
  if (!(upload = addCollection("_upload_"))) goto error;
  if (!(upload->masterLabel = createString(coll->masterUser))) goto error;
  if (!(upload->user = createString(coll->user))) goto error;
  strcpy(upload->masterHost, "localhost");

  // uploaded contents to the new collection
  if (catalog && !uploadCatalog(upload, catalog)) goto error;
  if (extract && !uploadExtract(upload, extract)) goto error;
  if (file && !(archive = uploadContent(upload, file))) goto error;

  // Do some checks depending on what we have
  if (catalog && extract && 
      !isCatalogRefbyExtract(upload)) goto error;
  if (extract && file && 
      !checkFileRefbyExtract(upload, archive)) goto error;
  if (catalog && !extract && file && 
      !checkFileRefbyCatalog(upload, archive)) goto error;

  // concatenate metadata to the true collection
  upload->memoryState |= EXPANDED;
  if (catalog) {
    if (!concatCatalog(coll, upload)) goto error;
  }
  if (extract || file) {
    if (!concatExtract(coll, upload)) goto error;
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
