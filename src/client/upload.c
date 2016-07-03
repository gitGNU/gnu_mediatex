
/*=======================================================================
 * Project: MediaTeX
 * Module : upload
 *
 * Manage upload

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

#include "mediatex-config.h"
#include "client/mediatex-client.h"

/*=======================================================================
 * Function   : createUploadFile 
 * Description: Create, by memory allocation an image file (ISO file)
 * Synopsis   : UploadFile* createUploadFile(void)
 * Input      : N/A
 * Output     : The address of the create empty image file
 =======================================================================*/
UploadFile* 
createUploadFile(void)
{
  UploadFile* rc = 0;

  rc = (UploadFile*)malloc(sizeof(UploadFile));
  if(rc == 0) goto error;
   
  memset(rc, 0, sizeof(UploadFile));

  return(rc);
 error:
  logMemory(LOG_ERR, "malloc: cannot create an UploadFile");
  return(rc);
}

/*=======================================================================
 * Function   : destroyUploadFile 
 * Description: Destroy an image file by freeing all the allocate memory.
 * Synopsis   : void destroyUploadFile(UploadFile* self)
 * Input      : UploadFile* self = the address of the image file to
 *              destroy.
 * Output     : Nil address of an image FILE
 =======================================================================*/
UploadFile* 
destroyUploadFile(UploadFile* self)
{
  if(self == 0) goto error;
  destroyString(self->source);
  destroyString(self->target);
  free(self);
 error:
  return (UploadFile*)0;
}


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
  ExtractTree* self = 0;

  logMain(LOG_DEBUG, "uploadExtract");
  checkCollection(upload);
  checkLabel(path);
  
  if (!(upload->extractTree = createExtractTree())) goto error;
  if (!(upload->extractDB = createString(path))) goto error;
  if (!parseExtractFile(upload, path)) goto error;

  // assert there is no INC content  
  if (!(self = upload->extractTree)) goto error;
  if (avl_count(self->incoming->childs) > 0) {
    logMain(LOG_ERR, "unexpected incoming rules on upload");
    rc = FALSE;
  }

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
  CheckData md5; 
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
  memset(&md5, 0, sizeof(CheckData));
  md5.path = path;
  md5.size = statBuffer.st_size;
  md5.opp = CHECK_CACHE_ID;
  if (!doChecksum(&md5)) goto error;

  // archive to upload
  if (!(archive = addArchive(upload, md5.fullMd5sum, statBuffer.st_size)))
    goto error;

  // add incoming extraction rule
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
  if (!releaseCollection(coll, EXTR)) rc = FALSE;
  if (!rc) {
    logMain(LOG_ERR, "areNotAlreadyThere fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : uploadFile 
 * Description: Ask daemon to upload the file
 * Synopsis   : static int
 *               uploadFile(Collection* coll, RG* upFiles)
 * Input      : Collection* coll
 *              RG* upFiles: UploadFile's ring (source, target + archive)
 * Output     : TRUE on success
 =======================================================================*/
static int
uploadFile(Collection* coll, RG* upFiles)
{
  int rc = FALSE;
  int socket = -1;
  RecordTree* tree = 0;
  Record* record = 0;
  UploadFile* upFile = 0;
  Archive* archive = 0;
  char* source = 0;
  char* target = 0;
  char buf[MAX_SIZE_STRING] = "";
  char* extra = 0;
  char* message = 0;
  char reply[576];
  int status = 0;
  int n = 0;

  logMain(LOG_DEBUG, "uploadFile");
  checkCollection(coll);
  if (isEmptyRing(upFiles)) {
    logMain(LOG_ERR, "Please provide a ring");
    goto error;
  }

  // build the UPLOAD message
  if (!getLocalHost(coll)) goto error;
  if (!(tree = createRecordTree())) goto error; 
  tree->collection = coll;
  tree->messageType = UPLOAD; 

  // add files to the UPLOAD message
  rgRewind(upFiles);
  while ((upFile = rgNext(upFiles))) {
    source = upFile->source;
    target = upFile->target;
    archive = upFile->archive;
  
    // tels "/source:target"
    if (!(extra = getAbsolutePath(source))) goto error;
    if (snprintf(buf, MAX_SIZE_STRING, "%s%s%s", extra, 
		 target?":":"", target?target:"") 
	>= MAX_SIZE_STRING) {
      logMain(LOG_ERR, "buffer too few to copy source and target paths");
      goto error;
    }

    extra = destroyString(extra);
    extra = createString(buf);
    if (!(record = addRecord(coll, coll->localhost, archive,
			     SUPPLY, extra))) goto error;
    extra = 0;
    if (!avl_insert(tree->records, record)) {
      logMain(LOG_ERR, "cannot add record to tree (already there?)");
      goto error;
    }
    record = 0;
  }
  
  // ask daemon to upload the file into the cache
  if ((socket = connectServer(coll->localhost)) == -1) goto error;
  if (!upgradeServer(socket, tree, 0)) goto error;
    
  // read reply
  if (env.dryRun) goto end;
  if ((n = tcpRead(socket, reply, 255)) > 0) {
    reply[n-1] = (char)0; // remove ending \n
  }
  if (sscanf(reply, "%i", &status) < 1) {
    logMain(LOG_ERR, "error parsing daemon reply: %s", reply);
    goto error;
  }
  message = strstr(reply, " ");
    
  if (status != 210) {
    logMain(LOG_ERR, "daemon says (%i)%s", status, message);
    goto error;
  }

  logMain(LOG_INFO, "daemon says (%i)%s", status, message);
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
 *                             RG* upFiles)
 * Input      : char* label: related collection
 *              char* catalog: catalog metadata file to upload
 *              char* extract: extract metadata file to upload
 *              RG* upFiles: ring of UploadFile* (source+target paths)
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxUpload(char* label, char* catalog, char* extract, RG* upFiles)
{ 
  int rc = FALSE;
  Collection *coll = 0;
  Collection *upload = 0;
  UploadFile* upFile = 0;
  int isAllowed = 0;
  
  logMain(LOG_DEBUG, "mdtxUpload");
  checkLabel(label);
  if (!catalog && !extract && isEmptyRing(upFiles)) {
    logMain(LOG_ERR, "please provide some content to upload");
    goto error;
  }
  if (!allowedUser(env.confLabel, &isAllowed, 0)) goto error;
  if (!isAllowed) goto error;
  if (!(coll = mdtxGetCollection(label))) goto error;

  // create a new collection to parse upload contents
  if ((upload = getCollection("_upload_"))) goto error;
  if (!(upload = addCollection("_upload_"))) goto error;
  if (!(upload->masterLabel = createString(coll->masterUser))) goto error;
  if (!(upload->user = createString(coll->user))) goto error;
  strcpy(upload->masterHost, "localhost");

  // uploaded contents to the new collection
  if (extract && !uploadExtract(upload, extract)) goto error;
  if (!isEmptyRing(upFiles)) {
    while ((upFile = rgNext(upFiles))) {
      if (!(upFile->archive = uploadContent(upload, upFile->source)))
	goto error;
    }
  }
  if (catalog && !uploadCatalog(upload, catalog)) goto error;

  // Do some checks depending on what we have
  if (extract && !isEmptyRing(upFiles)) {
    rgRewind(upFiles);
    while ((upFile = rgNext(upFiles))) {
      if (!isFileRefbyExtract(upload, upFile->archive)) goto error;
    }
  }
    
  // Check we erase nothing in actual extraction metadata
  if (extract || !isEmptyRing(upFiles)) {
    if (!areNotAlreadyThere(coll, upload)) goto error;
  }
  
  // ask daemon to upload the file
  if (!isEmptyRing(upFiles)) {
    if (!uploadFile(coll, upFiles)) goto error;
  }

  // prevent saveCollection from unlinking new NNN.txt files,
  // and also force reload on upload+[+] to use the new NNN.txt files
  // note: that does not disease upload coll as not set as LOADED
  // * MAYBE: simplify below code understanding by reparsing upload into
  // coll (no more NNN files nor reload needed on upload+[+])
  if (!clientDiseaseAll()) goto error;

  // serialize NNN.txt files
  upload->memoryState |= EXPANDED;
  if (catalog && !concatCatalog(coll, upload)) goto error;
  if (extract || !isEmptyRing(upFiles)) {
    if (!concatExtract(coll, upload)) goto error;
  }

  // tell the server we have upgraded the extraction metadata
  if (extract || !isEmptyRing(upFiles)) {
    if (!env.noRegression && !env.dryRun) {
      mdtxAsyncSignal(0); // send HUP signal to daemon
    }
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
