
/*=======================================================================
 * Version: $Id: upload.c,v 1.9 2015/08/17 01:31:52 nroche Exp $
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

  logMain(LOG_DEBUG, "uploadCatalog");
  checkCollection(upload);
  checkLabel(path);
  
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

  logMain(LOG_DEBUG, "uploadExtract");
  checkCollection(upload);
  checkLabel(path);
  
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

  logMain(LOG_DEBUG, "uploadContent");
  checkCollection(upload);
  checkLabel(path);

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
  if (archive) delArchive(upload, archive);
  return rc;
}


/*=======================================================================
 * Function   : isCatalogRefbyExtract
 * Description: check all archive from catalog are provided by extract
 * Synopsis   : int isCatalogRefbyExtract(Collection *coll)
 * Input      : Collection* coll              
 * Output     : TRUE on success
 =======================================================================*/
int 
isCatalogRefbyExtract(Collection *coll)
{ 
  int rc = FALSE;
  Archive* archive = 0;
  AVLNode *node = 0;

  logMain(LOG_DEBUG, "isCatalogRefbyExtract");
  checkCollection(coll);

  // loop on archive from upload catalog
  rc = TRUE;
  if (!avl_count(coll->archives)) goto end;
  for(node = coll->archives->head; node; node = node->next) {
    archive = (Archive*)node->item;
    if (!isEmptyRing(archive->documents)) {

      // check there is an extraction rule into upload extract
      if (!hasExtractRule(archive)) {
	logMain(LOG_ERR, 
		"Extract file should provide rule for %s:%lli "
		"provided by catalog",
		archive->hash, archive->size);
	rc = FALSE;
      }
    }
  }
  
 end:
 error:
  if (!rc) {
    logMain(LOG_ERR, "isCatalogRefbyExtract fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : isFileRefbyExtract
 * Description: check extract provides a container describing the data
 * Synopsis   : int isFileRefbyExtract(Collection *coll, 
 *                                     Archive* archive)
 * Input      : Collection* coll
 *              Archive* archive
 * Output     : TRUE on success
 =======================================================================*/
int 
isFileRefbyExtract(Collection *coll, Archive* archive)
{ 
  int rc = FALSE;

  logMain(LOG_DEBUG, "isFileRefbyExtract");
  checkCollection(coll);
  
  if (!archive->toContainer) {
    logMain(LOG_ERR, 
	    "Extract file should provide a container for %s%lli",
	    archive->hash, archive->size);
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "isFileRefbyExtract fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : isFileRefbyCatalog
 * Description: check catalog describes the data
 * Synopsis   : int isFileRefbyCatalog(Collection *coll, 
 *                                     Archive* archive)
 * Input      : Collection* coll
 *              Archive* archive
 * Output     : TRUE on success
 =======================================================================*/
int 
isFileRefbyCatalog(Collection *coll, Archive* archive)
{ 
  int rc = FALSE;

  logMain(LOG_DEBUG, "isFileRefbyCatalog");
  checkCollection(coll);
  
  if (isEmptyRing(archive->documents)) {
    logMain(LOG_ERR, 
	    "Archive %s%lli has no description on catalog",
	    archive->hash, archive->size);
    goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "isFileRefbyCatalog fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : areNotAlreadyThere
 * Description: check extraction rules are new (else parsing will fails)
 * Synopsis   : int areNotAlreadyThere(Collection *coll, 
 *                                     Collection *upload)
 * Input      : Collection* coll:   actuals extract rules
 *              Collection* upload: new extract rules
 *              Archive* archive
 * Output     : TRUE on success
 =======================================================================*/
int 
areNotAlreadyThere(Collection *coll, Collection* upload)
{ 
  int rc = FALSE;
  ExtractTree* self = 0;
  Archive* archUp = 0;
  Archive* archCol = 0;
  Container* container = 0;
  AVLNode *node = 0;
  RGIT* curr = 0; 

  logMain(LOG_DEBUG, "areNotAlreadyThere");
  checkCollection(coll);
  checkCollection(upload);
  if (!(self = upload->extractTree)) goto error;
  if (!loadCollection(coll, EXTR)) goto error;
  rc = TRUE;

  // loop on uploaded INC contents (should have only one in fact)
  container = self->incoming;
  for(node = container->childs->head; node; node = node->next) {
    archUp = ((FromAsso*)node->item)->archive;

    // check it is not already into actual extractTree:
    if ((archCol = getArchive(coll, archUp->hash, archUp->size))) {      
      if (hasExtractRule(archCol)) {
	logMain(LOG_ERR, 
		"Incoming archive %s%lli already has an extraction rule", 
		archUp->hash, archUp->size);
	rc = FALSE;
      }
    }
  }
  
  // loop on uploaded containers
  if (avl_count(self->containers)) {
    for(node = self->containers->head; node; node = node->next) {
      container = node->item;
      curr = 0;
      while ((archUp = rgNext_r(container->parents, &curr))) {

	// check container not already into actual extractTree
	if ((archCol = getArchive(coll, archUp->hash, archUp->size))) {
	  if (archCol->toContainer) {
	    logMain(LOG_ERR, 
		    "Incoming container %s%lli already set as container", 
		    archUp->hash, archUp->size);
	    rc = FALSE;
	  }
	}
      }
    }
  }

 error:
  if (!rc) {
    logMain(LOG_ERR, "areNotAlreadyThere fails");
  }
  if (!releaseCollection(coll, EXTR)) goto error;
  return rc;
}

/*=======================================================================
 * Function   : uploadFile 
 * Description: Ask daemon to upload the file
 * Synopsis   : static int
 *               uploadFile(Collection* coll, Archive* archive, 
 *                          char* source, char* target)
 * Input      : Collection* coll
 *              Archive* archive
 *            : char* source: path to the file to upload
 *              char* target: path to use into the cache
 * Output     : TRUE on success
 =======================================================================*/
static int
uploadFile(Collection* coll, Archive* archive, char* source, char* target)
{
  int rc = FALSE;
  int socket = -1;
  RecordTree* tree = 0;
  Record* record = 0;
  char* extra = 0;
  char* message = 0;
  char reply[256];
  int status = 0;
  int n = 0;

  logMain(LOG_DEBUG, "uploadFile");
  checkCollection(coll);
  checkLabel(source);

  // build the UPLOAD message
  if (!getLocalHost(coll)) goto error;
  if (!(tree = createRecordTree())) goto error; 
  tree->collection = coll;
  tree->messageType = UPLOAD; 
  if (!(extra = absolutePath(source))) goto error;
  if (!(record = addRecord(coll, coll->localhost, archive, SUPPLY, extra)))
    goto error;
  extra = 0;
  if (!rgInsert(tree->records, record)) goto error;
  record = 0;
  
  // ask daemon to upload the file into the cache
  logMain(LOG_INFO, "ask daemon to upload %s", source);
  if (env.noRegression) {
    logRecordTree(LOG_MAIN, LOG_INFO, tree, 0);
    goto end;
  }
  if ((socket = connectServer(coll->localhost)) == -1) goto error;
  if (!upgradeServer(socket, tree, 0)) goto error;
    
  // read reply
  if (env.dryRun) goto end;
  n = tcpRead(socket, reply, 255);
  tcpRead(socket, reply, 1);
  if (sscanf(reply, "%i %s", &status, message) < 1) {
    reply[(n<=0)?0:n-1] = (char)0; // remove ending \n
    logMain(LOG_ERR, "error parsing daemon reply: %s", reply);
    goto error;
  }
    
  logMain(LOG_INFO, "daemon says (%i) %s", status, message);
  if (status != 200) goto error;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "uploadFile fails");
  }
  extra = destroyString(extra);
  if (record) delRecord(coll, record);
  tree = destroyRecordTree(tree);
  if (!env.dryRun && socket != -1) close(socket);
  return rc;
}


/*=======================================================================
 * Function   : concatCatalog
 * Description: concatenate uploaded catalog metadata
 * Synopsis   : static int concatCatalog(Collection* coll, 
 *                                       Collection *upload)
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
 * Synopsis   : static int concatExtract(Collection* coll, 
 *                                       Collection *upload)
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
mdtxUpload(char* label, char* catalog, char* extract, 
	   char* file, char* targetPath)
{ 
  int rc = FALSE;
  Collection *coll = 0;
  Collection *upload = 0;
  Archive* archive = 0;

  logMain(LOG_DEBUG, "mdtxUpload");
  checkLabel(label);
  if (!catalog && !extract && !file) {
    logMain(LOG_ERR, "please provide some content to upload");
    goto error;
  }
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
      !isFileRefbyExtract(upload, archive)) goto error;
  if (catalog && !extract && file && 
      !isFileRefbyCatalog(upload, archive)) goto error;

  // Check we erase nothing in actual extraction metadata
  if ((extract || file) &&
      !areNotAlreadyThere(coll, upload)) goto error;

  // ask daemon to upload the file
  if (file && !uploadFile(coll, archive, file, targetPath)) goto error;

  // concatenate metadata to the true collection
  upload->memoryState |= EXPANDED;
  if (catalog && !concatCatalog(coll, upload)) goto error;
  if ((extract || file) && !concatExtract(coll, upload)) goto error;

  // tell the server we have upgrade files
  if ((extract || file) && !env.noRegression && !env.dryRun) {
     mdtxAsyncSignal(0); // send HUP signal to daemon
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
