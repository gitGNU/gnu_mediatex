/*=======================================================================
 * Version: $Id: extractTree.c,v 1.1 2014/10/13 19:39:10 nroche Exp $
 * Project: MediaTeX
 * Module : extraction tree
 *
 * Extraction producer interface

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Nicolas Roche
 
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

#include "../mediatex.h"
#include "../misc/log.h"
#include "strdsm.h"
#include "extractTree.h"

#include <avl.h>


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
    {"UNDEF", "REC", "ISO", "CAT", 
     "TGZ", "TBZ", "AFIO",
     "TAR", "CPIO", 
     "GZIP", "BZIP",
     "ZIP", "RAR"};

  if (self < 0 || self >= ETYPE_MAX) {
    logEmit(LOG_WARNING, "unknown extract type %i", self);
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
  if (!strcasecmp(label, "REC")) return REC;
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
  FromAsso* rc = NULL;

  if ((rc = (FromAsso*)malloc(sizeof(FromAsso))) == NULL)
    goto error;

  memset(rc, 0, sizeof(FromAsso));
 
  return rc;  
 error:  
  logEmit(LOG_ERR, "%s", "malloc: cannot create FromAsso");
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
  if (self == NULL) goto error;
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

  rc = cmpArchive2(v1->archive, v2->archive);
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
  Container* rc = NULL;

  if ((rc = (Container*)malloc(sizeof(Container))) == NULL)
    goto error;

  memset(rc, 0, sizeof(Container));
  if ((rc->parents = createRing()) == NULL) goto error;
  if (!(rc->childs = 
	avl_alloc_tree(cmpFromAssoAvl, 
		       (avl_freeitem_t)destroyFromAsso)))
    goto error;
  rc->score = -1; // still not comuted

  return rc;  
 error:  
  logEmit(LOG_ERR, "%s", "malloc: cannot create Container");
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
  if(self == NULL) goto error;
 
  // do not free the Archive provided by collection
  self->parents = destroyOnlyRing(self->parents); 

  // free the FromAsso tree (and content) too
  avl_free_tree(self->childs);

  free(self);
 error:
  return (Container*)0;
}


/*=======================================================================
 * Function   : serializeContainer
 * Description: Serialize a Container.
 * Synopsis   : int serializeContainer(Container* self, CvsFile* fd)
 * Input      : Container* self : what to serialize
 *              CvsFile* fd : where to serialize
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeContainer(Container* self, CvsFile* fd)
{
  int rc = FALSE;
  FromAsso* asso = NULL;
  Archive* archive = NULL;
  AVLNode *node = NULL;

  if(self == NULL) {
    logEmit(LOG_INFO, "%s", "cannot serialize empty Container");
    goto error;
  }

  cvsPrint(fd, "\n(%s\n", strEType(self->type));
  fd->doCut = FALSE;
 
  rgRewind(self->parents);
  if ((archive = rgNext(self->parents)) == NULL) goto error;
  do {
    if (!serializeExtractRecord(archive, fd)) goto error;
    cvsPrint(fd, "\n");
  }  while ((archive = rgNext(self->parents)) != NULL);

  if (avl_count(self->childs) > 0) {
    cvsPrint(fd, "=>\n");

    for(node = self->childs->head; node; node = node->next) {
      asso = node->item;
      if (!serializeExtractRecord(asso->archive, fd)) goto error;
      cvsPrint(fd, "\t%s\n", asso->path);
    }
  }

  cvsPrint(fd, ")\n");
  fd->doCut = TRUE;
  ++env.progBar.cur;
  rc = TRUE;
 error:
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

  if (self == NULL) {
    logEmit(LOG_WARNING, "%s", "do not serialize empty record"); 
    goto error;
  }

  cvsPrint(fd, "%s:%lli", self->hash, (long long int)self->size);
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
  ExtractTree* rc = NULL;

  if(!(rc = (ExtractTree*)malloc(sizeof(ExtractTree)))) goto error;    
  memset(rc, 0, sizeof(ExtractTree));

  if (!(rc->containers = 
	avl_alloc_tree(cmpContainerAvl, 
		       (avl_freeitem_t)destroyContainer)))
    goto error;
  rc->score = -1;

  return rc;
 error:
  logEmit(LOG_ERR, "%s", "malloc: cannot create ExtractTree");
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
  free(self);

 error:
  return (ExtractTree*)0;
}


/*=======================================================================
 * Function   : serializeExtractTree 
 * Description: Serialize a catalog to latex working place
 * Synopsis   : int serializeExtractTree(Collection* coll)
 * Input      : Collection* coll = what to serialize
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeExtractTree(Collection* coll)
{ 
  int rc = FALSE;
  CvsFile fd = {NULL, 0, NULL, FALSE, 0};
  ExtractTree* self = NULL;
  Container* container = NULL;
  AVLNode *node = NULL;
  int uid = getuid();

  checkCollection(coll);
  if (!(self = coll->extractTree)) goto error;
  logEmit(LOG_DEBUG, "serialize %s extract tree", coll->label);

  // we neeed to use the cvs collection directory
  if ((!coll->memoryState & EXPANDED)) {
    logEmit(LOG_ERR, "%s", "collection must be expanded first");
    goto error;
  }

  if (!becomeUser(coll->user, TRUE)) goto error;

  // output file
  if (env.dryRun) fd.fd = stdout;
  fd.path = coll->extractDB;
  if (!cvsOpenFile(&fd)) goto error;

  cvsPrint(&fd, "# MediaTeX extraction metadata: %s\n", coll->label);
  //cvsPrint(&fd, "# Version: $" "Id" "$\n");

  if (avl_count(self->containers)) {
    for(node = self->containers->head; node; node = node->next) {
      container = node->item;
      if (!serializeContainer(container, &fd)) goto error;
    }
  }
 
  rc = TRUE;
 error:
  if (!cvsCloseFile(&fd)) rc = FALSE;
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeExtractTree fails");
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
  logEmit(LOG_DEBUG, "addFromArchive %s/%s:%lli -> %s:%lli",
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size,
	  archive->hash, (long long int)archive->size);

  if (container->parents->nbItems == 0) goto next;
  switch (container->type) {
  case REC:
  case ISO:
    logEmit(LOG_ERR, "%s", 
	    "REC and ISO containers must only comes from one archive");
    goto error;
  case TGZ:
  case TBZ:
  case AFIO:
  case TAR:
  case CPIO:
  case ZIP:
    logEmit(LOG_ERR, "%s", 
	    "multi-volume archive is only implemented for RAR container");
    goto error;
  case GZIP:
  case BZIP:
    logEmit(LOG_ERR, "%s", 
	    "compression only container must only comes from one archive");
    goto error;
  default:
    break;
  }
 next:

// add archive to container's parents ring
  if (rgHaveItem(container->parents, archive)) {
    logEmit(LOG_ERR, "%s", "parent already added to the container");
    goto error;
  }
  if (!rgInsert(container->parents, archive)) goto error;

  // link container to archive
  if (archive->toContainer) {
    logEmit(LOG_ERR, "%s", "archive already comes from another container");
    goto error;
  }
  archive->toContainer = container;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "addFromArchive fails");
    if (container && archive) delFromArchive(coll, container, archive);
  }
  return rc;
}

/*=======================================================================
 * Function   : delFromArchive
 * Description: Del an archive prents to a container
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
  RGIT* curr = NULL;

  checkCollection(coll);
  if (!container || !archive) goto error;
  logEmit(LOG_DEBUG, "delFromArchive %s/%s:%lli -> %s:%lli",
	  strEType(container->type), container->parent->hash, 
	  (long long int)container->parent->size,
	  archive->hash, (long long int)archive->size);

  // del archive to container's parents ring
  if ((curr = rgHaveItem(container->parents, archive))) {
    rgRemove_r(container->parents, &curr);
  }

  // unlink container from archive
  archive->toContainer = NULL;
  
  // del container if archive was the first parent (its primary key)
  if (archive == container->parent) {
    if (!delContainer(coll, container)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "delFromArchive fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : addFromAsso
 * Description: Add an fromAsso if not already there
 * Synopsis   : FromAsso* getFromAsso(Collection* coll, Archive* archive
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
  FromAsso* rc = NULL;
  FromAsso* asso = NULL;
  char* string = NULL;
  
  checkCollection(coll);
  if (!container || !archive) goto error;
  logEmit(LOG_DEBUG, "addFromAsso %s:%lli -> %s/%s:%lli",
	  archive->hash, (long long int)archive->size,
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size);

  switch (container->type) {
  case REC:
    logEmit(LOG_ERR, "%s", 
	    "REC containers must not contains archive");
    goto error;
  case GZIP:
  case BZIP:
    if (avl_count(container->childs) > 0) {
      logEmit(LOG_ERR, "%s", 
	      "compression only container must only provides one archive");
      goto error;
    }
  default:
    break;
  }

  // add new one
  if (!(string = createString(path))) goto error;
  if (!(asso = createFromAsso())) goto error;
  asso->archive = archive;
  asso->container = container;
  asso->path = string;

  if (!rgInsert(archive->fromContainers, asso)) goto error;
  if (!avl_insert(container->childs, asso)) goto error;

  rc = asso;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "addFromAsso fails");
    if (asso) delFromAsso(coll, asso, TRUE);
  }
  return rc;
}

/*=======================================================================
 * Function   : delFromAsso
 * Description: Del an fromAsso if not already there
 * Synopsis   : void delFromAsso(Collection* coll, FromAsso* self)
 * Input      : Collection* coll: where to del
 *              FromAsso* self: association to del
 *              int doUnlinkContainer: FALSE if job done by delContainer
 * Output     : TRUE on success
 =======================================================================*/
int
delFromAsso(Collection* coll, FromAsso* self, int doUnlinkContainer)
{
  int rc = FALSE;
  RGIT* curr = NULL;
 
  checkCollection(coll);
  if (!self) goto error;
  logEmit(LOG_DEBUG, "delFromAsso %s:%lli -> %s/%s:%lli",
	  self->archive->hash, (long long int)self->archive->size,
	  strEType(self->container->type), self->container->parent->hash,
	  (long long int)self->container->parent->size);

  // delete asso from archive
  if ((curr = rgHaveItem(self->archive->fromContainers, self))) {
    rgRemove_r(self->archive->fromContainers, &curr);
  }

  // delete asso from container and free the fromAsso
  if (doUnlinkContainer) {
    avl_delete(self->container->childs, self);
  }
    
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "delFromAsso fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getContainer
 * Description: Search or may create a Container
 * Synopsis   : Container* getContainer(Collection* coll, 
 *                                        EType type, Archive* parent)
 * Input      : Collection* coll: where to find
 *              EType type : first id
 *              Archive* parent : second id
 * Output     : The address of the Container.
 =======================================================================*/
Container* 
getContainer(Collection* coll, EType type, Archive* parent)
{
  Container* rc = NULL;
  Container container;
  AVLNode* node = NULL;

  checkCollection(coll);
  checkArchive(parent);
  logEmit(LOG_DEBUG, "getContainer %s", strEType(type));

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
 * Synopsis   : Container* addContainer(Collection* coll, EType type)
 * Input      : Collection* coll : where to add
 *              EType type : type of the new container
 * Output     : Container* : the container we have just add
 =======================================================================*/
Container*
addContainer(Collection* coll, EType type, Archive* parent)
{
  Container* rc = NULL;
  Container* container = NULL;

  checkCollection(coll);
  checkArchive(parent);
  logEmit(LOG_DEBUG, "addContainer %s", strEType(type));

  // already there
  if ((container = getContainer(coll, type, parent))) {
    logEmit(LOG_ERR, "%s", "container already there");
    goto error;
  }

  // add new one
  if ((container = createContainer()) == NULL) goto error;
  container->type = type;
  container->parent = parent;
  if (!addFromArchive(coll, container, parent)) goto error;
  if (!avl_insert(coll->extractTree->containers, container)) goto error;

  rc = container;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "addContainer fails");
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
  Archive* arch = NULL;
  AVLNode *node = NULL;

  checkCollection(coll);
  if (!self) goto error;
  logEmit(LOG_DEBUG, "delContainer %s/%s:%lli", 
	  strEType(self->type), self->parent->hash,
	  (long long int)self->parent->size);

  // delete content associations to archives
  for(node = self->childs->head; node; node = node->next) {
    if (!delFromAsso(coll, node->item, FALSE)) goto error;
  }

  // delete container link with archives
  while((arch = rgHead(self->parents))) {
    arch->toContainer = NULL;
    rgDelete(self->parents);
  }

  // delete container from catalog tree and free it
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
  AVLNode *node = NULL;

  if(coll == NULL) goto error;
  logEmit(LOG_DEBUG, "diseaseExtractTree %s", coll);

  // diseases containers
  while ((node = coll->extractTree->containers->head))
    if (!delContainer(coll, node->item)) goto error;

  // force scores to be re-computed next
  coll->extractTree->score = -1;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "diseaseExtractTree fails");
  }
  return rc;
}


/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "utFunc.h"
GLOBAL_STRUCT_DEF;


/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually argv[0].
 * Output     : N/A
 =======================================================================*/
static void 
usage(char* programName)
{
  memoryUsage(programName);

  memoryOptions();
  //fprintf(stderr, "\t\t---\n");
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for confTree module.
 * Synopsis   : utconfTree
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = NULL;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MEMORY_SHORT_OPTIONS;
  struct option longOptions[] = {
    MEMORY_LONG_OPTIONS,
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MEMORY_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!createExempleConfiguration()) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!createExempleExtractTree(coll)) goto error;
  
  // test serializing
  if (!serializeExtractTree(coll)) {
    logEmit(LOG_ERR, "%s", "Error while serializing the extract exemple");
    goto error;
  }
  
  // test disease
  if (!diseaseExtractTree(coll)) goto error;
  env.dryRun = TRUE;
  if (!serializeExtractTree(coll)) goto error;
  if (avl_count(coll->extractTree->containers) != 0) goto error;
  /************************************************************************/

  freeConfiguration();
  rc = TRUE;
 error:
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */

