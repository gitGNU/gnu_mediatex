/*=======================================================================
 * Project: MediaTeX
 * Module : extraction tree
 *
 * Extraction producer interface

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


/*=======================================================================
 * Function   : serializeEtype
 * Description: Serialize a Etype.
 * Synopsis   : int serializeEtype(Etype self, CvsFile* fd)
 * Input      : Etype self : what to serialize
 *              CvsFile* fd : where to serialize
 * Output     : TRUE on success
 =======================================================================*/
char*
strEType(EType self)
{

  static char* typeLabels[ETYPE_MAX] =
    {"UNDEF", "INC", "IMG",
     "ISO", "CAT", 
     "TGZ", "TBZ", "AFIO",
     "TAR", "CPIO", 
     "GZIP", "BZIP",
     "ZIP", "RAR"};

  if (self < 0 || self >= ETYPE_MAX) {
    logMemory(LOG_WARNING, "unknown extract type %i", self);
    return "???";
  }

  return typeLabels[self];
}


/*=======================================================================
 * Function   : getEtype
 * Description: Get a Etype.
 * Synopsis   : EType getEtype(char* label)
 * Input      : char* label : label to translate
 * Output     : corresponding EType
 =======================================================================*/
EType
getEType(char* label)
{
  if (!strcasecmp(label, "INC")) return INC;
  if (!strcasecmp(label, "ISO")) return ISO;
  if (!strcasecmp(label, "CAT")) return CAT;
  if (!strcasecmp(label, "TGZ")) return TGZ;
  if (!strcasecmp(label, "TBZ")) return TBZ;
  if (!strcasecmp(label, "AFIO")) return AFIO;
  if (!strcasecmp(label, "TAR")) return TGZ;
  if (!strcasecmp(label, "CPIO")) return CPIO;
  if (!strcasecmp(label, "GZIP")) return GZIP;
  if (!strcasecmp(label, "BZIP")) return BZIP;
  if (!strcasecmp(label, "ZIP")) return ZIP;
  if (!strcasecmp(label, "RAR")) return RAR;
  return -1;
}


/*=======================================================================
 * Function   : createFromAsso
 * Description: Create, by memory allocation an FromAsso
 * Synopsis   : FromAsso* createFromAsso(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
FromAsso* 
createFromAsso(void)
{
  FromAsso* rc = 0;

  if ((rc = (FromAsso*)malloc(sizeof(FromAsso))) == 0)
    goto error;

  memset(rc, 0, sizeof(FromAsso));
 
  return rc;  
 error:  
  logMemory(LOG_ERR, "malloc: cannot create FromAsso");
  return destroyFromAsso(rc);
}

/*=======================================================================
 * Function   : destroyFromAsso
 * Description: Destroy an FromAsso by freeing own allocated memory.
 * Synopsis   : void destroyFromAsso(FromAsso* self)
 * Input      : FromAsso* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a FromAsso.
 =======================================================================*/
FromAsso* 
destroyFromAsso(FromAsso* self)
{
  if (self == 0) goto error;
  self->path = destroyString(self->path);
  free(self);
 error:
  return (FromAsso*)0;
}

/*=======================================================================
 * Function   : cmpFromAssoAvl
 * Description: compare two FromAsso
 * Synopsis   : int cmpFromAssoAvl(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on FromAsso
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int 
cmpFromAssoAvl(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items */
  FromAsso* v1 = (FromAsso*)p1;
  FromAsso* v2 = (FromAsso*)p2;

  rc = cmpArchiveAvl(v1->archive, v2->archive);
  if (!rc) rc = cmpContainerAvl(v1->container, v2->container);
  return rc;
}


/*=======================================================================
 * Function   : createContainer
 * Description: Create, by memory allocation an Container
 * Synopsis   : Container* createContainer(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Container* 
createContainer(void)
{
  Container* rc = 0;

  if ((rc = (Container*)malloc(sizeof(Container))) == 0)
    goto error;

  memset(rc, 0, sizeof(Container));
  if ((rc->parents = createRing()) == 0) goto error;
  if (!(rc->childs = 
	avl_alloc_tree(cmpFromAssoAvl, 
		       (avl_freeitem_t)destroyFromAsso)))
    goto error;
  rc->score = -1; // still not computed

  return rc;  
 error:  
  logMemory(LOG_ERR, "malloc: cannot create Container");
  return destroyContainer(rc);
}

/*=======================================================================
 * Function   : destroyContainer
 * Description: Destroy an Container by freeing own allocated memory.
 * Synopsis   : void destroyContainer(Container* self)
 * Input      : Container* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a Container.
 =======================================================================*/
Container* 
destroyContainer(Container* self)
{
  if(self == 0) goto error;
 
  // do not free the Archive provided by collection
  self->parents = destroyOnlyRing(self->parents); 

  // free the FromAsso tree (and content) too
  avl_free_tree(self->childs);

  free(self);
 error:
  return (Container*)0;
}


/*=======================================================================
 * Function   : hasExtractionPath
 * Description: check if an IMG's extraction path is already provided
 * Synopsis   : int hasExtractionPath(Archive* self)
 * Input      : Archive* self : what to check
 * Output     : TRUE on success
 =======================================================================*/
static int 
hasExtractionPath(Archive* self)
{
  FromAsso* fromAsso = 0;
  RGIT* curr = 0;

  while ((fromAsso = rgNext_r(self->fromContainers, &curr))) {
    if (fromAsso->container->type == INC ||
	fromAsso->container->type == IMG) continue;
    break;
  }
  
  return (fromAsso != 0);
}

/*=======================================================================
 * Function   : serializeContainer
 * Description: Serialize a Container.
 * Synopsis   : int serializeContainer(Collection* coll, 
 *                                     Container* self, CvsFile* fd)
 * Input      : Collection* coll
 *              Container* self : what to serialize
 *              CvsFile* fd : where to serialize
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeContainer(Collection* coll, Container* self, CvsFile* fd)
{
  int rc = FALSE;
  FromAsso* asso = 0;
  Archive* archive = 0;
  AVLNode *node = 0;

  if(self == 0) {
    logMemory(LOG_INFO, "cannot serialize empty Container");
    goto error;
  }

  if (avl_count(self->childs) < 1) {
    // not blocking but we prefer to stop now
    logMemory(LOG_CRIT, "do not serialize an empty container!");
    goto error;
  }

  fd->print(fd, "\n(%s\n", strEType(self->type));
  fd->doCut = FALSE;
 
  // serialise parents (only INC and IMG container doesn't have parent)
  if (self->type != INC && self->type != IMG) {
    rgRewind(self->parents);
    if ((archive = rgNext(self->parents)) == 0) goto error;
    do {
      if (!serializeExtractRecord(archive, fd)) goto error;
      fd->print(fd, "\n");
    }  while ((archive = rgNext(self->parents)));
  }

  // serialize childs
  if (avl_count(self->childs) > 0) {
    fd->print(fd, "=>\n");

    for (node = self->childs->head; node; node = node->next) {
      asso = node->item;

      // remove safe incoming
      if (self->type == INC && !isIncoming(coll, asso->archive)) continue;

      // remove image when path is available from another rule
      if (self->type == IMG && hasExtractionPath(asso->archive)) continue;
      
      if (!serializeExtractRecord(asso->archive, fd)) goto error;
      fd->print(fd, "\t%s\n", asso->path);
    }
  }

  fd->print(fd, ")\n");
  fd->doCut = TRUE;
  ++env.progBar.cur;
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "serializeContainer fails");
  }
  return(rc);
}


/*=======================================================================
 * Function   : cmpContainerAvl
 * Description: compare two Container
 * Synopsis   : int cmpContainerAvl(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on Container
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int 
cmpContainerAvl(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on items */
  Container* v1 = (Container*)p1;
  Container* v2 = (Container*)p2;

  rc = v1->type - v2->type;

  // manage INC container (which has no parent)
  if (!v1->parent && !v2->parent) return 0;
  if (!v1->parent) return -1;
  if (!v2->parent) return +1;

  if (!rc) rc = cmpArchive(&v1->parent, &v2->parent);
  return rc;
}


/*=======================================================================
 * Function   : serializeExtractRecord
 * Description: Serialize a ExtractRecord.
 * Synopsis   : int serializeExtractRecord(Archive* self, CvsFile* fd)
 * Input      : Archive* self = what to serialize
 *              CvsFile* fd
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeExtractRecord(Archive* self, CvsFile* fd)
{
  int rc = FALSE;

  if (self == 0) {
    logMemory(LOG_WARNING, "do not serialize empty record"); 
    goto error;
  }

  fd->print(fd, "%s:%lli", self->hash, (long long int)self->size);
  rc = TRUE;
 error:
  return rc;
}


/*=======================================================================
 * Function   : createExtractTree
 * Description: Create, by memory allocation a ExtractTree projection.
 * Synopsis   : ExtractTree* createExtractTree(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
ExtractTree* 
createExtractTree(void)
{
  ExtractTree* rc = 0;

  if(!(rc = (ExtractTree*)malloc(sizeof(ExtractTree)))) goto error;    
  memset(rc, 0, sizeof(ExtractTree));

  if (!(rc->containers = 
	avl_alloc_tree(cmpContainerAvl, 
		       (avl_freeitem_t)destroyContainer)))
    goto error;

  // special INC container for newly uploaded files
  if (!(rc->incoming = createContainer())) goto error;
  rc->incoming->type = INC;

  // special IMG container to reminds supports paths (facultative)
  if (!(rc->images = createContainer())) goto error;
  rc->images->type = IMG;

  rc->score = -1;

  return rc;
 error:
  logMemory(LOG_ERR, "malloc: cannot create ExtractTree");
  return destroyExtractTree(rc);
}

/*=======================================================================
 * Function   : destroyExtractTree
 * Description: Destroy a ExtractTree by freeing all the allocate memory.
 * Synopsis   : void destroyExtractTree(ExtractTree* self)
 * Input      : ExtractTree* self = the address of the ExtractTree to
 *              destroy.
 * Output     : Nil address of a ExtractTree.
 =======================================================================*/
ExtractTree* 
destroyExtractTree(ExtractTree* self)
{
  if (!self) goto error;

  avl_free_tree(self->containers);
  self->incoming = destroyContainer(self->incoming);
  self->images = destroyContainer(self->images);

  free(self);

 error:
  return (ExtractTree*)0;
}


/*=======================================================================
 * Function   : serializeExtractTree 
 * Description: Serialize the extraction metadata
 * Synopsis   : int serializeExtractTree(Collection* coll, CvsFile* fd)
 * Input      : Collection* coll = what to serialize
 *              CvsFile* fd = serializer object:
 *              - fd = {0, 0, 0, FALSE, 0, cvsCutOpen, cvsCutPrint};
 *              - fd = {0, 0, 0, FALSE, 0, cvsCatOpen, cvsCatPrint};
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeExtractTree(Collection* coll, CvsFile* fd)
{ 
  int rc = FALSE;
  ExtractTree* self = 0;
  Container* container = 0;
  AVLNode *node = 0;
  int uid = getuid();

  checkCollection(coll);
  if (!(self = coll->extractTree)) goto error;
  logMemory(LOG_DEBUG, "serialize %s extract tree", coll->label);

  if (!fd) goto error;
  fd->nb = 0;
  fd->fd = 0;
  fd->doCut = FALSE;
  fd->offset = 0;

  // we neeed to use the cvs collection directory
  if ((!coll->memoryState & EXPANDED)) {
    logMemory(LOG_ERR, "collection must be expanded first");
    goto error;
  }

  if (!becomeUser(coll->user, TRUE)) goto error;

  // output file
  if (env.dryRun) fd->fd = stdout;
  fd->path = coll->extractDB;
  if (!fd->open(fd)) goto error;

  fd->print(fd, "# MediaTeX extraction metadata: %s\n", coll->label);
  //fd->print(fd, "# Version: $" "Id" "$\n");

  // serialize INC container if not empty
  if (avl_count(self->incoming->childs) > 0) {
    if (!serializeContainer(coll, self->incoming, fd)) goto error;
  }

  // serialize IMG container if not empty
  if (avl_count(self->images->childs) > 0) {
    if (!serializeContainer(coll, self->images, fd)) goto error;
  }

  // serialize all other containers
  if (avl_count(self->containers)) {
    for(node = self->containers->head; node; node = node->next) {
      container = node->item;
      if (!serializeContainer(coll, container, fd)) goto error;
    }
  }
 
  rc = TRUE;
 error:
  if (!cvsClose(fd)) rc = FALSE;
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logMemory(LOG_ERR, "serializeExtractTree fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : addFromArchive
 * Description: Add an archive parents to a container if not already there
 * Synopsis   : int getFromArchive(Collection* coll, 
 *                               Container* container, Archive* archive);
 * Input      : Collection* coll : where to add
 *              Container* container: container having a new parent
 *              Archive* archive : input archive use by the container
 * Output     : TRUE on success
 * Note       : there is at most 1 container by archive
 =======================================================================*/
int 
addFromArchive(Collection* coll, Container* container, Archive* archive)
{
  int rc = FALSE;

  checkCollection(coll);
  if (!container || !archive) goto error;
  logMemory(LOG_DEBUG, "addFromArchive %s/%s:%lli -> %s:%lli",
	    strEType(container->type), 
	    container->type == INC || container->type == IMG?
	    "-":container->parent->hash,
	    container->type == INC || container->type == IMG?
	    0:(long long int)container->parent->size,
	    archive->hash, (long long int)archive->size);

  if (container->type == INC || container->type == IMG) {
    logMemory(LOG_ERR, "%s container cannot have parent",
	      strEType(container->type));
    goto error;
  }
  
  if (container->parents->nbItems == 0) goto next;
  switch (container->type) {
  case ISO:
    logMemory(LOG_ERR, 
	    "ISO containers must only comes from one archive");
    goto error;
  case TGZ:
  case TBZ:
  case AFIO:
  case TAR:
  case CPIO:
  case ZIP:
    logMemory(LOG_ERR, 
	    "multi-volume archive is only implemented for RAR container");
    goto error;
  case GZIP:
  case BZIP:
    logMemory(LOG_ERR, 
	    "compression only container must only comes from one archive");
    goto error;
  default:
    break;
  }
 next:

  // add archive to container's parents ring
  if (rgHaveItem(container->parents, archive)) {
    logMemory(LOG_ERR, "parent already added to the container");
    goto error;
  }
  if (!rgInsert(container->parents, archive)) goto error;

  // link container to archive
  if (archive->toContainer) {
    logMemory(LOG_ERR, "archive already comes from %s/%s:%lli container",
	      strEType(archive->toContainer->type),
	      archive->toContainer->parent->hash, 
	      (long long int)archive->toContainer->parent->size);
    goto error;
  }
  archive->toContainer = container;

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addFromArchive fails");
    if (container && archive) delFromArchive(coll, container, archive);
  }
  return rc;
}

/*=======================================================================
 * Function   : delFromArchive
 * Description: Del an archive parent from a container
 * Synopsis   : int getFromArchive(Collection* coll, 
 *                               Container* container, Archive* archive);
 * Input      : Collection* coll : where to del
 *              Container* container: container having a new parent
 *              Archive* archive : input archive use by the container
 * Output     : TRUE on success
 * Note       : not call by delContainer (else infinite loop)
 =======================================================================*/
int 
delFromArchive(Collection* coll, Container* container, Archive* archive)
{
  int rc = FALSE;
  RGIT* curr = 0;

  checkCollection(coll);
  if (!container || !archive) goto error;
  logMemory(LOG_DEBUG, "delFromArchive %s/%s:%lli -> %s:%lli",
	    strEType(container->type), 
	    container->type == INC || container->type == IMG?
	    "-":container->parent->hash,
	    container->type == INC || container->type == IMG?
	    0:(long long int)container->parent->size,
	    archive->hash, (long long int)archive->size);

  // del archive to container's parents ring
  if ((curr = rgHaveItem(container->parents, archive))) {
    rgRemove_r(container->parents, &curr);
  }

  // unlink container from archive
  archive->toContainer = 0;
  
  // del container if archive was the first parent (its primary key)
  if (archive == container->parent) {
    if (!delContainer(coll, container)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delFromArchive fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getFromAsso
 * Description: Search a fromAsso into a Container
 * Synopsis   : fromAsso* getFromAsso(Container* container, 
 *                                    Archive* parent)
 * Input      : Container* container: where to find
 *              Archive* archive: content to find 
 * Output     : The address of the fromAsso.
 =======================================================================*/
FromAsso* 
getFromAsso(Container* container, Archive* archive)
{
  FromAsso* rc = 0;
  FromAsso asso;
  AVLNode* node = 0;

  checkContainer(container);
  checkArchive(archive);
  logMemory(LOG_DEBUG, "%s", "getFromAsso");

  // look for fromAsso
  asso.archive = archive;
  asso.container = container;
  if ((node = avl_search(container->childs, &asso))) {
    rc = (FromAsso*)node->item;
  }
 error:
  return rc;
}

/*=======================================================================
 * Function   : addFromAsso
 * Description: Add a fromAsso if not already there
 * Synopsis   : FromAsso* addFromAsso(Collection* coll, Archive* archive
 *                                    Container* container)
 * Input      : Collection* coll : where to add
 *              Archive* archive : id
 *              Container* container : id
 *              char* path : path to file into the container
 * Output     : FromAsso* : the FromAsso we have found/added
 * Note       : archive from container may have multiple instances
 *              (as an archive may comes from severals containers)
 =======================================================================*/
FromAsso* 
addFromAsso(Collection* coll, Archive* archive, Container* container,
	    char* path)
{
  FromAsso* rc = 0;
  FromAsso* asso = 0;
  char* string = 0;
  struct tm date;
  time_t time = 0;

  checkCollection(coll);
  if (!container || !archive) goto error;
  logMemory(LOG_DEBUG, "addFromAsso %s:%lli -> %s/%s:%lli",
	    archive->hash, (long long int)archive->size,
	    strEType(container->type), 
	    container->type == INC || container->type == IMG?
	    "-":container->parent->hash,
	    container->type == INC || container->type == IMG?
	    0:(long long int)container->parent->size);

  switch (container->type) {
  case GZIP:
  case BZIP:
    if (avl_count(container->childs) > 0) {
      logMemory(LOG_ERR, 
	      "compression only container must only provides one archive");
      goto error;
    }    
    break;

  case INC:
    // having 1 inc asso and 1 normal asso => remove the inc asso
    if (archive->fromContainers->nbItems > 0) {
      logMemory(LOG_NOTICE, 
		"%s:%lli file is now archived (no more an incoming)",
		archive->hash, (long long int)archive->size);
      asso = archive->fromContainers->head->it;
      goto end;
    }

    if (sscanf(path, "%d-%d-%d,%d:%d:%d",
	       &date.tm_year, &date.tm_mon, &date.tm_mday,
	       &date.tm_hour, &date.tm_min, &date.tm_sec)
	!= 6) {
      logCommon(LOG_ERR, "sscanf: error parsing date '%s'", path);
      goto error;
    }

    date.tm_year -= 1900; // from GNU/Linux burning date
    date.tm_mon -= 1;     // month are managed from 0 to 11 
    date.tm_isdst = -1;   // no information available about spring date
    if ((time = mktime(&date)) == -1) goto error;

    // having 2 incoming asso we take the oldest date
    if (!archive->uploadTime || archive->uploadTime > time) {
      archive->uploadTime = time;
    }

    break;
    
  case IMG:
    // ignore having several IMG assos if they exacly match
    if ((asso = getFromAsso(container, archive))) {
      if (strcmp(asso->path, path)) {
	logMemory(LOG_ERR,
		  "cannot add several IMG asso with different paths"
		  " (%s:%lli)",
		  archive->hash, (long long int)archive->size);
	asso = 0;
	goto error;
      }
      goto end;
    }
    break;
    
  default:
    // having 1 inc asso and 1 normal asso => remove the inc asso
    if (archive->uploadTime) {
      logMemory(LOG_NOTICE, 
		"%s:%lli file is now archived (no more an incoming)", 
		archive->hash, (long long int)archive->size);
      archive->uploadTime = 0;
    }
    break;
  }

  // add new content
  if (!(string = createString(path))) goto error;
  if (!(asso = createFromAsso())) goto error;
  asso->archive = archive;
  asso->container = container;
  asso->path = string;

  // incomings do not provides extraction path, and both
  // incomings and images are not linked to archive
  if (container->type != INC && container->type != IMG) {
    if (!rgInsert(archive->fromContainers, asso)) goto error;
  }

  // only provide archives once by container
  if (!avl_insert(container->childs, asso)) {
    if (errno != EEXIST) {
      logMemory(LOG_ERR, "fails to add add %s:%lli content",
		archive->hash, (long long int)archive->size);
    }
    else {
      logMemory(LOG_ERR, "cannot add %s:%lli (already there)",
		archive->hash, (long long int)archive->size);
    }
    goto error;
  }
 end:
  rc = asso;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addFromAsso fails");
    if (asso) delFromAsso(coll, asso);
  }
  return rc;
}

/*=======================================================================
 * Function   : delFromAsso
 * Description: Del an fromAsso if not already there
 * Synopsis   : void delFromAsso(Collection* coll, FromAsso* self)
 * Input      : Collection* coll: where to del
 *              FromAsso* self: association to del
 * Output     : TRUE on success
 =======================================================================*/
int
delFromAsso(Collection* coll, FromAsso* self)
{
  int rc = FALSE;
  RGIT* curr = 0;
 
  checkCollection(coll);
  if (!self) goto error;
  logMemory(LOG_DEBUG, "delFromAsso %s:%lli -> %s/%s:%lli",
	    self->archive->hash, (long long int)self->archive->size,
	    strEType(self->container->type), 
	    self->container->type == INC || self->container->type == IMG?
	    "-":self->container->parent->hash,
	    self->container->type == INC || self->container->type == IMG?
	    0:(long long int)self->container->parent->size);

  // incomings do not provides extraction path
  if (self->container->type == INC) {
    self->archive->uploadTime = 0;
  }
  else {
    // delete asso from archive
    if ((curr = rgHaveItem(self->archive->fromContainers, self))) {
      rgRemove_r(self->archive->fromContainers, &curr);
    }
  }

  // delete asso from container and free the fromAsso
  avl_delete(self->container->childs, self);
    
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delFromAsso fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getContainer
 * Description: Search a Container
 * Synopsis   : Container* getContainer(Collection* coll, 
 *                                        EType type, Archive* parent)
 * Input      : Collection* coll: where to find
 *              EType type: first id
 *              Archive* parent : second id
 * Output     : The address of the Container.
 =======================================================================*/
Container* 
getContainer(Collection* coll, EType type, Archive* parent)
{
  Container* rc = 0;
  Container container;
  AVLNode* node = 0;

  checkCollection(coll);
  checkArchive(parent);
  logMemory(LOG_DEBUG, "getContainer %s", strEType(type));

  // look for container
  container.type = type;
  container.parent = parent;
  if ((node = avl_search(coll->extractTree->containers, &container))) {
    rc = (Container*)node->item;
  }
 error:
  return rc;
}


/*=======================================================================
 * Function   : addContainer
 * Description: Add a new extraction container
 * Synopsis   : Container* addContainer(Collection* coll, EType type,
                           Archive* parent)
 * Input      : Collection* coll : where to add
 *              EType type : type of the new container
 *              Archive* parent : first parent
 * Output     : Container* : the container we have just add
 =======================================================================*/
Container*
addContainer(Collection* coll, EType type, Archive* parent)
{
  Container* rc = 0;
  Container* container = 0;

  checkCollection(coll);
  checkArchive(parent);
  logMemory(LOG_DEBUG, "addContainer %s", strEType(type));

  if (type == INC || type == IMG) {
    logMemory(LOG_ERR, "%s container always exists and cannot be add",
	      strEType(type));
    goto error;
  }

  // already there
  if ((container = getContainer(coll, type, parent))) {
    logMemory(LOG_ERR, "container already there");
    container = 0;
    goto error;
  }

  // add new one
  if ((container = createContainer()) == 0) goto error;
  container->type = type;
  container->parent = parent;
  if (!addFromArchive(coll, container, parent)) goto error;
  if (!avl_insert(coll->extractTree->containers, container)) {
    if (errno != EEXIST) {
      logMemory(LOG_ERR, "fails to add %s container", strEType(type))
    }
    else {
      logMemory(LOG_ERR, "cannot add %i container (already there)",
		strEType(type))
    }
    goto error;
  }
  
  rc = container;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addContainer fails");
    container = destroyContainer(container);
  }
  return rc;
}


/*=======================================================================
 * Function   : delContainer
 * Description: Del an extraction container
 * Synopsis   : int delContainer(Collection* coll, Container* self)
 * Input      : Collection* coll: where to del
 *              Container* self: the container to delete
 * Output     : TRUE on success
 =======================================================================*/
int
delContainer(Collection* coll, Container* self)
{
  int rc = FALSE;
  Archive* arch = 0;
  AVLNode *node = 0;

  checkCollection(coll);
  if (!self) goto error;
  logMemory(LOG_DEBUG, "delContainer %s/%s:%lli", 
	    strEType(self->type), 
	    self->type == INC || self->type == IMG?
	    "-":self->parent->hash,
	    self->type == INC || self->type == IMG?
	    0:(long long int)self->parent->size);

  // delete content associations to archives
  while ((node = self->childs->head)) {
    if (!delFromAsso(coll, node->item)) goto error;
  }

  // delete container link with archives
  while ((arch = rgHead(self->parents))) {
    arch->toContainer = 0;
    rgRemove(self->parents);
  }

  // delete container from extract tree and free it
  avl_delete(coll->extractTree->containers, self);

  rc = TRUE;
 error:
  return rc;
}

/*=======================================================================
 * Function   : diseaseExtractTree
 * Description: Disease a ExtractTree by freeing all the allocate memory.
 * Synopsis   : void diseaseExtractTree(ExtractTree* self)
 * Input      : ExtractTree* self = the address of the ExtractTree to
 *              disease.
 * Output     : TRUE on success
 =======================================================================*/
int
diseaseExtractTree(Collection* coll)
{
  int rc = FALSE;
  AVLNode *node = 0;

  if(coll == 0) goto error;
  logMemory(LOG_DEBUG, "diseaseExtractTree %s", coll);

  // diseases containers
  if (!delContainer(coll, coll->extractTree->incoming)) goto error;
  if (!delContainer(coll, coll->extractTree->images)) goto error;
  while ((node = coll->extractTree->containers->head))
    if (!delContainer(coll, node->item)) goto error;

  // force scores to be re-computed next
  coll->extractTree->score = -1;

  // try to disease archives
  if (!diseaseArchives(coll)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "diseaseExtractTree fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

