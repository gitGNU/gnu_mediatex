/*=======================================================================
 * Project: MediaTeX
 * Module : recordTree
 *
 * recordTree producer interface
 *
 * Note: records physically belongs to
 * - coll->cacheTree->recordTree for local records
 * - server->recordTree for remote records

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche
 
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

/*=======================================================================
 * Function   : cmpRecord
 * Description: compare 2 records
 * Synopsis   : int cmpRecord(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2 : the records
 * Output     : <, = or >0 respectively for lower, equal or greater
 =======================================================================*/
int
cmpRecord(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Record*
   */
  
  Record* a1 = *((Record**)p1);
  Record* a2 = *((Record**)p2);

  if (!rc) rc = cmpArchive(&a1->archive, &a2->archive);
  if (!rc) rc = cmpServer(&a1->server, &a2->server);

  // do sort on REMOVE flag!
  if (!rc) rc = a1->type - a2->type;

  if (!rc) {
    // extra should not but may be null
    if (!isEmptyString(a1->extra) && !isEmptyString(a2->extra))
      rc = strcmp(a1->extra, a2->extra);
    if (isEmptyString(a1->extra) && !isEmptyString(a2->extra))
      rc = -1;
    if (!isEmptyString(a1->extra) && isEmptyString(a2->extra))
      rc = 1;
  }
 
  return rc;
}

/*=======================================================================
 * Function   : cmpRecordAvl
 * Description: compare 2 records
 * Synopsis   : int cmpRecord(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2 : the records
 * Output     : <, = or >0 respectively for lower, equal or greater
 * Note       : sort by chronological dates and by growing sizes
 =======================================================================*/
int 
cmpRecordAvl(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on items
   * and items are suposed to be Record* 
   */
  
  Record* a1 = (Record*)p1;
  Record* a2 = (Record*)p2;
  
  // chronological order 
  if (!rc) rc = (a1->date - a2->date);

  // growing sizes
  if (!rc) rc = a1->archive->size - a2->archive->size;

  if (!rc) rc = cmpRecord(&a1, &a2);
  return rc;
}

/*=======================================================================
 * Function   : cmpRecordPath
 * Description: compare 2 records on path
 * Synopsis   : int cmpRecordPath(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2 : the records
 * Output     : <, = or >0 respectively for lower, equal or greater
 * Note       : sort by alphabetic order 
 =======================================================================*/
int 
cmpRecordPath(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Record* 
   */
  
  Record* a1 = *((Record**)p1);
  Record* a2 = *((Record**)p2);

  if (!rc) rc = strcmp(a1->extra, a2->extra);
  if (!rc) rc = cmpRecord(p1, p2);
 
  return rc;
}

/*=======================================================================
 * Function   : cmpRecordPathAvl
 * Description: compare 2 records on path
 * Synopsis   : int cmpRecordPathAvl(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2 : the records
 * Output     : <, = or >0 respectively for lower, equal or greater
 * Note       : sort by alpabetic order (uniq key) 
 =======================================================================*/
int 
cmpRecordPathAvl(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on items
   * and items are suposed to be Record* 
   */
  
  Record* a1 = (Record*)p1;
  Record* a2 = (Record*)p2;

  if (!rc) rc = strcmp(a1->extra, a2->extra);

  // use for local cache so no need to compare on other fields
  return rc;
}

/*=======================================================================
 * Function   : createRecord
 * Description: Create, by memory allocation a Record
 *              configuration projection.
 * Synopsis   : Record* createRecord(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Record* 
createRecord(void)
{
  Record* rc = 0;

  if ((rc = (Record*)malloc(sizeof(Record))) == 0)
    goto error;
   
  memset(rc, 0, sizeof(Record));
  return rc;
 error:
  logMemory(LOG_ERR, "malloc: cannot create Record");
  return rc;
}

/*=======================================================================
 * Function   : destroyRecord
 * Description: Destroy a configuration by freeing all the allocate memory.
 * Synopsis   : void destroyRecord(Record* self)
 * Input      : Record* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a configuration.
 =======================================================================*/
Record* 
destroyRecord(Record* self)
{
  Record* rc = 0;

  if(self == 0) goto error;
    
#if 1 // developpement mode
  self->archive = 0;
  self->server = 0;
  self->date = 0;
  self->type = UNDEF_;
#endif 
  self->extra = destroyString(self->extra);

  free(self);
 error:
  return(rc);
}

/*=======================================================================
 * Function   : logRecord
 * Description: Log a configuration.
 * Synopsis   : void logRecord(int logModule, int logPriority, 
 *                             Record* self)
 * Input      : int logModule = caller module
 *              int logPriority = log priority to use
 *              Record* self = what to log
 * Output     : TRUE on success
 =======================================================================*/
int 
logRecord(int logModule, int logPriority, Record* self)
{
  struct tm date;

  checkRecord(self);

  if (self->type & REMOVE) goto end;
  if (localtime_r(&self->date, &date) == (struct tm*)0) goto error;

  logEmit(logModule, logPriority, "%c "
	   "%04i-%02i-%02i,%02i:%02i:%02i "
	   "%*s %*s %*llu %s",
	  (self->type & 0x3) == DEMAND?'D':
	  (self->type & 0x3) == SUPPLY?'S':'?',
	  date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	  date.tm_hour, date.tm_min, date.tm_sec,
	  MAX_SIZE_MD5, self->server->fingerPrint, 
	  MAX_SIZE_MD5, self->archive->hash, 
	  MAX_SIZE_SIZE, (long long unsigned int)self->archive->size,
	  self->extra?self->extra:"");

  logEmit(logModule, logPriority, "# ^ %s", strRecordType(self));
 end:
  return TRUE;
 error:
  logEmit(logModule, LOG_ERR, "fails to log a record");
  return FALSE;
}

/*=======================================================================
 * Function   : serializeRecord
 * Description: Serialize a configuration.
 * Synopsis   : int serializeRecord(RecordTree* tree, Record* self)
 * Input      : RecordTree* tree = context (to retrieve AESData)
 *              Record* self = what to serialize
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeRecord(RecordTree* tree, Record* self)
{
  struct tm date;

  checkRecord(self);
  checkRecordTree(tree);

  // not easy to read when serializing to stdout because of aesPrint
  if (!env.noRegression) {
    logMemory(LOG_DEBUG, "serialize Record: %s, %s %s:%lli",
    strRecordType(self), self->server->fingerPrint, 
    self->archive->hash, (long long int)self->archive->size);
  }
 
  if (self->type & REMOVE) goto end;

  switch (getRecordType(self)) {
  case MALLOC_SUPPLY:
    logMemory(LOG_INFO, "ignore malloc record");
    goto end;
  case UNDEF_RECORD:
    logMemory(LOG_WARNING, "ignore undefined record");
    goto end;
  default:
    ;
  }
  
  if (localtime_r(&self->date, &date) == (struct tm*)0) {
    logMemory(LOG_ERR, "localtime_r returns on error");
    goto error;
  }

  aesPrint(&tree->aes, "%c "
	   "%04i-%02i-%02i,%02i:%02i:%02i "
	   "%*s %*s %*llu %s\n",
	   //(self->type & REMOVE)?'-':' ', (no more used)
	  (self->type & 0x3) == DEMAND?'D':
	  (self->type & 0x3) == SUPPLY?'S':'?',
	  date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	  date.tm_hour, date.tm_min, date.tm_sec,
	  MAX_SIZE_MD5, self->server->fingerPrint, 
	  MAX_SIZE_MD5, self->archive->hash, 
	  MAX_SIZE_SIZE, (long long unsigned int)self->archive->size,
	  self->extra?self->extra:"");

  if (env.logHandler->severity[LOG_MEMORY]->code >=  LOG_INFO) {
    aesPrint(&tree->aes, "# ^ %s\n", strRecordType(self));
  }

 end:
  return TRUE;
 error:
  logMemory(LOG_ERR, "fails to serialize a record");
  return FALSE;
}

/*=======================================================================
 * Function   : getRecordType
 * Description: get the record type
 * Synopsis   : getRecordType(Record* self);
 * Input      : Record* self 
 * Output     : RecordType
 =======================================================================*/
RecordType 
getRecordType(Record* self)
{
  RecordType rc = UNDEF_RECORD;

  checkRecord(self);

  if (self->server->isLocalhost && isEmptyString(self->extra)) {
    logMemory(LOG_ERR, 
	    "extra string is needed to get the local record type");
    goto error;
  }

  switch (self->type & 0x3) {
  case DEMAND:
    if (!self->server->isLocalhost)  
      rc = REMOTE_DEMAND;
    else if (self->extra[0] != '!')
      rc = FINAL_DEMAND;
    else if (self->extra[1] == 'w')
      rc = LOCAL_DEMAND;
    break;

  case SUPPLY:
    if (!self->server->isLocalhost)  
      rc = REMOTE_SUPPLY;
    else if (self->extra[0] == '/')
      rc = FINAL_SUPPLY;
    else if (self->extra[0] != '!')
      rc = LOCAL_SUPPLY;
    else if (self->extra[1] == 'm')
      rc = MALLOC_SUPPLY;
    break;
  }

 error:
  if (!rc) {
    logMemory(LOG_ERR, "getRecordType fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : strRecordType
 * Description: return a string fot the record type
 * Synopsis   : strRecordType(Record* self);
 * Input      : Record* self 
 * Output     : char*    
 =======================================================================*/
char* 
strRecordType2(RecordType type)
{
  switch (type) {
  case FINAL_SUPPLY:
    return "FINAL_SUPPLY";
  case LOCAL_SUPPLY:
    return "LOCAL_SUPPLY";
  case REMOTE_SUPPLY:
    return "REMOTE_SUPPLY";
  case MALLOC_SUPPLY:
    return "MALLOC_SUPPLY";
  case FINAL_DEMAND:
    return "FINAL_DEMAND";
  case LOCAL_DEMAND:
    return "LOCAL_DEMAND";
  case REMOTE_DEMAND:
    return "REMOTE_DEMAND";
  default:
    return "UNDEF_RECORD";
  }
}

/*=======================================================================
 * Function   : strRecordType
 * Description: return a string fot the record type
 * Synopsis   : strRecordType(Record* self);
 * Input      : Record* self 
 * Output     : char*    
 =======================================================================*/
char* 
strRecordType(Record* self)
{
  return strRecordType2(getRecordType(self));
}

/*=======================================================================
 * Function   : createRecordTree
 * Description: Create, by memory allocation a RecordTree
 *              configuration projection.
 * Synopsis   : RecordTree* createRecordTree(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
RecordTree* 
createRecordTree(void)
{
  RecordTree* rc = 0;

  if ((rc = (RecordTree*) malloc (sizeof (RecordTree))) 
     == 0) {
    logMemory(LOG_ERR, "malloc: cannot create RecordTree");
    goto error;
  }

  memset (rc, 0, sizeof(RecordTree));

  // set AES for uncrypted serialisation to sdtout by default
  rc->aes.fd = STDOUT_FILENO;
  rc->aes.way = ENCRYPT;
  rc->doCypher = FALSE;

  if (!(rc->records =
	avl_alloc_tree(cmpRecordAvl, (avl_freeitem_t)destroyRecord)))
    goto error;
  
  return rc;
 error:
  logMemory(LOG_ERR, "fails to create RecordTree");
  return destroyRecordTree(rc);
}

/*=======================================================================
 * Function   : destroyRecordTree
 * Description: Destroy a configuration by freeing all the allocate memory.
 * Synopsis   : void destroyRecordTree(RecordTree* self)
 * Input      : RecordTree* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a configuration.
 =======================================================================*/
RecordTree* 
destroyRecordTree(RecordTree* self)
{
  RecordTree* rc = 0;

  if(self) {
    if (self->records) {
      avl_free_tree(self->records); // records items are freed
      self->records = 0;
    }
    free(self);
  }
  
  return(rc);
}

/*=======================================================================
 * Function   : strMessageType
 * Description: return a string for the record type
 * Synopsis   : strMessageType(MessageType self);
 * Input      : MessageType self 
 * Output     : char*    
 =======================================================================*/
char* 
strMessageType(MessageType self)
{
  switch (self) {
  case DISK:
    return "DISK";
   case CGI:
    return "CGI";
  case HAVE:
    return "HAVE";
  case NOTIFY:
    return "NOTIFY";
  case UPLOAD:
    return "UPLOAD";
  default:
    return "UNKNOWN";
  }
}

/*=======================================================================
 * Function   : logRecordTree
 * Description: Log a recordTree
 * Synopsis   : int logRecordTree(int logModule, int logPriority,
 *                                RecordTree* self, char* fingerPrint)
 * Input      : int logModule = caller module
 *              int logPriority = log priority to use
 *              RecordTree* self = what to log
 *              char* fingerPrint = original fingerprint when Natted
 * Output     : N/A
 =======================================================================*/
void 
logRecordTree(int logModule, int logPriority,
	      RecordTree* self, char* fingerPrint)
{ 
  int rc = FALSE;
  Record *record = 0;
  AVLNode *node = 0;

  checkRecordTree(self);
 
  // use local server fingerPrint if not provided
  if (fingerPrint == 0) {
    fingerPrint = self->collection->userFingerPrint;
  }

  // headers
  logEmit(logModule, logPriority, "%s", "# Collection's records:");
  logEmit(logModule, logPriority, "%s", "Headers"); 
  logEmit(logModule, logPriority, "  Collection %-*s",
	   MAX_SIZE_COLL, self->collection->label);
  logEmit(logModule, logPriority, "  Type       %s", 
	  strMessageType(self->messageType));
  logEmit(logModule, logPriority, "  Server     %s", fingerPrint);
  logEmit(logModule, logPriority, "  DoCypher   %s", 
	  (self->doCypher==TRUE)?"TRUE":"FALSE");

  logEmit(logModule, logPriority, "%s", "Body");

  // body
  logEmit(logModule, logPriority, "# %19s %*s %*s %*s %s",
	   "date", 
	   MAX_SIZE_MD5, "host",
	   MAX_SIZE_MD5, "hash", 
	   MAX_SIZE_SIZE, "size", 
	   "extra");

  rc = TRUE;
  for (node = self->records->head; node; node = node->next) {
    record = node->item;
    if (!logRecord(logModule, logPriority, record)) rc = FALSE;
  }
  
 error:
  if (!rc) {
    logEmit(logModule, LOG_ERR, "logRecordTree fails");
  }
}

/*=======================================================================
 * Function   : serializeRecordTree
 * Description: Serialize a configuration.
 * Synopsis   : int serializeRecordTree(RecordTree* self, char* path,
 *                                      char* fingerPrint)
 * Input      : RecordTree* self = what to serialize
 *              char* path       = file where to write
 *              fingerPrint      = original fingerprint when Natted
 * Output     : TRUE on success
 * Note       : output in use is self->aes->fd. It is:
 *               - stdout by default
 *               - updated if a path is provided
 =======================================================================*/
int 
serializeRecordTree(RecordTree* self, char* path, char* fingerPrint)
{ 
  int rc = FALSE;
  Server* localhost = 0;
  Record *record = 0;
  AVLNode *node = 0;
  AESData* aes = 0;
  int itIs = FALSE;

  checkRecordTree(self);
  logMemory(LOG_INFO, "Serializing %s record tree in file: %s", 
	  strMessageType(self->messageType), path?path:"stdout");

  aes = &(self->aes);
  if (!(localhost = getLocalHost(self->collection))) goto error;

  // output file to use (may already be set into aes->fd)
  if (!env.dryRun && !isEmptyString(path)) {
    if ((aes->fd = creat(path, S_IRWXU|S_IRGRP)) == -1) {
      logMemory(LOG_ERR, "Cannot write into the records tree file: %s", 
	      strerror(errno));
      goto error;
    }
  }

  // use local server fingerPrint if not provided
  if (fingerPrint == 0) {
    fingerPrint = self->collection->userFingerPrint;
  }

  // un-encrypted headers
  aes->doCypher = FALSE;
  
  aesPrint(aes, "%s", "# Collection's records:\n");
  aesPrint(aes, "%s", "Headers\n"); 
  aesPrint(aes, "  Collection %-*s\n",
	   MAX_SIZE_COLL, self->collection->label);
  aesPrint(aes, "  Type       %s\n", strMessageType(self->messageType));
  aesPrint(aes, "  Server     %s\n", fingerPrint);
  aesPrint(aes, "  DoCypher   %s", (self->doCypher==TRUE)?"TRUE":"FALSE");

  aesFlush(aes); // finish properly the un-encrypted headers
  aesPrint(aes, "%s", "\nBody          \n"); // must be 16 char here

  // crypted body
  if (self->doCypher) aes->doCypher = TRUE;
  aesPrint(aes, "# %19s %*s %*s %*s %s\n",
	   "date", 
	   MAX_SIZE_MD5, "host",
	   MAX_SIZE_MD5, "hash", 
	   MAX_SIZE_SIZE, "size", 
	   "extra");
  rc = TRUE;

  for (node = self->records->head; node; node = node->next) {
    record = node->item;

    // cleaning some records when saving on disk
    if (self->messageType == DISK) {

      // usefull to comment this for debuging
      //  (harmless but server should not serialize final-support on disk)
      // do not serialize final-supplies on md5sum.txt file
      if (getRecordType(record) == FINAL_SUPPLY) continue;

      // remove record that comes from badly configured networks
      if (getRecordType(record) == REMOTE_SUPPLY) {
	if (!isReachable(self->collection, 
			 localhost, record->server, &itIs)) goto error;
	if (!itIs) continue;
      }
    }

    if (!serializeRecord(self, record)) rc = FALSE;
  }
  
  if (!aesFlush(aes)) goto error;
  if (aes->fd == STDOUT_FILENO) write(aes->fd, "\n", 1);
  
  // if a socket, we do not close the file descriptor 
  if (aes->fd != STDOUT_FILENO && !isEmptyString(path)) {
    close(aes->fd);
  }
  rc = TRUE;
 error:
  return rc;
}


/*=======================================================================
 * Function   : newRecord
 * Description: find a record 
 * Synopsis   : Record* newRecord(Server* server, Archive* archive, 
 *                                Type type, char* extra)
 * Input      : Collection* coll: where to find
 *              Server* server: id1
 *              Archive* archive: id2
 *              Type  type: SUPPLY or DEMAND
 *              char* extra: file's path or custumer's mail
 * Output     : pointer on the new Record or 0 on error
 =======================================================================*/
Record* 
newRecord(Server* server, Archive* archive, Type type, char* extra)
{
  Record* rc = 0;
  Record* record = 0;

  checkServer(server);
  checkArchive(archive);
  logMemory(LOG_DEBUG, "newRecord: (%i), %s, %s:%lli",
	  type, server->fingerPrint, 
	  archive->hash, (long long int)archive->size);

  if ((type&0x3) != SUPPLY && (type&0x3) != DEMAND) {
    logMemory(LOG_ERR, "please choose a type (supply or demand) "
	    "for the new Record");
    goto error;
  }

  if ((record = createRecord()) == 0) goto error;
  if ((record->date = currentTime()) == -1) goto error;
  record->server = server;
  record->archive = archive;
  record->type = type;
  record->extra = extra;
  extra = 0; // consume string
  if (getRecordType(record) == UNDEF_RECORD) goto error;
  
  rc = record;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "newRecord fails");
    record = destroyRecord(record);
  }
  destroyString(extra);
  return rc;
}


/*=======================================================================
 * Function   : addRecord
 * Description: find a record 
 * Synopsis   : Record* addRecord(Collection* coll, Server* server, 
 *                            Archive* archive, Type type, char* extra)
 * Input      : Collection* coll: where to find
 *              Server* server: id1
 *              Archive* archive: id2
 *              Type  type: SUPPLY or DEMAND
 *              char* extra: file's path or custumer's mail
 * Output     : pointer on the new Record or 0 on error
 * Note       : severals record may have the same ids
 *              IS THIS A GOOD THINK ?   
 =======================================================================*/
Record* addRecord(Collection* coll, Server* server, Archive* archive,
		  Type type, char* extra)
{
  Record* rc = 0;
  Record* record = 0;

  checkCollection(coll);
  checkServer(server);
  checkArchive(archive);
  logMemory(LOG_DEBUG, "addRecord: (%i), %s, %s:%lli",
	  type, server->fingerPrint,
	  archive->hash, (long long int)archive->size);

  if (!(record = newRecord(server, archive, type, extra))) goto error;

  // add record to archive ring
  if (!rgInsert(archive->records, record)) goto error;
  
  /* // add record to server btree */
  /* if (!avl_insert(server->records, record)) { */
  /*   logMemory(LOG_ERR, "cannot add record (already there?)"); */
  /*   goto error; */
  /* } */

  rc = record;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addRecord fails");
    if (record) delRecord(coll, record); 
  }
  return rc;
}


/*=======================================================================
 * Function   : delRecord
 * Description: del a record 
 * Synopsis   : int delRecord(Collection* coll, Record* self)
 * Input      : Collection* coll: where to find
 *              Record* self: the record to del
 * Output     : TRUE on success
 =======================================================================*/
int delRecord(Collection* coll, Record* self)
{
  int rc = FALSE;
  RGIT* curr = 0;
  //AVLNode* node = 0;
 
  checkCollection(coll);
  checkRecord(self);
  logMemory(LOG_DEBUG, "delRecord: (%i), %s, %s:%lli",
	  self->type, self->server->fingerPrint,
	  self->archive->hash, (long long int)self->archive->size);

  // del record from archive ring
  if ((curr = rgHaveItem(self->archive->records, self))) {
    rgRemove_r(self->archive->records, &curr);
  }

  /* // del record from server ring */
  /* if ((node = avl_search(self->server->records, self))) { */
  /*   avl_unlink_node(self->server->records, node); */
  /*   remind(node); */
  /*   free(node); */
  /* } */

  // free the record
  self = destroyRecord(self);

  rc = TRUE;
 error:
 if (!rc) {
    logMemory(LOG_ERR, "delRecord fails");
 }
  return rc;
}

/*=======================================================================
 * Function   : diseaseRecordTree
 * Description: Disease a record tree.
 * Synopsis   : int DiseaseRecordTree(RecordTree* self)
 * Input      : RecordTree* self = what to Disease
 * Output     : TRUE on success
 =======================================================================*/
int 
diseaseRecordTree(RecordTree* self)
{ 
  int rc = FALSE;
  Record *record = 0;
  AVLNode* node = 0;
  
  checkRecordTree(self);
  checkCollection(self->collection);
  logMemory(LOG_DEBUG, "Disease %s related record tree", 
	  self->collection->label);
  
  for (node = self->records->head; node; node = node->next) {
    record = node->item;
    if (!delRecord(self->collection, record)) goto error;
    node->item = 0;
  }
  avl_free_nodes(self->records);
  
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_INFO, "diseaseRecordTree fails");
  }
  return(rc);
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
