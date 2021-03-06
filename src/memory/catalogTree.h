/*=======================================================================
 * Project: MediaTeX
 * Module : archive tree
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

#ifndef MDTX_MEMORY_CATALOG_H
#define MDTX_MEMORY_CATALOG_H 1

#include "mediatex-types.h"

typedef enum {CATE=0, DOC, HUM, ARCH, ROLE, CTYPE_MAX} CType;

struct Carac
{
  char*     label;
};

// uni-direction association
struct AssoCarac
{
  Carac* carac;
  char*  value;
};

struct Role
{
  int       id;    // role_ID.html pages
  char*     label;
  AVLTree*  assos; // AssoRole*

  /* * MAYBE : father + child like category */
};

/* (Human x Role x Document) */
struct AssoRole
{
  Human*    human;
  Document* document;
  Role*     role;
};

struct Category
{
  int      id;         // ID fo html pages
  char*    label;
  int      show;       // if we display it in the html index or not

  RG*      fathers;    // Category*
  RG*      childs;     // Category*
  RG*      assoCaracs; // AssoCarac*
  RG*      humans;     // Human*
  AVLTree* documents;  // Document*
};

struct Human
{
  int    id;         // ID fo html pages
  char*  firstName;
  char*  secondName;

  RG*    categories; // Category*
  RG*    assoCaracs; // AssoCarac*
  RG*    assoRoles;  // AssoRole*
};

struct Document
{
  int    id;         // ID fo html pages
  char*  label;

  RG*    categories; // Category*
  RG*    assoCaracs; // AssoCarac*
  RG*    assoRoles;  // AssoHRD*
  RG*    archives;   // Archives*
};

struct CatalogTree 
{
  // entities
  RG*      caracs;     // Carac*
  RG*      roles;      // Role*
  AVLTree* humans;     // Human*
  AVLTree* documents;  // Document*
  RG*      categories; // Category*

  int maxId[CTYPE_MAX];
};

/* API */

char* strCType(CType self);
Carac* createCarac(void);
Carac* destroyCarac(Carac* self);
int cmpCarac(const void *p1, const void *p2);

Role* createRole(void);
Role* destroyRole(Role* self);
int cmpRole(const void *p1, const void *p2);

AssoCarac* createAssoCarac();
AssoCarac* destroyAssoCarac(AssoCarac* self);
int cmpAssoCarac(const void *p1, const void *p2);

AssoRole* createAssoRole(void);
AssoRole* destroyAssoRole(AssoRole* self);
int cmpAssoRole(const void *p1, const void *p2);

int serializeCatalogArchive(Archive* self, CvsFile* file);

Human* createHuman(void);
Human* destroyHuman(Human* self);
int cmpHuman(const void *p1, const void *p2);
int serializeHuman(Human* self, CvsFile* fd);

Document* createDocument(void);
Document* destroyDocument(Document* self);
int cmpDocument(const void *p1, const void *p2);
int serializeDocument(Document* self, CvsFile* fd);

Category* createCategory(void);
Category* destroyCategory(Category* self);
int cmpCategory(const void *p1, const void *p2);
int serializeCategory(Category* self, CvsFile* fd);

CatalogTree* createCatalogTree(void);
CatalogTree* destroyCatalogTree(CatalogTree* self);
int serializeCatalogTree(Collection* coll, CvsFile* fd);

/* API */

Carac* getCarac(Collection* coll, char* label);
Carac* addCarac(Collection* coll, char* label);
int delCarac(Collection* coll, Carac* self);

Role* getRole(Collection* coll, char* label);
Role* addRole(Collection* coll, char* label);
int delRole(Collection* coll, Role* self);

AssoCarac* getAssoCarac(Collection* coll, Carac* carac, CType type, 
			void* entity, char* value);
AssoCarac* addAssoCarac(Collection* coll, Carac* carac, CType type, 
			void* entity, char* value);

AssoRole* getAssoRole(Collection* coll, Role* role, Human* human, 
		      Document* document);
AssoRole* addAssoRole(Collection* coll, Role* role, Human* human, 
		      Document* document);
int delAssoRole(Collection* coll, AssoRole* self);

int addHumanToCategory(Collection* coll, Human* human, Category* category);
int delHumanFromCategory(Collection* coll, Human* human, 
			 Category* category);
Human* getHuman(Collection* coll, char* firstName, char* secondName);
Human* addHuman(Collection* coll, char* firstName, char* secondName);
int delHuman(Collection* coll, Human* self);

int addArchiveToDocument(Collection* coll, Archive* archive, 
			 Document* document);
int delArchiveFromDocument(Collection* coll, Archive* archive, 
			   Document* document);
int addDocumentToCategory(Collection* coll, Document* document, 
			  Category* category);
int delDocumentFromCategory(Collection* coll, Document* document, 
			    Category* category);
Document* getDocument(Collection* coll, char* label);
Document* addDocument(Collection* coll, char* label);
int delDocument(Collection* coll, Document* self);

int addCategoryLink(Collection* coll, Category* father, Category* child);
int delCategoryLink(Collection* coll, Category* father, Category* child);
Category* getCategory(Collection* coll, char* label);
Category* addCategory(Collection* coll, char* label, int show);
int delCategory(Collection* coll, Category* self);

int diseaseCatalogTree(Collection* coll);

void* avl_insert_2(AVLTree* tree, void* item);

#endif /* MDTX_MEMORY_CATALOG_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
