/*=======================================================================
 * Project: MediaTeX
 * Module : admCatalogTree
 *
 * Catalog producer interface

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

void* avl_insert_2(AVLTree* tree, void* item)
{
  return avl_insert(tree, item);
}


/*=======================================================================
 * Function   : caracType2string
 * Description: get the type as a string to be displayed
 * Synopsis   : char* caracType2string(CType type)
 * Input      : CaracType type: type of carac
 * Output     : The correspondaing string
 =======================================================================*/
char*
strCType(CType self)
{
  static char* typeLabels[CTYPE_MAX] =
    {"CATE", "DOC", "HUM", "ARCH", "ROLE"};

  if (self < 0 || self >= CTYPE_MAX) {
    logMemory(LOG_WARNING, "unknown catalog type %i", self);
    return "???";
  }

  return typeLabels[self];
}


/*=======================================================================
 * Function   : createCarac
 * Description: Create, by memory allocation a Carac
 * Synopsis   : Carac* createCarac(void)
 * Input      : CaracType type: type of carac
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Carac* 
createCarac()
{
  Carac* rc = 0;

  if ((rc = (Carac*)malloc(sizeof(Carac))) == 0) {
    logMemory(LOG_ERR, "malloc: cannot create a Carac");
    goto error;
  }

  memset(rc, 0, sizeof(Carac));

  return rc;  
 error:  
  return destroyCarac(rc);
}

/*=======================================================================
 * Function   : destroyCarac
 * Description: Destroy a Carac by freeing all the allocate memory.
 * Synopsis   : void destroyCarac(Carac* self)
 * Input      : Carac* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a Carac.
 =======================================================================*/
Carac* 
destroyCarac(Carac* self)
{
  Carac* rc = 0;

  if(self) {
    self->label = destroyString(self->label);
    free(self);
  }

  return(rc);
}

/*=======================================================================
 * Function   : cmpCarac
 * Description: compare two caracs
 * Synopsis   : int cmpCarac(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on Carac
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int 
cmpCarac(const void *p1, const void *p2)
{
  /* p1 and p2 are pointers on &items */
  Carac* v1 = *((Carac**)p1);
  Carac* v2 = *((Carac**)p2);

  return strcmp(v1->label, v2->label);
}


/*=======================================================================
 * Function   : createAssoCarac
 * Description: Create, by memory allocation a AssoCarac
 * Synopsis   : AssoCarac* createAssoCarac(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
AssoCarac* 
createAssoCarac()
{
  AssoCarac* rc = 0;

  if ((rc = (AssoCarac*)malloc(sizeof(AssoCarac))) == 0) {
    logMemory(LOG_ERR, "malloc: cannot create AssoCarac");
    goto error;
  }

  memset(rc, 0, sizeof(AssoCarac));
  return rc;
  
 error:  
  return destroyAssoCarac(rc);
}

/*=======================================================================
 * Function   : destroyAssoCarac
 * Description: Destroy a AssoCarac by freeing all the allocate memory.
 * Synopsis   : void destroyAssoCarac(AssoCarac* self)
 * Input      : AssoCarac* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a AssoCarac.
 =======================================================================*/
AssoCarac* 
destroyAssoCarac(AssoCarac* self)
{
  AssoCarac* rc = 0;

  if(self) {
    self->value = destroyString(self->value);
    free(self);
  }

  return(rc);
}

/*=======================================================================
 * Function   : cmpAssoCarac
 * Description: compare two assoCaracs
 * Synopsis   : int cmpAssoCarac(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on AssoCarac
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int 
cmpAssoCarac(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items */
  AssoCarac* v1 = *((AssoCarac**)p1);
  AssoCarac* v2 = *((AssoCarac**)p2);

  rc = cmpCarac(&v1->carac, &v2->carac);
  if (!rc) rc = strcmp(v1->value, v2->value);
  return rc;
}

/*=======================================================================
 * Function   : serializeAssoCarac
 * Description: Serialize a AssoCarac.
 * Synopsis   : int serializeAssoCarac(AssoCarac* self, CvsFile* fd)
 * Input      : AssoCarac* self = what to serialize
 *              FILE fd = where to serialize
 * Output     : TRUE on success
 =======================================================================*/
int
serializeAssoCarac(AssoCarac* self, CvsFile* fd)
{
  int rc = FALSE;

  if(self == 0) goto error;
  logMemory(LOG_DEBUG, "serialize AssoCarac: %s/%s", 
	  self->carac->label, self->value);

  fd->print(fd, "  \"%s\" = \"%s\"\n", self->carac->label, self->value);

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "cannot serialize empty AssoCarac");
  }
  return(rc);
}


/*=======================================================================
 * Function   : cmpAssoRole
 * Description: compare two assoRoles
 * Synopsis   : int cmpAssoRole(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on AssoRole**
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int 
cmpAssoRole(const void *p1, const void *p2)
{
  int rc = 0;
  
  /* p1 and p2 are pointers on &items */
  AssoRole* v1 = *((AssoRole**)p1);
  AssoRole* v2 = *((AssoRole**)p2);

  rc = cmpRole(&v1->role, &v2->role);
  if (!rc) rc = cmpHuman(&(v1->human), &(v2->human));
  if (!rc) rc = cmpDocument(&(v1->document), &(v2->document));
  return rc;
}

/*=======================================================================
 * Function   : cmpAssoRoleAvl
 * Description: compare two assoRoles
 * Synopsis   : int cmpAssoRoleAvl(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on AssoRole*
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int 
cmpAssoRoleAvl(const void *p1, const void *p2)
{
  int rc = 0;
  
  /* p1 and p2 are pointers on &items */
  const AssoRole* v1 = p1;
  const AssoRole* v2 = p2;

  rc = cmpRole(&v1->role, &v2->role);
  if (!rc) rc = cmpHuman(&(v1->human), &(v2->human));
  if (!rc) rc = cmpDocument(&(v1->document), &(v2->document));
  return rc;
}

/*=======================================================================
 * Function   : createRole
 * Description: Create, by memory allocation a Role
 * Synopsis   : Role* createRole(void)
 * Input      : RoleType type: type of role
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Role* 
createRole(void)
{
  Role* rc = 0;

  if ((rc = (Role*)malloc(sizeof(Role))) == 0) {
    logMemory(LOG_ERR, "malloc: cannot create a Role");
    goto error;
  }

  memset(rc, 0, sizeof(Role));
  if (!(rc->assos 
	= avl_alloc_tree(cmpAssoRoleAvl, 
			 (avl_freeitem_t)destroyAssoRole)))
    goto error;
  
  return rc;  
 error:  
  return destroyRole(rc);
}

/*=======================================================================
 * Function   : destroyRole
 * Description: Destroy a Role by freeing all the allocate memory.
 * Synopsis   : void destroyRole(Role* self)
 * Input      : Role* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a Role.
 =======================================================================*/
Role* 
destroyRole(Role* self)
{
  Role* rc = 0;

  if(self) {
    self->label = destroyString(self->label);
    
    // do destroy the assoRole too
    avl_free_tree(self->assos);
    free(self);
  }

  return(rc);
}

/*=======================================================================
 * Function   : cmpRole
 * Description: compare two roles
 * Synopsis   : int cmpRole(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on Role
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int 
cmpRole(const void *p1, const void *p2)
{
  /* p1 and p2 are pointers on &items */
  Role* v1 = *((Role**)p1);
  Role* v2 = *((Role**)p2);

  return strcmp(v1->label, v2->label);
}


/*=======================================================================
 * Function   : createAssoRole
 * Description: Create, by memory allocation a AssoRole
 * Synopsis   : AssoRole* createAssoRole(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
AssoRole* 
createAssoRole()
{
  AssoRole* rc = 0;

  if ((rc = (AssoRole*)malloc(sizeof(AssoRole))) == 0) {
    logMemory(LOG_ERR, "malloc: cannot create AssoRole");
    goto error;
  }

  memset(rc, 0, sizeof(AssoRole));
  return rc;

 error:  
  return destroyAssoRole(rc);
}

/*=======================================================================
 * Function   : destroyAssoRole
 * Description: Destroy a AssoRole by freeing all the allocate memory.
 * Synopsis   : void destroyAssoRole(AssoRole* self)
 * Input      : AssoRole* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a AssoRole.
 =======================================================================*/
AssoRole* 
destroyAssoRole(AssoRole* self)
{
  AssoRole* rc = 0;

  if(self) {
    free(self);
  }

  return(rc);
}

/*=======================================================================
 * Function   : serializeAssoRole
 * Description: Serialize a AssoRole.
 * Synopsis   : int serializeAssoRole(AssoRole* self, CvsFile* fd)
 * Input      : AssoRole* self = what to serialize
 *              FILE fd = where to serialize
 * Output     : TRUE on success
 =======================================================================*/
int
serializeAssoRole(AssoRole* self, CvsFile* fd)
{
  int rc = FALSE;

  logMemory(LOG_DEBUG, "serialize AssoRole: %s", self->role->label);
  if(self == 0) goto error;

  fd->print(fd, "  With \"%s\" = \"%s\" \"%s\"\n", 
	  self->role->label, 
	  self->human->firstName, self->human->secondName);

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "cannot serialize empty AssoRole");
  }
  return(rc);
}

/*=======================================================================
 * Function   : serializeCatalogArchive
 * Description: Serialize an archive.
 * Synopsis   : int serializeCatalogArchive(CRecord* self, CvsFile* fd)
 * Input      : Archive* self = what to serialize
 *              FILE fd = where to serialize
 * Output     : TRUE on success
 =======================================================================*/
int
serializeCatalogArchive(Archive* self, CvsFile* fd)
{
  int rc = FALSE;
  AssoCarac *assoCarac = 0;

  checkArchive(self);
  logMemory(LOG_DEBUG, "serialize archive: %s:%lli", 
	    self->hash, self->size);

  fd->print(fd, "Archive\t %s:%lli\n", self->hash, self->size);
  fd->doCut = FALSE;

  // serialize assoCaracs
  if (!isEmptyRing(self->assoCaracs)) {
    rgSort(self->assoCaracs, cmpAssoCarac);
    rgRewind(self->assoCaracs);
    while ((assoCarac = rgNext(self->assoCaracs))) {
      if (!serializeAssoCarac(assoCarac, fd)) goto error;
    }
  }

  fd->print(fd, "\n");
  
  fd->doCut = TRUE;
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "cannot serialize empty CRecord");
  }
  return(rc);
}


/*=======================================================================
 * Function   : createHuman
 * Description: Create, by memory allocation a Human
 * Synopsis   : Human* createHuman(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Human*
createHuman(void)
{
  Human* rc = 0;

  if ((rc = (Human*)malloc(sizeof(Human))) == 0) goto error;
  memset(rc, 0, sizeof(Human));

  if ((rc->categories = createRing()) == 0 ||
      (rc->assoCaracs = createRing()) == 0 ||
      (rc->assoRoles = createRing()) == 0)
    goto error;

  return rc;
 error:
  logMemory(LOG_ERR, "malloc: cannot create Human");
  return destroyHuman(rc);
}

/*=======================================================================
 * Function   : destroyHuman
 * Description: Destroy a Human by freeing all the allocate memory.
 * Synopsis   : void destroyHuman(Human* self)
 * Input      : Human* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a Human.
 =======================================================================*/
Human*
destroyHuman(Human* self)
{
  Human* rc = 0;

  if(self) {
    self->firstName  = destroyString(self->firstName);
    self->secondName = destroyString(self->secondName);

    // delete assoCarac associations
    self->assoCaracs
      = destroyRing(self->assoCaracs,
		    (void*(*)(void*)) destroyAssoCarac);

    // do not destroy childs
    self->categories = destroyOnlyRing(self->categories);
    self->assoRoles = destroyOnlyRing(self->assoRoles);
    free(self);
  }

  return(rc);
}

/*=======================================================================
 * Function   : cmpHuman
 * Description: compare two Humans alphabetically
 * Synopsis   : int cmpHuman(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on Human
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int cmpHuman(const void *p1, const void *p2)
{
  int rc = 0;
  
  /* p1 and p2 are pointers on &items */
  Human* v1 = *((Human**)p1);
  Human* v2 = *((Human**)p2);

  rc = strcmp(v1->firstName, v2->firstName);
  if (!rc) rc = strcmp(v1->secondName, v2->secondName);

  return rc;
}

// same function for avl trees
int cmpHumanAvl(const void *p1, const void *p2)
{
  int rc = 0;
  
  /* p1 and p2 are pointers on items */
  const Human* v1 = p1;
  const Human* v2 = p2;

  rc = strcmp(v1->firstName, v2->firstName);
  if (!rc) rc = strcmp(v1->secondName, v2->secondName);

  return rc;
}

/*=======================================================================
 * Function   : serializeHuman
 * Description: Serialize a Human.
 * Synopsis   : int serializeHuman(Human* self, CvsFile* fd)
 * Input      : Human* self = what to serialize
 *              CvsFile* fd = where to serialize
 * Output     : TRUE on success
 =======================================================================*/
int
serializeHuman(Human* self, CvsFile* fd)
{
  int rc = FALSE;
  AssoCarac *assoCarac = 0;
  Category *cathegory = 0;
  int i = -1;

  if(self == 0) goto error;
  logMemory(LOG_DEBUG, "serialize Human %i: %s %s",
	  self->id, self->firstName, self->secondName);

  fd->print(fd, "Human\t \"%s\" \"%s\"", self->firstName, self->secondName);
  fd->doCut = FALSE;
  
  // serialize categories
  if (!isEmptyRing(self->categories)) {
    rgSort(self->categories, cmpCategory);
    rgRewind(self->categories);
    while ((cathegory = rgNext(self->categories))) {
      fd->print(fd, "%s\"%s\"", (++i)?", ":": ", cathegory->label);
    }
  }
  fd->print(fd, "\n");

  // serialize assoCaracs
  if (!isEmptyRing(self->assoCaracs)) {
    rgSort(self->assoCaracs, cmpAssoCarac);
    rgRewind(self->assoCaracs);
    while ((assoCarac = rgNext(self->assoCaracs))) {
      serializeAssoCarac(assoCarac, fd);
    }
  }
  fd->print(fd, "\n");

  ++env.progBar.cur;
  fd->doCut = TRUE;
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "cannot serialize empty Human");
  }
  return(rc);
}


/*=======================================================================
 * Function   : createDocument
 * Description: Create, by memory allocation a Document
 * Synopsis   : Document* createDocument(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Document*
createDocument(void)
{
  Document* rc = 0;

  if ((rc = (Document*)malloc(sizeof(Document))) == 0) goto error;
  memset(rc, 0, sizeof(Document));

  if ((rc->categories = createRing()) == 0 ||
      (rc->assoCaracs = createRing()) == 0 ||
      (rc->assoRoles = createRing()) == 0 ||
      (rc->archives = createRing()) == 0)
    goto error;

  return rc;
 error:
  logMemory(LOG_ERR, "malloc: cannot create Document");
  return destroyDocument(rc);
}

/*=======================================================================
 * Function   : destroyDocument
 * Description: Destroy a Document by freeing all the allocate memory.
 * Synopsis   : void destroyDocument(Document* self)
 * Input      : Document* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a Document.
 =======================================================================*/
Document*
destroyDocument(Document* self)
{
  Document* rc = 0;

  if(self) {
    self->label = destroyString(self->label);
 
    // delete assoCarac associations
    self->assoCaracs
      = destroyRing(self->assoCaracs,
		    (void*(*)(void*)) destroyAssoCarac);

    // do not destroy childs
    self->archives = destroyOnlyRing(self->archives);
    self->assoRoles = destroyOnlyRing(self->assoRoles);
    self->categories = destroyOnlyRing(self->categories);
    free(self);
  }

  return(rc);
}

/*=======================================================================
 * Function   : cmpDocument
 * Description: compare two Documents alphabetically
 * Synopsis   : int cmpDocument(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on Document*
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int
cmpDocument(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items */
  Document* v1 = *((Document**)p1);
  Document* v2 = *((Document**)p2);

  rc = strcmp(v1->label, v2->label);
  return rc;
}

/*=======================================================================
 * Function   : cmpDocumentAvl
 * Description: compare two Documents alphabetically
 * Synopsis   : int cmpDocumentAvl(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on Document
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
// same function for AVL trees
int
cmpDocumentAvl(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on items */
  Document* v1 = (Document*)p1;
  Document* v2 = (Document*)p2;

  rc = strcmp(v1->label, v2->label);
  return rc;
}

/*=======================================================================
 * Function   : serializeDocument
 * Description: Serialize a Document.
 * Synopsis   : int serializeDocument(Document* self, CvsFile* fd)
 * Input      : ioHandler* stdout = where to serialize
 *              Document* self = what to serialize
 * Output     : TRUE on success
 =======================================================================*/
int
serializeDocument(Document* self, CvsFile* fd)
{
  int rc = FALSE;
  AssoCarac *assoCarac = 0;
  Archive *archive = 0;
  AssoRole *assoRole = 0;
  Category *cathegory = 0;
  int i=-1;
 
  if(self == 0) goto error;
  logMemory(LOG_DEBUG, "serialize Document %s", self->label);

  fd->print(fd, "Document \"%s\"", self->label);
  fd->doCut = FALSE;

  // serialize categories
  if (!isEmptyRing(self->categories)) {
    rgSort(self->categories, cmpCategory);
    rgRewind(self->categories);
    while ((cathegory = rgNext(self->categories))) {
      fd->print(fd, "%s\"%s\"", (++i)?", ":": ", cathegory->label);
    }
  }
  fd->print(fd, "\n");
  
  // serialize assoRoles
  if (!isEmptyRing(self->assoRoles)) {
    rgSort(self->assoRoles, cmpAssoRole);
    rgRewind(self->assoRoles);
    while ((assoRole = rgNext(self->assoRoles))) {
      if (!serializeAssoRole(assoRole, fd)) goto error;
    }
  }

  // serialize assoCaracs
  if (!isEmptyRing(self->assoCaracs)) {
    rgSort(self->assoCaracs, cmpAssoCarac);
    rgRewind(self->assoCaracs);
    while ((assoCarac = rgNext(self->assoCaracs))) {
      if (!serializeAssoCarac(assoCarac, fd)) goto error;
    }
  }

  // serialize archives
  if (!isEmptyRing(self->archives)) {
    rgSort(self->archives, cmpArchive);
    rgRewind(self->archives);
    while ((archive = rgNext(self->archives))) {
      fd->print(fd, "  %s:%lli\n", archive->hash, archive->size);
    }
  }

  fd->print(fd, "\n");

  ++env.progBar.cur;
  fd->doCut = TRUE;
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "serializeDocument fails");
  }
  return(rc);
}


/*=======================================================================
 * Function   : createCategory
 * Description: Create, by memory allocation a Category
 * Synopsis   : Category* createCategory(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Category*
createCategory(void)
{
  Category* rc = 0;

  if ((rc = (Category*)malloc(sizeof(Category))) == 0) goto error;
  memset(rc, 0, sizeof(Category));

  if ((rc->fathers = createRing()) == 0 ||
      (rc->childs = createRing()) == 0 ||
      (rc->assoCaracs = createRing()) == 0 ||
      (rc->humans = createRing()) == 0);

  if (!(rc->documents = avl_alloc_tree(cmpDocumentAvl, 0)))
    goto error;

  return rc;
 error:
  logMemory(LOG_ERR, "malloc: cannot create Category");
  return destroyCategory(rc);
}

/*=======================================================================
 * Function   : destroyCategory
 * Description: Destroy a Category by freeing all the allocate memory.
 * Synopsis   : void destroyCategory(Category* self)
 * Input      : Category* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a Category.
 =======================================================================*/
Category*
destroyCategory(Category* self)
{
  Category* rc = 0;

  if(self) {
    self->label = destroyString(self->label);
    
    // delete assoCarac associations
    self->assoCaracs
      = destroyRing(self->assoCaracs,
		    (void*(*)(void*)) destroyAssoCarac);
    
    /* do not destroy objects */
    self->humans = destroyOnlyRing(self->humans);
    self->childs = destroyOnlyRing(self->childs);
    self->fathers = destroyOnlyRing(self->fathers);
    avl_free_tree(self->documents);
    free(self);
  }

  return(rc);
}

/*=======================================================================
 * Function   : cmpCategory
 * Description: compare two Category on (fatherId, Id)
 * Synopsis   : int orderCategory(const void *p1, const void *p2)
 * Input      : p1 and p2 are pointers on Category
 * Output     : p1 = p2 ? 0 : (p1 < p2 ? -1 : 1)
 =======================================================================*/
int
cmpCategory(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items */
  Category* v1 = *((Category**)p1);
  Category* v2 = *((Category**)p2);

  rc = strcmp(v1->label, v2->label);
  return rc;
}

/*=======================================================================
 * Function   : serializeCategory
 * Description: Serialize a Category.
 * Synopsis   : int serializeCategory(ioHandler* stdout, Category* self)
 * Input      : CvsFile* fd = where to serialize
 *              Category* self = what to serialize
 * Output     : TRUE on success
 =======================================================================*/
int
serializeCategory(Category* self, CvsFile* fd)
{
  int rc = FALSE;
  Category *category = 0;
  AssoCarac  *assoCarac  = 0;
  int i=-1;

  if(self == 0) goto error;
  logMemory(LOG_DEBUG, "serialize Category %s", self->label);

  fd->print(fd, "%sCategory \"%s\"", self->show?"Top ":"", self->label);
  fd->doCut = FALSE;

  // serialize father's
  if (!isEmptyRing(self->fathers)) {
    //rgSort(self->fathers, cmpCategory);
    rgRewind(self->fathers);
    while ((category = rgNext(self->fathers))) {
      fd->print(fd, "%s\"%s\"", (++i)?", ":": ", category->label);
    }
  }
  fd->print(fd, "\n");

  // serialize assoCaracs
  if (!isEmptyRing(self->assoCaracs)) {
    rgSort(self->assoCaracs, cmpAssoCarac);
    rgRewind(self->assoCaracs);
    while ((assoCarac = rgNext(self->assoCaracs))) {
      if (!serializeAssoCarac(assoCarac, fd)) goto error;
    }
  }
  fd->print(fd, "\n");

  fd->doCut = TRUE;
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "serializeCategory fails");
  }
  return(rc);
}

/*=======================================================================
 * Function   : createCatalogTree
 * Description: Create, by memory allocation a CatalogTree projection.
 * Synopsis   : CatalogTree* createCatalogTree(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
CatalogTree*
createCatalogTree(void)
{
  CatalogTree* rc = 0;
  //int i = 0;

  if((rc = (CatalogTree*)malloc(sizeof(CatalogTree))) == 0)
    goto error;
    
  memset(rc, 0, sizeof(CatalogTree));

  /* entities */
  if ((rc->roles = createRing()) == 0
      || (rc->categories = createRing()) == 0)
    goto error;

  if (!(rc->documents = 
	avl_alloc_tree(cmpDocumentAvl, (avl_freeitem_t)destroyDocument))
      || !(rc->humans = 
	   avl_alloc_tree(cmpHumanAvl, (avl_freeitem_t)destroyHuman)))
      goto error;

  if ((rc->caracs = createRing()) == 0) goto error;

  return rc;
 error:
  logMemory(LOG_ERR, "malloc: cannot create CatalogTree");
  return destroyCatalogTree(rc);
}


/*=======================================================================
 * Function   : destroyCatalogTree
 * Description: Destroy a CatalogTree by freeing all the allocate memory.
 * Synopsis   : void destroyCatalogTree(CatalogTree* self)
 * Input      : CatalogTree* self = the address of the CatalogTree to
 *              destroy.
 * Output     : Nil address of a CatalogTree.
 =======================================================================*/
CatalogTree*
destroyCatalogTree(CatalogTree* self)
{
  CatalogTree* rc = 0;
  //int i = 0;

  if(self) {

    /* entities */
    /* do not destroy archives (owned by the collection) */

    self->roles
      = destroyRing(self->roles,
		    (void*(*)(void*)) destroyRole);

    avl_free_tree(self->humans);
    avl_free_tree(self->documents);

    self->categories
      = destroyRing(self->categories,
		    (void*(*)(void*)) destroyCategory);

    self->caracs
      = destroyRing(self->caracs,
		    (void*(*)(void*)) destroyCarac);

    free(self);
  }
  return(rc);
}


/*=======================================================================
 * Function   : serializeCatalogTree
 * Description: Serialize the catalog metadata
 * Synopsis   : int serializeCatalogTree(Collection* coll, CvsFile* fd)
 * Input      : Collection* coll = what to serialize
 *              CvsFile* fd = serializer object:
 *              - fd = {0, 0, 0, FALSE, 0, cvsCutOpen, cvsCutPrint};
 *              - fd = {0, 0, 0, FALSE, 0, cvsCatOpen, cvsCatPrint};
 * Output     : TRUE on success
 =======================================================================*/
int
serializeCatalogTree(Collection* coll, CvsFile* fd)
{
  int rc = FALSE;
  Human* human = 0;
  Archive* archive = 0;
  Document* document = 0;
  Category* category = 0;
  CatalogTree* self = 0;
  AVLNode *node = 0;
  int uid = getuid();

  checkCollection(coll);
  if (!(self = coll->catalogTree)) goto error;
  logMemory(LOG_DEBUG, "serialize %s document tree", coll->label);

  if (!fd) goto error;
  fd->nb = 0;
  fd->fd = 0;
  fd->doCut = FALSE;
  fd->offset = 0;

  // we neeed to use the cvs collection directory
  if (!coll->memoryState & EXPANDED) {
    logMemory(LOG_ERR, "collection must be expanded first");
    goto error;
  }
  
  if (!becomeUser(coll->user, TRUE)) goto error;

  // output file
  if (env.dryRun) fd->fd = stdout;
  fd->path = coll->catalogDB;  
  if (!fd->open(fd)) goto error;

  fd->print(fd, "# MediaTeX collection catalog: %s\n", coll->label);
  //fd->print(fd, "# Version: $" "Id" "$\n");

  fd->print(fd, "\n# Categories:\n\n");
  if (!isEmptyRing(self->categories)) {
    //rgSort(self->categories, cmpCategory);
    rgRewind(self->categories);
    while ((category = rgNext(self->categories))) {
      if (!serializeCategory(category, fd)) goto error;
    }
  }

  fd->print(fd, "# Humans:\n\n");
  if (avl_count(self->humans)) {
    for (node = self->humans->head; node; node = node->next) {
      human = (Human*)node->item;
      if (!serializeHuman(human, fd)) goto error;
    }
  }

  fd->print(fd, "# Archives:\n\n");
  if (avl_count(coll->archives)) {
    for (node = coll->archives->head; node; node = node->next) {
      archive = (Archive*)node->item;
      if (!isEmptyRing(archive->assoCaracs)) {
	if (!serializeCatalogArchive(archive, fd)) goto error;
      }
    }
  }

  fd->print(fd, "# Documents:\n\n");
  if (avl_count(self->documents)) {
    for (node = self->documents->head; node; node = node->next) {
      document = (Document*)node->item;
      if (!serializeDocument(document, fd)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!cvsClose(fd)) rc = FALSE;
  if (!logoutUser(uid)) rc = FALSE;
  if (!rc) {
    logMemory(LOG_ERR, "serializeCatalogTree fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getCarac
 * Description: Search or may create a Carac
 * Synopsis   : Carac* getCarac(Collection* coll, 
 *                                           CaracType type, char* label)
 * Input      : Collection* coll: where to find
 *              CaracType type: type of carac
 *              char* label: id of the carac
 * Output     : The address of the Carac.
 =======================================================================*/
Carac* 
getCarac(Collection* coll, char* label)
{
  Carac* rc = 0;
  RGIT* curr = 0;

  checkCollection(coll);
 
  // look for carac
  while ((rc = rgNext_r(coll->catalogTree->caracs, &curr)))
    if (!strcmp(rc->label, label)) break;

 error:
  return rc;
}

/*=======================================================================
 * Function   : addCarac
 * Description: Search or may create a Carac
 * Synopsis   : Carac* addCarac(Collection* coll, 
 *                                           CaracType type, char* label)
 * Input      : Collection* coll: where to find
 *              CaracType type: type of carac
 *              char* label: id of the carac
 * Output     : The address of the Carac.
 =======================================================================*/
Carac* 
addCarac(Collection* coll, char* label)
{
  Carac* rc = 0;
  Carac* carac = 0;

  checkCollection(coll);
  logMemory(LOG_DEBUG, "create the %s carac", label);

  // already there
  if ((carac = getCarac(coll, label))) goto end;

  // add new one if not already there
  if (!(carac = createCarac())) goto error;
  if (!(carac->label = createString(label))) goto error;
  if (!rgInsert(coll->catalogTree->caracs, carac)) goto error; 

 end:
  rc = carac;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addCarac fails");
    carac = destroyCarac(carac);
  }
  return rc;
}

/*=======================================================================
 * Function   : delCarac
 * Description: Del a carac
 * Synopsis   : int delCarac(Collection* coll, Carac* self)
 * Input      : Collection* coll : where to del
 *              Carac* self : the carac to del
 * Output     : TRUE on success
 =======================================================================*/
int
delCarac(Collection* coll, Carac* self)
{
  int rc = FALSE;
  RGIT* curr = 0;
  
  checkCollection(coll);
  logMemory(LOG_DEBUG, "delCarac %s", self->label);

  // delete carac from catalogTree rings
  if ((curr = rgHaveItem(coll->catalogTree->caracs, self))) {
    rgRemove_r(coll->catalogTree->caracs, &curr);
  }

  // free the carac
  destroyCarac(self);

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delCarac fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getAssoCarac
 * Description: Search or may create an AssoCarac
 * Synopsis   : AssoCarac* getAssoCarac(Collection* coll, 
 *                            Carac* carac, void* entity, char* value)
 * Input      : Collection* coll: where to find
 *              Carac* carac: related carac
 *              Entity* entity: related entity
 *              char* value: ie
 * Output     : The address of the AssoCarac.
 =======================================================================*/
AssoCarac* 
getAssoCarac(Collection* coll, Carac* carac, CType type, 
	     void* entity, char* value)
{
  AssoCarac* rc = 0;
  RG* ring = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  if (!carac || !entity || !value) goto error;
  logMemory(LOG_DEBUG, "getAssoCarac %s %s=%s", 
	  strCType(type), carac->label, value);

  switch (type) {
  case CATE:
    ring = ((Category*)entity)->assoCaracs;
    break;
  case DOC:
    ring = ((Document*)entity)->assoCaracs;
    break;
  case HUM:
    ring = ((Human*)entity)->assoCaracs;
    break;
  case ARCH:
    ring = ((Archive*)entity)->assoCaracs;
    break;
  default:
    logMemory(LOG_ERR, "unknown type %i", type);
    goto error;
  }

  // look for assoCarac
  while ((rc = rgNext_r(ring, &curr))) {
    if (carac == rc->carac && !strcmp(rc->value, value)) break;
  }
  
 error:
  return rc;
}

/*=======================================================================
 * Function   : addAssoCarac
 * Description: Search or may create an AssoCarac
 * Synopsis   : AssoCarac* addAssoCarac(Collection* coll, 
 *                            Carac* carac, void* entity, char* value)
 * Input      : Collection* coll: where to find
 *              Carac* carac: related carac
 *              Entity* entity: related entity
 *              char* value: ie
 * Output     : The address of the AssoCarac.
 =======================================================================*/
AssoCarac* 
addAssoCarac(Collection* coll, Carac* carac, CType type, 
	     void* entity, char* value)
{
  AssoCarac* rc = 0;
  AssoCarac* asso = 0;
  RG* ring = 0;

  checkCollection(coll);
  if (!carac || !entity || !value) goto error;
  logMemory(LOG_DEBUG, "addAssoCarac %s %s=%s", 
	  strCType(type), carac->label, value);

  // already there
  if ((asso = getAssoCarac(coll, carac, type, entity, value))) goto end;

  // add new one if not already there
  if ((asso = createAssoCarac()) == 0) goto error;
  if (!(asso->value = createString(value))) goto error; 
  asso->carac = carac;

  // add it to the entity tree
  switch (type) {
  case CATE:
    ring = ((Category*)entity)->assoCaracs;
    break;
  case DOC:
    ring = ((Document*)entity)->assoCaracs;
    break;
  case HUM:
    ring = ((Human*)entity)->assoCaracs;
    break;
  case ARCH:
    ring = ((Archive*)entity)->assoCaracs;
    break;
  default:
    logMemory(LOG_INFO, "unknown carac type %i", type);
    goto error;
  }
  if (!rgInsert(ring, asso)) goto error;

 end:
  rc = asso;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addAssoCarac fails");
    asso = destroyAssoCarac(asso);
  }
  return rc;
}

/*=======================================================================
 * Function   : getRole
 * Description: Search or may create a Role
 * Synopsis   : Role* getRole(Collection* coll, 
 *                                           RoleType type, char* label)
 * Input      : Collection* coll: where to find
 *              RoleType type: type of role
 *              char* label: id of the role
 * Output     : The address of the Role.
 =======================================================================*/
Role* 
getRole(Collection* coll, char* label)
{
  Role* rc = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  logMemory(LOG_DEBUG, "getRole %s", label);

  // look for role
  while ((rc = rgNext_r(coll->catalogTree->roles, &curr)))
    if (!strcmp(rc->label, label)) break;

 error:
  return rc;
}

/*=======================================================================
 * Function   : addRole
 * Description: Search or may create a Role
 * Synopsis   : Role* addRole(Collection* coll, 
 *                                           RoleType type, char* label)
 * Input      : Collection* coll: where to find
 *              RoleType type: type of role
 *              char* label: id of the role
 * Output     : The address of the Role.
 =======================================================================*/
Role* 
addRole(Collection* coll, char* label)
{
  Role* rc = 0;
  Role* role = 0;

  checkCollection(coll);
  logMemory(LOG_DEBUG, "create the %s role", label);

  // already there
  if ((role = getRole(coll, label))) goto end;

  // add new one if not already there
  if (!(role = createRole())) goto error;
  if (!(role->label = createString(label))) goto error;

  if (!rgInsert(coll->catalogTree->roles, role)) goto error;
   role->id = coll->catalogTree->maxId[ROLE]++;
 end:
  rc = role;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addRole fails");
    role = destroyRole(role);
  }
  return rc;
}

/*=======================================================================
 * Function   : delRole
 * Description: Del a role
 * Synopsis   : int delRole(Collection* coll, Role* self)
 * Input      : Collection* coll : where to del
 *              Role* self : the role to del
 * Output     : TRUE on success
 =======================================================================*/
int
delRole(Collection* coll, Role* self)
{
  int rc = FALSE;
  AVLNode *node = 0;
  RGIT* curr = 0;
  
  checkCollection(coll);
  logMemory(LOG_DEBUG, "delRole %s", self->label);

  // delete related assossiations
  while ((node = self->assos->head))
    if (!delAssoRole(coll, node->item)) goto error;

  // delete role from catalogTree rings
  if ((curr = rgHaveItem(coll->catalogTree->roles, self))) {
    rgRemove_r(coll->catalogTree->roles, &curr);
  }

  // free the role
  destroyRole(self);

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delRole fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getAssoRole
 * Description: Search or may create an AssoRole
 * Synopsis   : AssoRole* getAssoRole(Collection* coll, Role* role, 
 *                 Human* human, Document* document)
 * Input      : Collection* coll: where to find
 *              Role* role: related role
 *              Human* human: related human
 *              Document* document: related document
 * Output     : The address of the AssoRole.
 =======================================================================*/
AssoRole* 
getAssoRole(Collection* coll, 
	    Role* role, Human* human, Document* document)
{
  AssoRole* rc = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  if (!role || !human || !document) goto error;
  logMemory(LOG_DEBUG, "getAssoRole %s, %s-%s, %s", role->label, 
	  human->firstName, human->secondName, document->label);

  // look for assoRole
  while ((rc = rgNext_r(document->assoRoles, &curr))) {
    if (rc->human == human && rc->role == role) break;
  }
  
 error:
  return rc;
}

/*=======================================================================
 * Function   : addAssoRole
 * Description: Add an AssoRole if not already there
 * Synopsis   : AssoRole* addAssoRole(Collection* coll, Role* role, 
 *                 Human* human, Document* document)
 * Input      : Collection* coll: where to find
 *              Role* role: related role
 *              Human* human: related human
 *              Document* document: related document
 * Output     : The address of the AssoRole.
 =======================================================================*/
AssoRole* 
addAssoRole(Collection* coll, 
	    Role* role, Human* human, Document* document)
{
  AssoRole* rc = 0;
  AssoRole* asso = 0;
 
  checkCollection(coll);
  if (!role || !human || !document) goto error;
  logMemory(LOG_DEBUG, "addAssoRole %s, %s-%s, %s", role->label, 
	  human->firstName, human->secondName, document->label);

  // already there
  if ((asso = getAssoRole(coll, role, human, document))) goto end;

  // add new one if not already there
  if ((asso = createAssoRole()) == 0) goto error;
  asso->role = role;
  asso->human = human;
  asso->document = document;
  
  // add it to the role ring
  if (!avl_insert_2(role->assos, asso)) {
    logMemory(LOG_ERR, "assoRole already added ?");
    goto error;
  }

  // add it to the human ring
  if (!rgInsert(human->assoRoles, asso)) goto error;
  
  // add it to the document tree
  if (!rgInsert(document->assoRoles, asso)) goto error;

 end:
  rc = asso;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "cannot add an assoRole");
    if (asso) delAssoRole(coll, asso);
  }
  return rc;
}

/*=======================================================================
 * Function   : delAssoRole
 * Description: Del a assoRole
 * Synopsis   : int delAssoRole(Collection* coll, AssoRole* self)
 * Input      : Collection* coll : where to del
 *              AssoRole* self : the assoRole to del
 * Output     : TRUE on success
 =======================================================================*/
int
delAssoRole(Collection* coll, AssoRole* self)
{
  int rc = FALSE;
  RGIT* curr = 0;
  
  checkCollection(coll);
  if (!self) goto error;
  logMemory(LOG_DEBUG, "delAssoRole %s, %s-%s, %s", self->role->label, 
	  self->human->firstName, self->human->secondName, 
	  self->document->label);

  // delete from human ring
  if ((curr = rgHaveItem(self->human->assoRoles, self))) {
    rgRemove_r(self->human->assoRoles, &curr);
  }

  // delete from document ring
  if ((curr = rgHaveItem(self->document->assoRoles, self))) {
    rgRemove_r(self->document->assoRoles, &curr);
  }

  // free the assoRole  
  avl_delete(self->role->assos, self);

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delAssoRole fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : addHumanToCategory
 * Description: Add a human to a category
 * Synopsis   : int addHumanToCategory(Collection* coll, 
 *                                    Human* human, Category* category)
 * Input      : Collection* coll: where to find
 *              Human* human
 *              Category* category
 * Output     : TRUE on success
 =======================================================================*/
int addHumanToCategory(Collection* coll, Human* human, Category* category)
{
  int rc = FALSE;

  checkCollection(coll);
  if (!human || !category) goto error;
  logMemory(LOG_DEBUG, "addHumanToCategory %s-%s, %s",
	  human->firstName, human->secondName, category->label);

  // add human to category ring
  if (!rgHaveItem(category->humans, human) &&
      !rgInsert(category->humans, human)) goto error;
  
  // add human to category ring
  if (!rgHaveItem(human->categories, category) &&
      !rgInsert(human->categories, category)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addHumanToCategory fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : delHumanToCategory
 * Description: Del a human to a category
 * Synopsis   : int delHumanToCategory(Collection* coll, 
 *                                    Human* human, Category* category)
 * Input      : Collection* coll: where to find
 *              Human* human
 *              Category* category
 * Output     : TRUE on success
 =======================================================================*/
int delHumanToCategory(Collection* coll, Human* human, Category* category)
{
  int rc = FALSE;
  RGIT* curr = 0;

  checkCollection(coll);
  if (!human || !category) goto error;
  logMemory(LOG_DEBUG, "delHumanToCategory %s-%s, %s",
	  human->firstName, human->secondName, category->label);

  // del human to category ring
  if ((curr = rgHaveItem(category->humans, human))) {
    rgRemove_r(category->humans, &curr);
  }
  
  // del human to category ring
  if ((curr = rgHaveItem(human->categories, category))) {
    rgRemove_r(human->categories, &curr);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delHumanToCategory fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getHuman
 * Description: Search or may create a Human
 * Synopsis   : Human* getHuman(Collection* coll, 
 *                              char* firstName, char* secondName)
 * Input      : Collection* coll: where to find
 *             char* firstName: id
 *             char* secondName: id
 * Output     : The address of the Human.
 =======================================================================*/
Human* 
getHuman(Collection* coll, char* firstName, char* secondName)
{
  Human* rc = 0;
  Human human;
  AVLNode* node = 0;

  checkCollection(coll);
  checkLabel(firstName);
  //checkLabel(secondName); may be null
  logMemory(LOG_DEBUG, "getHuman %s-%s", firstName, secondName);

  // look for human
  human.firstName = firstName;
  human.secondName = secondName;
  if ((node = avl_search(coll->catalogTree->humans, &human))) {
    rc = (Human*)node->item;
  }
 error:
  return rc;
}

/*=======================================================================
 * Function   : addHuman
 * Description: Search or may create a Human
 * Synopsis   : Human* addHuman(Collection* coll, 
 *                              char* firstName, char* secondName)
 * Input      : Collection* coll: where to find
 *             char* firstName: id
 *             char* secondName: id
 * Output     : The address of the Human.
 =======================================================================*/
Human* 
addHuman(Collection* coll, char* firstName, char* secondName)
{
  Human* rc = 0;
  Human* human = 0;

  checkCollection(coll);
  checkLabel(firstName);
  //checkLabel(secondName); may be null
  logMemory(LOG_DEBUG, "addHuman %s-%s", firstName, secondName);

  // already there
  if ((human = getHuman(coll, firstName, secondName))) goto end;

  // add new one if not already there
  if ((human = createHuman()) == 0) goto error;
  if (!(human->firstName = createString(firstName))) goto error;
  if (!(human->secondName = createString(secondName))) goto error;
  human->id = coll->catalogTree->maxId[HUM]++;
  if (!avl_insert_2(coll->catalogTree->humans, human)) goto error;

 end:
  rc = human;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addHuman fails");
    human = destroyHuman(human);
  }	    
  return rc;
}

/*=======================================================================
 * Function   : delHuman
 * Description: Del a carac
 * Synopsis   : int delHuman(Collection* coll, Human* self)
 * Input      : Collection* coll : where to del
 *              Human* self : the carac to del
 * Output     : TRUE on success
 =======================================================================*/
int
delHuman(Collection* coll, Human* self)
{
  int rc = FALSE;
  AssoRole* ar = 0;
  Category* cat = 0;
  RGIT* curr = 0;
  RGIT* curr2 = 0;
  
  checkCollection(coll);
  if (!self) goto error;
  logMemory(LOG_DEBUG, "delHuman %s-%s", self->firstName, self->secondName);

  // delete assoRole associations
  while ((ar = rgNext_r(self->assoRoles, &curr))) {
    if (!delAssoRole(coll, ar)) goto error;
  }

  // delete human from categories rings
  curr = 0;
  while ((cat = rgNext_r(self->categories, &curr))) {
    if ((curr2 = rgHaveItem(cat->humans, self))) {
      rgRemove_r(cat->humans, &curr2);
    }
  }

  // delete human from document tree and free it
  avl_delete(coll->catalogTree->humans, self);

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delHuman fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : addArchiveToDocument
 * Description: Add a archive to a document
 * Synopsis   : int addArchiveToDocument(Collection* coll, 
 *                                    Archive* archive, Document* document)
 * Input      : Collection* coll: where to find
 *              Archive* archive
 *              Document* document
 * Output     : TRUE on success
 =======================================================================*/
int addArchiveToDocument(Collection* coll, 
			 Archive* archive, Document* document)
{
  int rc = FALSE;

  checkCollection(coll);
  checkArchive(archive);
  if (!archive) goto error;
  logMemory(LOG_DEBUG, "addArchiveToDocument %s:%lli, %s",
	  archive->hash, (long long int)archive->size, document->label);

  // add archive to document ring
  if (!rgHaveItem(document->archives, archive) &&
      !rgInsert(document->archives, archive)) goto error;
  
  // add document to archive ring
  if (!rgHaveItem(archive->documents, document) &&
      !rgInsert(archive->documents, document)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addArchiveToDocument fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : delArchiveFromDocument
 * Description: Del a archive to a document
 * Synopsis   : int delArchiveFromDocument(Collection* coll, 
 *                                    Archive* archive, Document* document)
 * Input      : Collection* coll: where to find
 *              Archive* archive
 *              Document* document
 * Output     : TRUE on success
 =======================================================================*/
int delArchiveFromDocument(Collection* coll, 
			   Archive* archive, Document* document)
{
  int rc = FALSE;
  RGIT* curr = 0;

  checkCollection(coll);
  checkArchive(archive);
  if (!archive) goto error;
  logMemory(LOG_DEBUG, "delArchiveFromDocument %s:%lli, %s",
	  archive->hash, (long long int)archive->size, document->label);

  // del archive to document ring
  if ((curr = rgHaveItem(document->archives, archive))) {
    rgRemove_r(document->archives, &curr);
  }

  // del document to archive ring
  if ((curr = rgHaveItem(archive->documents, document))) {
    rgRemove_r(archive->documents, &curr);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delArchiveFromDocument fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : addDocumentToCategory
 * Description: Add a document to a category
 * Synopsis   : int addDocumentToCategory(Collection* coll, 
 *                             Document* document, Category* category)
 * Input      : Collection* coll: where to find
 *              Document* document
 *              Category* category
 * Output     : TRUE on success
 =======================================================================*/
int addDocumentToCategory(Collection* coll, Document* document, 
			  Category* category)
{
  int rc = FALSE;

  checkCollection(coll);
  if (!document || !category) goto error;
  logMemory(LOG_DEBUG, "addDocumentToCategory %s, %s",
	  document->label, category->label);

  // add document to category tree
  if (!avl_insert_2(category->documents, document)) {
    if (errno != EEXIST) {
      goto error;
    }
  }
  
  // add category to document ring
  if (!rgHaveItem(document->categories, category) &&
      !rgInsert(document->categories, category)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addDocumentToCategory fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : delDocumentToCategory
 * Description: Del a document to a category
 * Synopsis   : int delDocumentToCategory(Collection* coll, 
 *                          Document* document, Category* category)
 * Input      : Collection* coll: where to find
 *              Document* document
 *              Category* category
 * Output     : TRUE on success
 =======================================================================*/
int delDocumentToCategory(Collection* coll, Document* document, 
			  Category* category)
{
  int rc = FALSE;
  RGIT* curr = 0;

  checkCollection(coll);
  if (!document || !category) goto error;
  logMemory(LOG_DEBUG, "delDocumentToCategory %s, %s",
	  document->label, category->label);

  // del document from category tree (do not free it)
  avl_delete(category->documents, document);
  
  // del category from document ring
  if ((curr = rgHaveItem(document->categories, category))) {
    rgRemove_r(document->categories, &curr);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delDocumentToCategory fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getDocument
 * Description: Search or may create a Document
 * Synopsis   : Document* getDocument(Collection* coll, int id);
 * Input      : Collection* coll: where to find
 *             int id: ie
 * Output     : The address of the Document.
 * Note       : several document may have the same label
 =======================================================================*/
Document* 
getDocument(Collection* coll, char* label)
{
  Document* rc = 0;
  Document document;
  AVLNode* node = 0;

  checkCollection(coll);
  logMemory(LOG_DEBUG, "getDocument %s", label);

  // look for document
  document.label = label;
  if ((node = avl_search(coll->catalogTree->documents, &document))) {
    rc = (Document*)node->item;
  }
 error:
  return rc;
}

/*=======================================================================
 * Function   : addDocument
 * Description: Search or may create a Document
 * Synopsis   : Document* addDocument(Collection* coll)
 * Input      : Collection* coll: where to find
 *              char* label: label
 * Output     : The address of the Document.
 *
 * Note       : here we decided not to have a key on label
 =======================================================================*/
Document* 
addDocument(Collection* coll, char* label)
{
  Document* rc = 0;
  Document* document = 0;

  checkCollection(coll);
  checkLabel(label);
  logMemory(LOG_DEBUG, "addDocument %s", label);

  // add new one if not already there
  if ((document = createDocument()) == 0) goto error;
  if (!(document->label = createString(label))) goto error; 
  if (avl_insert_2(coll->catalogTree->documents, document)) {
    document->id = coll->catalogTree->maxId[DOC]++;
  }
  else {
    if (errno != EEXIST) {
      logMemory(LOG_ERR, "fails to add document");
      goto error;
    }

    // already there
    document = destroyDocument(document);
    if (!(document = getDocument(coll, label))) {
      logMemory(LOG_ERR, "document not already there as expected");
      goto error;
    }
  }

  rc = document;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addDocument fails");
    document = destroyDocument(document);
  }
  return rc;
}

/*=======================================================================
 * Function   : delDocument
 * Description: Del a document
 * Synopsis   : int delDocument(Collection* coll, Document* self)
 * Input      : Collection* coll : where to del
 *              Document* self : the carac to del
 * Output     : TRUE on success
 =======================================================================*/
int
delDocument(Collection* coll, Document* self)
{
  int rc = FALSE;
  AssoRole* aR = 0;
  Category* cat = 0;
  Archive* arch = 0;
  RGIT* curr = 0;
  RGIT* curr2 = 0;
  
  checkCollection(coll);
  if (!self) goto error;
  logMemory(LOG_DEBUG, "delDocument %i (%s)", self->id, self->label);

  // delete assoRole associations
  curr = 0;
  while ((aR = rgNext_r(self->assoRoles, &curr))) {
    if (!delAssoRole(coll, aR)) goto error;
  }

  // delete document from categories tree (do not free it)
  curr = curr2 = 0;
  while ((cat = rgNext_r(self->categories, &curr))) {
    avl_delete(cat->documents, self);
  }

  // delete document from archive rings
  curr = curr2 = 0;
  while ((arch = rgNext_r(self->archives, &curr))) {
    if ((curr2 = rgHaveItem(arch->documents, self))) {
      rgRemove_r(arch->documents, &curr2);
    }
  }

  // delete document from document tree btree and free it
  avl_delete(coll->catalogTree->documents, self);

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delDocument fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : delArchiveCatalog
 * Description: Del an archive from catalog
 * Synopsis   : int delArchiveCatalog(Collection* coll, Archive* self)
 * Input      : Collection* coll : where to del
 *              Archive* self : the archive to del
 * Output     : TRUE on success
 =======================================================================*/
int
delArchiveCatalog(Collection* coll, Archive* self)
{
  int rc = FALSE;
  AssoCarac* aC = 0;
  Document* doc = 0;
  //RGIT* curr = 0;
  
  checkCollection(coll);
  checkArchive(self);
  logMemory(LOG_DEBUG, "delArchiveCatalog %s:%lli",
	  self->hash, (long long int)self->size);

  // delete assoCarac associations
  while ((aC = rgHead(self->assoCaracs))) {
    rgRemove(self->assoCaracs);
    destroyAssoCarac(aC);
  }

  // delete from document rings
  while ((doc = rgHead(self->documents))) {
    if (!delArchiveFromDocument(coll, self, doc)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delArchiveCatalog fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : addCategoryLink
 * Description: Search or may create a Category link
 * Synopsis   : Category* addCategory(Collection* coll,
 *                  Collection* coll, Category* father, Category* child)
 * Input      : Collection* coll: where to find
 *              Category* father
 *              Category* child
 * Output     : TRUE on success
 =======================================================================*/
int
addCategoryLink(Collection* coll, Category* father, Category* child)
{
  int rc = FALSE;

  checkCollection(coll);
  if (!father || !child) goto error;
  logMemory(LOG_DEBUG, "addCategoryLink %s -> %s", 
	  child->label, father->label);

  // add link to father
  if (!rgHaveItem(father->childs, child) &&
      !rgInsert(father->childs, child)) goto error;

  // add link to child
  if (!rgHaveItem(child->fathers, father) &&
      !rgInsert(child->fathers, father)) goto error;
  
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "addCategoryLink fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : delCategoryoLink
 * Description: Search or may create a Category link
 * Synopsis   : Category* delCategory(Collection* coll,
 *                  Collection* coll, Category* father, Category* child)
 * Input      : Collection* coll: where to find
 *              Category* father
 *              Category* child
 * Output     : TRUE on success
 =======================================================================*/
int
delCategoryLink(Collection* coll, Category* father, Category* child)
{
  int rc = FALSE;
  RGIT* curr = 0;

  checkCollection(coll);
  if (!father || !child) goto error;
  logMemory(LOG_DEBUG, "delCategoryLink %s -> %s", 
	  child->label, father->label);
  
  // delete father link
  if ((curr = rgHaveItem(father->childs, child))) {
    rgRemove_r(father->childs, &curr);
  }

  // delete child link
  if ((curr = rgHaveItem(child->fathers, father))) {
    rgRemove_r(child->fathers, &curr);
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delCategoryLink fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getCategory
 * Description: Search or may create a Category
 * Synopsis   : Category* getCategory(Collection* coll, char* label)
 * Input      : Collection* coll: where to find
 *              char* label: category name
 * Output     : The address of the Category.
 =======================================================================*/
Category* 
getCategory(Collection* coll, char* label)
{
  Category* rc = 0;
  RGIT* curr = 0;
  
  checkCollection(coll);
  checkLabel(label);
  logMemory(LOG_DEBUG, "getCategory %s", label);
  
  // look for category
  while ((rc = rgNext_r(coll->catalogTree->categories, &curr)) 
	!= 0)
    if (!strcmp(rc->label, label)) break;
  
 error:
  return rc;
}

/*=======================================================================
 * Function   : addCategory
 * Description: Search or may create a Category
 * Synopsis   : Category* addCategory(Collection* coll, char* label)
 * Input      : Collection* coll: where to find
 *             char* label: id
 * Output     : The address of the Category.
 =======================================================================*/
Category* 
addCategory(Collection* coll, char* label, int show)
{
  Category* rc = 0;
  Category* cat = 0;

  checkCollection(coll);
  checkLabel(label);
  logMemory(LOG_DEBUG, "addCategory %s", label);

  // already there
  if ((cat = getCategory(coll, label))) goto end;

  // add new one if not already there
  if (!(cat = createCategory())) goto error;
  if (!(cat->label = createString(label))) goto error;
  if (!rgInsert(coll->catalogTree->categories, cat)) goto error;
  cat->id = coll->catalogTree->maxId[CATE]++;

 end:
  if (show) cat->show = TRUE;
  rc = cat;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "cannot add %s category", label);
    cat = destroyCategory(cat);
  }
  return rc;
}

/*=======================================================================
 * Function   : delCategory
 * Description: Del a category
 * Synopsis   : int delCategory(Collection* coll, Category* self)
 * Input      : Collection* coll : where to del
 *              Category* self : the carac to del
 * Output     : TRUE on success
 =======================================================================*/
int
delCategory(Collection* coll, Category* self)
{
  int rc = FALSE;
  Category* cat = 0;
  Human* hum = 0;
  Document* doc = 0;
  RGIT* curr = 0;
  RGIT* curr2 = 0;
  AVLNode* node = 0;
  
  checkCollection(coll);
  if (!self) goto error;
  logMemory(LOG_DEBUG, "delCategory %s", self->label);

  // delete category from fathers ring
  while ((cat = rgHead(self->fathers)))
    if (!delCategoryLink(coll, cat, self)) goto error;

  // delete category from childs ring  curr = 0;
  while ((cat = rgHead(self->childs)))
    if (!delCategoryLink(coll, self, cat)) goto error;

  // delete category from humans rings
  curr = 0;
  curr2 = 0;
  while ((hum = rgNext_r(self->humans, &curr))) {
    if ((curr2 = rgHaveItem(hum->categories, self))) {
      rgRemove_r(hum->categories, &curr2);
    }
  }

  // delete category from documents rings
  curr = 0;
  for (node = self->documents->head; node; node = node->next) {
    doc = node->item;
    if ((curr = rgHaveItem(doc->categories, self))) {
      rgRemove_r(doc->categories, &curr2);
    }
  }

  // delete category from document tree rings
  // O(n2) when call by diseaseCatalogTree !
  if ((curr = rgHaveItem(coll->catalogTree->categories, self))) {
    rgRemove_r(coll->catalogTree->categories, &curr);
  }

  // free the category
  destroyCategory(self);
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "delCategory fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : diseaseCatalogTree
 * Description: Disease a CatalogTree by freeing all the allocate memory.
 * Synopsis   : void diseaseCatalogTree(CatalogTree* self)
 * Input      : CatalogTree* self = the address of the CatalogTree to
 *              disease.
 * Output     : TRUE on success
 =======================================================================*/
int
diseaseCatalogTree(Collection* coll)
{
  int rc = FALSE;
  CatalogTree* self = 0;
  Category* category = 0;
  Carac* carac = 0;
  Role* role = 0;
  AVLNode *node = 0;
  int i = 0;

  if(coll == 0) goto error;
  if((self = coll->catalogTree) == 0) goto error;
  logMemory(LOG_DEBUG, "diseaseCatalogTree %s", coll);
 
  // disease roles
  while ((role = rgHead(self->roles))) 
    if (!delRole(coll, role)) goto error;

  // disease humans
  while ((node = self->humans->head))
    if (!delHuman(coll, node->item)) goto error;

  // disease documents
  while ((node = self->documents->head))
    if (!delDocument(coll, node->item)) goto error;

  // diseases categories
  while ((category = rgHead(self->categories)))
    if (!delCategory(coll, category)) goto error;

  // disease archives
  if (avl_count(coll->archives)) {
    for (node = coll->archives->head; node; node = node->next) {
      if (!delArchiveCatalog(coll, node->item)) goto error;
    }
  }

  // disease caracs
  while ((carac = rgHead(self->caracs)))
    if (!delCarac(coll, carac)) goto error;

  // try to disease archives
  if (!diseaseArchives(coll)) goto error;

  // reset html ids
  for (i=0; i<CTYPE_MAX; ++i) {
    self->maxId[i] = 0;
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "diseaseCatalogTree fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
