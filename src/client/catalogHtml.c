/*=======================================================================
 * Version: $Id: catalogHtml.c,v 1.2 2014/11/13 16:36:17 nroche Exp $
 * Project: MediaTeX
 * Module : Archive catalog's latex serializer
 *
 * Archive catalog's latex serializer interface

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
#include "../misc/html.h"
#include "../memory/strdsm.h"
#include "../memory/ardsm.h"
#include "../memory/confTree.h"
#include "../memory/serverTree.h"
#include "../memory/catalogTree.h"
#include "../memory/extractTree.h"
#include "../common/openClose.h"
#include "commonHtml.h"
#include "catalogHtml.h"

/*=======================================================================
 * Function   : getHumanUri
 * Description: Get relative path to human page
 * Synopsis   : int getHumanUri(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
int getHumanUri(char* buf, char* path, int id) 
{
  int rc = FALSE;

  rc = getRelativeUri(buf, path, "humans");
  if (rc && id >= 0) rc = getItemUri(buf + strlen(buf), id, ".shtml");

  return rc;
}

/*=======================================================================
 * Function   : getDocListUri
 * Description: Get relative path to archive list page
 * Synopsis   : int getDocListUri(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
static int getDocListUri(char* buf, char* path, int id)
{
  int rc = FALSE;

  rc = getRelativeUri(buf, path, "docLists");
  if (rc && id > 0) rc = getListUri(buf + strlen(buf), id);
  
  return rc;
}

/*=======================================================================
 * Function   : getHumListUri
 * Description: Get relative path to archive list page
 * Synopsis   : int getHumListUri(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
static int getHumListUri(char* buf, char* path, int id) 
{
  int rc = FALSE;

  rc = getRelativeUri(buf, path, "humLists");
  if (rc && id > 0) rc = getListUri(buf + strlen(buf), id);
  
  return rc;
}

/*=======================================================================
 * Function   : getRoleListUri
 * Description: Get relative path to a role list page
 * Synopsis   : int getRoleListUri(char* buf, char* path, 
 *                                 int roleId, int listId)
 *              char* path = path to use as prefix
 *              int id = role id
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
int getRoleListUri(char* buf, char* path, int roleId, int listId)
{
  int rc = FALSE;


  rc = getRelativeUri(buf, path, "roles");
  rc = rc && (sprintf(buf + strlen(buf), "/%03i", roleId) > 0);    
  if (rc && listId > 0) rc = getListUri(buf + strlen(buf), listId);
  
  return rc;
}

/*=======================================================================
 * Function   : getCateListUri
 * Description: Get relative path to a category list page
 * Synopsis   : int getCateListUri(char* buf, char* path, 
 *                                 int cateId, int listId)
 *              char* path = path to use as prefix
 *              int id = category id
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
int getCateListUri(char* buf, char* path, int cateId, int listId)
{
  int rc = FALSE;


  rc = getRelativeUri(buf, path, "categories");
  rc = rc && (sprintf(buf + strlen(buf), "/%03i", cateId) > 0);    
  if (rc && listId > 0) rc = getListUri(buf + strlen(buf), listId);
  
  return rc;
}

/*=======================================================================
 * Function   : htmlIndexArchive
 * Description: HTML for Archive.
 * Synopsis   : static int htmlIndexArchive(FILE* fd, 
 *                                    Collection* coll, Archive* self)
 * Input      : FILE* fd = where to latexalize
 *              Archive* self = what to latexalize
 * Output     : TRUE on success
 =======================================================================*/
static int 
htmlIndexArchive(FILE* fd, Collection* coll, Archive* self)
{
  int rc = FALSE;
  AssoCarac *assoCarac = NULL;
  char url[512];
  char score[8];
  char* path = "floppy-icon.png";
  RGIT* curr = NULL;

  checkArchive(self);
  logEmit(LOG_DEBUG, "htmlIndexArchive: %s:%lli", 
	  self->hash, (long long int)self->size);

  if (!sprintf(url, 
	       "../../../cgi/get.cgi?hash=%s&size=%lli", 
	       self->hash, (long long int)self->size)) goto error;
  if (!sprintf(score, "(%5.2f)", self->extractScore)) goto error;

  htmlLiOpen(fd);

  // look for a thumbnail
  while ((assoCarac = rgNext_r(self->assoCaracs, &curr))) {
    if (!strcmp(assoCarac->carac->label, "icon")) {
      path = assoCarac->value;
      break;
    }
  }

  htmlImage(fd, "../../../icons", path, url);

  getArchiveUri(url, "../../../score", self);
  htmlLink(fd, NULL, url, score);

  /* latexalize caracs */
  if (!isEmptyRing(self->assoCaracs)) {
    rgRewind(self->assoCaracs);
    htmlUlOpen(fd);

    while ((assoCarac = rgNext(self->assoCaracs)) != NULL) {
      if (!strcmp(assoCarac->carac->label, "icon")) continue;
      if (!htmlAssoCarac(fd, assoCarac)) goto error;
    }

    htmlUlClose(fd);
  }

  htmlLiClose(fd);
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "htmlIndexArchive fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlRoleList
 * Description: Latexalize an Human list.
 * Synopsis   : int serializeHtmlHumList(Collection* coll, int i, int n)
 * Input      : Collection* coll
 *              int i = current role list
 *              int n = last role list
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlRoleList(Collection* coll, RG* assoRoles, int i, int n)
{
  int rc = FALSE;
  AssoRole* asso = NULL;
  FILE *fd = stdout;
  char *path = NULL;
  char url[128];
  char text[128];
  int j = 0;
  int roleId = -1;
  int humId = -1;

  checkCollection(coll);
  if (!assoRoles) goto error;
  asso = rgCurrent(assoRoles);
  roleId = asso->role->id;
  humId = asso->human->id;

  getRoleListUri(url, "/", roleId, i);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  fprintf(fd, "<!--#include virtual='../header.shtml' -->");
  htmlPOpen(fd);
  
  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;

  htmlUlOpen(fd);
  for (j = 0; j < MAX_INDEX_PER_PAGE && asso; ++j) {

    getHumanUri(url, "../../..", humId);
    if (!sprintf(text, "%s %s", asso->human->firstName,
		 asso->human->secondName)) goto error;
    htmlLiOpen(fd);
    htmlLink(fd, NULL, url, text);
    htmlLiClose(fd);

    while ((asso = rgNext(assoRoles)) != NULL && humId == asso->human->id);
    if (asso) humId = asso->human->id;
  }
  htmlUlClose(fd);

  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;

  htmlPClose(fd);
  htmlSSIFooter(fd, "../../../..");

   ++env.progBar.cur;
  rc = TRUE;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlRoleList fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : serializeHtmlRole
 * Description: HTML for Role.
 * Synopsis   : static int serializeHtmlRole(Collection* coll, Carac* self)
 * Input      : Collection* coll = context
 *              Carac* self = Role to serialize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlRole(Collection* coll, Role* self)
{
  int rc = FALSE;
  AssoRole* assoRole = NULL;
  FILE* fd = stdout;
  char* path = NULL;
  char* path2 = NULL;
  char tmp[128];
  int humId = -1;
  int nbHum = 0;
  int i = 0, n = 0;

  if(self == NULL) goto error;
  logEmit(LOG_DEBUG, "serializeHtmlRole %i: %s", self->id, self->label);

  // serialize the role header (included by each list)
  if (!getRoleListUri(tmp, "/", self->id, 0)) goto error;
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, tmp))) goto error;

  // build the directory for this role
  if (mkdir(path, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }

  // compute the number of human into this role
  if (!isEmptyRing(self->assos)) {
    if (!rgSort(self->assos, cmpAssoRole)) goto error; 

    rgRewind(self->assos);
    while ((assoRole = rgNext(self->assos)) != NULL) {
      if (humId == assoRole->human->id) continue;
      humId = assoRole->human->id;
      ++nbHum;
    }
  }

  // serialize header file for this role
  if (!(path2 = createString(path))) goto error;
  if (!(path2 = catString(path2, "/header.shtml"))) goto error;
  logEmit(LOG_INFO, "Serialize %s", path2); 
  if (!env.dryRun && (fd = fopen(path2, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path2, strerror(errno)); 
    goto error;
  }  
  htmlSSIHeader2(fd, "../../..", "index");
  htmlPOpen(fd);
  htmlBold(fd, _("Role "));
  if (!htmlCaps(fd, self->label)) goto error;
  if (!fprintf(fd, " : %i", nbHum)) goto error;
  htmlBr(fd);
  htmlPClose(fd);

  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }

  // serialize human lists for this role
  if (!isEmptyRing(self->assos)) {
 
    // build lists sub-directories (group by MAX_FILES_PER_DIR)
    n = (nbHum - 1) / MAX_INDEX_PER_PAGE +1;
    logEmit(LOG_INFO, "have %i human, so %i lists for role %s", 
	    nbHum, n, self->label);
    if (!htmlMakeDirs(path, n)) goto error;
      
    // populate role list subdirectories
    rgRewind(self->assos);
    rgNext(self->assos); // current point on first item
    for (i=1 ; i<=n; ++i) {
      if (!serializeHtmlRoleList(coll, self->assos, i, n)) goto error;
    }
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlRole fails");
  }
  path = destroyString(path);
  path2 = destroyString(path2);
  return rc;
}

/*=======================================================================
 * Function   : serializeHtmlHuman
 * Description: HTML for Human.
 * Synopsis   : static int serializeHtmlHuman(Collection* coll, Human* self)
 * Input      : Collection* coll = context
 *              Human* self = what to latexalize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlHuman(Collection* coll, Human* self)
{
  int rc = FALSE;
  Category* category = NULL;
  AssoCarac *assoCarac = NULL;
  AssoRole* assoRole = NULL;
  Role*     role = NULL;
  FILE* fd = stdout;
  char* path = NULL;
  char url[128];
  char text[128];
  int nbCategory = -1;

  if(self == NULL) goto error;
  logEmit(LOG_DEBUG, "serializeHtmlHuman %i: %s %s", 
	  self->id, self->firstName, self->secondName);

  getHumanUri(url, "/", self->id);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logEmit(LOG_INFO, "serialize: %s", path); 
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
    goto error;
  }  

  htmlSSIHeader(fd, "../../..", "index");

  if (!sprintf(text, "%s %s", self->firstName, self->secondName)) 
    goto error;
  
  htmlPOpen(fd);
  htmlBold(fd, _("Person"));
  if (!fprintf(fd, "%s", " : ")) goto error;
  if (!htmlCaps(fd, text)) goto error;
  htmlPClose(fd);

  // categories
  if (!isEmptyRing(self->categories)) {
    htmlPOpen(fd);
    if (!fprintf(fd, "%s", "(")) goto error;
    rgRewind(self->categories);
    while ((category = rgNext(self->categories)) != NULL) {
      getCateListUri(url, "../..", category->id, 1);
      fprintf(fd, "%s", (++nbCategory)?", ":""); // may return 0
      htmlLink(fd, NULL, url, category->label);
    }
    if (!fprintf(fd, "%s", ")")) goto error;
    htmlPClose(fd);
  }

  htmlBr(fd);

  // caracs
  if (!isEmptyRing(self->assoCaracs)) {
    htmlPOpen(fd);
    if (!fprintf(fd, "%s", _("Characteristics:\n"))) goto error;
    htmlUlOpen(fd);
    rgRewind(self->assoCaracs);
    while((assoCarac = rgNext(self->assoCaracs)) != NULL) {
      if (!htmlAssoCarac(fd, assoCarac)) goto error;
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // roles
  if (!isEmptyRing(self->assoRoles)) {
    htmlPOpen(fd);
    if (!fprintf(fd, "%s", _("\nRoles :\n"))) goto error;
    htmlUlOpen(fd);
    rgRewind(self->assoRoles);
    if (!rgSort(self->assoRoles, cmpAssoRole)) goto error;
    while ((assoRole = rgNext(self->assoRoles)) != NULL) {

      if (!role || assoRole->role != role) {
	if (role) htmlUlClose(fd);
	role = assoRole->role;
	htmlLiOpen(fd);
	getRoleListUri(url, "../..", assoRole->role->id, 1);
	htmlLink(fd, NULL, url, assoRole->role->label);
	htmlUlOpen(fd);
      }

      htmlLiOpen(fd);
      getDocumentUri(url, "../..", assoRole->document->id);
      if (!fprintf(fd, "%s", _("for "))) goto error;
      htmlLink(fd, NULL, url, assoRole->document->label);
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  htmlSSIFooter(fd, "../../..");

  ++env.progBar.cur;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlHuman fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : serializeHtmlDocument
 * Description: HTML for Document.
 * Synopsis   : static int serializeHtmlDocument(Collection* coll, 
 *                                                     Document* self)
 * Input      : Collection* coll = context
 *              Document* self = what to serialize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlDocument(Collection* coll, Document* self)
{
  int rc = FALSE;
  Category* category = NULL;
  AssoCarac  *assoCarac  = NULL;
  AssoRole* assoRole = NULL;
  Archive *archive = NULL;
  FILE* fd = stdout;
  char* path = NULL;
  char url[128];
  char text[128];
  int nbCategory = -1;

  if(self == NULL) goto error;
  logEmit(LOG_DEBUG, "serializeHtmlDocument %i: %s", self->id, self->label);

  getDocumentUri(url, "/", self->id);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logEmit(LOG_DEBUG, "serialize: %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
    goto error;
  }  

  htmlSSIHeader(fd, "../../..", "index");

  htmlPOpen(fd);
  htmlBold(fd, "Document");
  if (!fprintf(fd, "%s", " : ")) goto error;
  if (!htmlCaps(fd, self->label)) goto error;
  htmlPClose(fd);
  
  // latexalize categories
  if (!isEmptyRing(self->categories)) {
    htmlPOpen(fd);
    if (!fprintf(fd, "%s", "(")) goto error;
    rgRewind(self->categories);
    while ((category = rgNext(self->categories)) != NULL) {
      getCateListUri(url, "../..", category->id, 1);
      fprintf(fd, "%s", (++nbCategory)?", ":""); // may return 0
      htmlLink(fd, NULL, url, category->label);
    }
    if (!fprintf(fd, "%s", ")")) goto error;
    htmlPClose(fd);
  }

  htmlBr(fd);

  // caracs
  if (!isEmptyRing(self->assoCaracs)) {
    htmlPOpen(fd);
    if (!fprintf(fd, "%s", _("Characteristics:\n"))) goto error;
    htmlUlOpen(fd);
    rgRewind(self->assoCaracs);
    while((assoCarac = rgNext(self->assoCaracs)) != NULL) {
      if (!htmlAssoCarac(fd, assoCarac)) goto error;
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // roles
  if (!isEmptyRing(self->assoRoles)) {
    htmlPOpen(fd);
    if (!fprintf(fd, "%s", _("\nParteners:\n"))) goto error;
    htmlUlOpen(fd);
    rgRewind(self->assoRoles);
    while ((assoRole = rgNext(self->assoRoles)) != NULL) {
      htmlLiOpen(fd);

      getRoleListUri(url, "../..", assoRole->role->id, 1);
      htmlLink(fd, NULL, url, assoRole->role->label);
      if (!fprintf(fd, "%s", " : ")) goto error;

      getHumanUri(url, "../..", assoRole->human->id);
      if (!sprintf(text, "%s %s", assoRole->human->firstName,
		   assoRole->human->secondName)) goto error;

      htmlLink(fd, NULL, url, text);
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // archives
  htmlPOpen(fd);
  if (!fprintf(fd, _("\nRecords:\n"))) goto error;
  htmlUlOpen(fd);
  if (!isEmptyRing(self->archives)) {
    rgRewind(self->archives); 
    while ((archive = rgNext(self->archives)) != NULL) {
      htmlIndexArchive(fd, coll, archive);
    }
  }
  else {
    htmlLiOpen(fd);
    if (!fprintf(fd, _("No record\n"))) goto error;
    htmlLiClose(fd);
  }
  htmlUlClose(fd);
  htmlPClose(fd);
 
  htmlSSIFooter(fd, "../../..");

  ++env.progBar.cur;
  rc = TRUE;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlDocument fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlCateList
 * Description: Latexalize an Human list.
 * Synopsis   : int serializeHtmlHumList(Collection* coll, int i, int n)
 * Input      : Collection* coll
 *              int i = current category list
 *              int n = last cate list
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlCateList(Collection* coll, Category* self, int i, int n)
{
  int rc = FALSE;
  RG* documents = NULL;
  Document* document = NULL;
  FILE *fd = stdout;
  char *path = NULL;
  char url[128];
  char text[128];
  int j = 0;
  int cateId = -1;

  checkCollection(coll);
  if (!self) goto error;
  cateId = self->id;

  getCateListUri(url, "/", cateId, i);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  fprintf(fd, "<!--#include virtual='../header.shtml' -->");

  if (isEmptyRing(documents = self->documents)) goto end;
  document = rgCurrent(documents);
  htmlPOpen(fd);
  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;

  htmlUlOpen(fd);
  for (j = 0; j < MAX_INDEX_PER_PAGE && document; ++j) {

    getDocumentUri(url, "../../..", document->id);
    if (!sprintf(text, "%s", document->label)) goto error;
    htmlLiOpen(fd);
    htmlLink(fd, NULL, url, text);
    htmlLiClose(fd);

    document = rgNext(documents);
  }
  htmlUlClose(fd);

  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;
  htmlPClose(fd);

 end:
  htmlSSIFooter(fd, "../../../..");

  ++env.progBar.cur;
  rc = TRUE;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlRoleList fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : serializeHtmlCategory
 * Description: HTML for Category.
 * Synopsis   : static int serializeHtmlCategory(Collection* coll, 
 *                                                       Category* self)
 * Input      : Collection* coll = context
 *              Category* self = what to serialize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlCategory(Collection* coll, Category* self)
{
  int rc = FALSE;
  AssoCarac  *assoCarac = NULL;
  Human *human = NULL;
  Category* cat = NULL;
  FILE* fd = stdout;
  char* path = NULL;
  char* path2 = NULL;
  char tmp[128];
  char url[128];
  int nbDoc = 0, i = 0, n = 0;

  if(self == NULL) goto error;
  logEmit(LOG_DEBUG, "serializeHtmlCategory %i: %s",
  	  self->id, self->label);

  // serialize the category header (included by each list of doc)
  if (!getCateListUri(tmp, "/", self->id, 0)) goto error;
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, tmp))) goto error;

  // build the directory for this category
  if (mkdir(path, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }

  // serialize header file for this role
  if (!(path2 = createString(path))) goto error;
  if (!(path2 = catString(path2, "/header.shtml"))) goto error;
  logEmit(LOG_INFO, "Serialize %s", path2);
  if (!env.dryRun && (fd = fopen(path2, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path2, strerror(errno));
    goto error;
  }
  htmlSSIHeader2(fd, "../../..", "index");
  htmlPOpen(fd);
  htmlBold(fd, _("Class"));
  if (!fprintf(fd, "%s", " : ")) goto error;
  if (!htmlCaps(fd, self->label)) goto error;
  htmlBr(fd);
  htmlPClose(fd);
  htmlBr(fd);

  // father
  if (!isEmptyRing(self->fathers)) {
    htmlPOpen(fd);
    if (!fprintf(fd, "%s", _("\nParent classes: "))) goto error;
    htmlUlOpen(fd);
    rgRewind(self->fathers);
    while ((cat = rgNext(self->fathers)) != NULL) {
      getCateListUri(url, "../../..", cat->id, 1);
      htmlLiOpen(fd);
      htmlLink(fd, NULL, url, cat->label);
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // childs
  if (!isEmptyRing(self->childs)) {
    htmlPOpen(fd);
    if (!fprintf(fd, "%s", _("\nSub-classes:\n"))) goto error;
    htmlUlOpen(fd);
    rgRewind(self->childs);
    while ((cat = rgNext(self->childs)) != NULL) {
        getCateListUri(url, "../../..", cat->id, 1);
      htmlLiOpen(fd);
      htmlLink(fd, NULL, url, cat->label);
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // caracs
  if (!isEmptyRing(self->assoCaracs)) {
    htmlPOpen(fd);
    if (!fprintf(fd, "%s", _("\nCharacteristics:\n"))) goto error;
    htmlUlOpen(fd);
    rgRewind(self->assoCaracs);
    while((assoCarac = rgNext(self->assoCaracs)) != NULL) {
      if (!htmlAssoCarac(fd, assoCarac)) goto error;
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // humans
  if (!isEmptyRing(self->humans)) {
    htmlPOpen(fd);
    if (!fprintf(fd, _("\nParteners:\n"))) goto error;
    htmlUlOpen(fd);
    if (!rgSort(self->humans, cmpHuman)) goto error;
    while ((human = rgNext(self->humans)) != NULL) {
      getHumanUri(url, "../../..", human->id);
      if (!sprintf(tmp, "%s %s", human->firstName, human->secondName))
  	goto error;

      htmlLiOpen(fd);
      htmlLink(fd, NULL, url, tmp);
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

   // documents
  nbDoc = self->documents->nbItems;
  htmlPOpen(fd);
  if (!fprintf(fd, _("\nDocuments: %i\n"), nbDoc)) goto error;

  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }

  // serialize document lists for this category
  if (!isEmptyRing(self->documents)) {
    if (!rgSort(self->documents, cmpDocument)) goto error;

    // compute the number of document into this category
    n = (nbDoc - 1) / MAX_INDEX_PER_PAGE +1;
    logEmit(LOG_INFO, "have %i doc, so %i lists for category %s",
  	    nbDoc, n, self->label);

    // build lists sub-directories (group by MAX_FILES_PER_DIR)
    if (!htmlMakeDirs(path, n)) goto error;
      
    // populate category list subdirectories
    rgRewind(self->documents);
    rgNext(self->documents); // current point on first item
    for (i=1 ; i<=n; ++i) {
      if (!serializeHtmlCateList(coll, self, i, n)) goto error;
    }
  }
  else {
    // serialize an empty file to be loaded anyway
    if (!htmlMakeDirs(path, 1)) goto error;
    if (!serializeHtmlCateList(coll, self, 1, 0)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlCategory fails");
  }
  path = destroyString(path);
  path2 = destroyString(path2);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlDocList
 * Description: Latexalize an Document list.
 * Synopsis   : int serializeHtmlDocList(Collection* coll, int i, int n)
 * Input      : Collection* coll
 *              int i = current document list
 *              int n = last document list
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlDocList(Collection* coll, AVLNode **node, int i, int n)
{
  int rc = FALSE;
  Document* document = NULL;
  FILE *fd = stdout;
  char *path = NULL;
  char url[128];
  int j = 0;

  checkCollection(coll);

  getDocListUri(url, "/", i);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  fprintf(fd, "<!--#include virtual='../header.shtml' -->");
  if (!node) goto end; // empty page if needed

  htmlPOpen(fd);
  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;

  htmlUlOpen(fd);
  for (j = 0; j < MAX_INDEX_PER_PAGE && *node; ++j) {
    document = (Document*)(*node)->item;

    getDocumentUri(url, "../..", document->id);
    htmlLiOpen(fd);
    htmlLink(fd, NULL, url, document->label);
    htmlLiClose(fd);

    *node = (*node)->next;
  }
  htmlUlClose(fd);

  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;
  htmlPClose(fd);
 end:
  htmlSSIFooter(fd, "../../..");

  ++env.progBar.cur;
  rc = TRUE;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlDocList fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : serializeHtmlDocLists
 * Description: HTML for Role.
 * Synopsis   : static int serializeHtmlDocLists(Collection* coll, Carac* self)
 * Input      : Collection* coll = context
 *              Carac* self = Role to serialize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlDocLists(Collection* coll, CatalogTree* self)
{
  int rc = FALSE;
  AVLNode *node = NULL;
  FILE* fd = stdout;
  char tmp[128];
  char* path = NULL;
  char* path2 = NULL;
  int nbDoc = 0;
  int i = 0;
  int n = 0;

  if(self == NULL) goto error;
  logEmit(LOG_DEBUG, "%s", "serializeHtmlDocLists");

  nbDoc = avl_count(self->documents);
  if (!getDocListUri(tmp, "/", 0)) goto error;
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, tmp))) goto error;

  // build the directory
  if (mkdir(path, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }
  // serialize header file for the document list
  if (!(path2 = createString(path))) goto error;
  if (!(path2 = catString(path2, "/header.shtml"))) goto error;
  logEmit(LOG_INFO, "Serialize %s", path2); 
  if (!env.dryRun && (fd = fopen(path2, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path2, strerror(errno)); 
    goto error;
  }  
  htmlSSIHeader2(fd, "../..", "index");
  htmlPOpen(fd);
  htmlBold(fd, _("All documents "));
  if (!fprintf(fd, " : %i", nbDoc)) goto error;
  htmlBr(fd);
  htmlPClose(fd);

  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }

  // get the total number of lists of documents
  n = (nbDoc - 1) / MAX_INDEX_PER_PAGE +1;
  logEmit(LOG_INFO, "have %i document lists", n);

  // build lists sub-directories (group by MAX_FILES_PER_DIR)
  if (!htmlMakeDirs(path, n)) goto error;
  
  // serialize lists
  node = self->documents->head;
  for (i=1 ; i<=n; ++i) {
    if (!serializeHtmlDocList(coll, &node, i, n)) goto error;
  }

  // empty list if needed
  if (n == 0) {
    if (!htmlMakeDirs(path, 1)) goto error;
    if (!serializeHtmlDocList(coll, NULL, 1, 0)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlDocLists fails");
  }
  path2 = destroyString(path2);
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlHumList
 * Description: Latexalize an Human list.
 * Synopsis   : int serializeHtmlHumList(Collection* coll, int i, int n)
 * Input      : Collection* coll
 *              int i = current human list
 *              int n = last human list
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlHumList(Collection* coll, AVLNode **node, int i, int n)
{
  int rc = FALSE;
  CatalogTree* self = NULL;
  Human* human = NULL;
  FILE *fd = stdout;
  char *path = NULL;
  char url[128];
  char text[128];
  int j = 0;

  checkCollection(coll);
  if (!(self = coll->catalogTree)) goto error;

  getHumListUri(url, "/", i);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  fprintf(fd, "<!--#include virtual='../header.shtml' -->");
  htmlPOpen(fd);

  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;

  htmlUlOpen(fd);
  for (j = 0; j < MAX_INDEX_PER_PAGE && *node; ++j) {
    human = (Human*)(*node)->item;

    getHumanUri(url, "../..", human->id);
    if (!sprintf(text, "%s %s", human->firstName,
		 human->secondName)) goto error;
    htmlLiOpen(fd);
    htmlLink(fd, NULL, url, text);
    htmlLiClose(fd);

    *node = (*node)->next;
  }
  htmlUlClose(fd);

  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;

  htmlPClose(fd);
  htmlSSIFooter(fd, "../../..");

  ++env.progBar.cur;
  rc = TRUE;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlHumList fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : serializeHtmlHumLists
 * Description: HTML for Role.
 * Synopsis   : static int serializeHtmlHumLists(Collection* coll, 
 *                                                 Carac* self)
 * Input      : Collection* coll = context
 *              Carac* self = Role to serialize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlHumLists(Collection* coll, CatalogTree* self)
{
  int rc = FALSE;
  AVLNode *node = NULL;
  FILE* fd = stdout;
  char tmp[128];
  char* path = NULL;
  char* path2 = NULL;
  int nbHum = 0;
  int i = 0;
  int n = 0;

  if(self == NULL) goto error;
  logEmit(LOG_DEBUG, "%s", "serializeHtmlDocLists");

  nbHum = avl_count(self->humans);
  if (!getHumListUri(tmp, "/", 0)) goto error;
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, tmp))) goto error;

  // build the directory
  if (mkdir(path, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }
  // serialize header file for this role
  if (!(path2 = createString(path))) goto error;
  if (!(path2 = catString(path2, "/header.shtml"))) goto error;
  logEmit(LOG_INFO, "Serialize %s", path2); 
  if (!env.dryRun && (fd = fopen(path2, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path2, strerror(errno)); 
    goto error;
  }  
  htmlSSIHeader2(fd, "../..", "index");
  htmlPOpen(fd);
  htmlBold(fd, _("All parteners "));
  if (!fprintf(fd, " : %i", nbHum)) goto error;
  htmlBr(fd);
  htmlPClose(fd);

  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }

  // get the total number of lists of documents
  n = (nbHum - 1) / MAX_INDEX_PER_PAGE +1;
  logEmit(LOG_INFO, "have %i human lists", n);
  if (nbHum <= 0) goto end;

  // build lists sub-directories (group by MAX_FILES_PER_DIR)
  if (!htmlMakeDirs(path, n)) goto error;
  
  // serialize lists
  node = self->humans->head;
  n = (nbHum - 1) / MAX_INDEX_PER_PAGE +1;
  for (i=1 ; i<=n; ++i) {
    if (!serializeHtmlHumList(coll, &node, i, n)) goto error;
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlHumLists fails");
  }
  path2 = destroyString(path2);
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlMainIndex
 * Description: Serialize index.shtml file
 * Synopsis   : int serializeHtmlMainIndex(Collection* coll)
 * Input      : Collection* coll = context
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlMainIndex(Collection* coll)
{ 
  int rc = FALSE;
  CatalogTree* self = NULL;
  FILE* fd = stdout;
  char* path = NULL;

  if (!(self = coll->catalogTree)) goto error;

  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, "/index.shtml"))) goto error;
  logEmit(LOG_DEBUG, "serialize: %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
    goto error;
  }  

  htmlSSIHeader(fd, "..", "index");

  htmlPOpen(fd);
  htmlBold(fd, _("Welcome!"));
  if (!fprintf(fd, "%s", _(" on collection "))) goto error;
  if (!htmlCaps(fd, coll->label)) goto error;
  htmlBr(fd);
  if (!fprintf(fd, "%s", _("(you are on "))) goto error;
  if (!htmlCaps(fd, getConfiguration()->host)) goto error;
  if (!fprintf(fd, "%s", _(" server)"))) goto error;
  htmlBr(fd);
  htmlPClose(fd);
  
  /* TODO: include user's text here */
   
  htmlSSIFooter(fd, "..");

  rc = TRUE;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlMainIndex fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : htmlCategoryMenu
 * Description: Add an entries into the menu
 * Synopsis   : static int htmlCategoryMenu(FILE* fd, 
 *                                       Category* self, int depth)
 * Input      : FILE* fd : file to write
 *              Category* self : entry to latexalize
 *              int depth : indentation in the menu
 * Output     : N/A
 =======================================================================*/
static int
htmlCategoryMenu(FILE* fd, Category* self, int depth)
{
  int rc = FALSE;
  Category* category = NULL;
  char url[128];

  if (self == NULL) goto error;
  logEmit(LOG_DEBUG, "htmlCategoryMenu %i", depth);

  getCateListUri(url, "<!--#echo var='HOME' -->/index", self->id, 1);
  if (depth>0) htmlLiOpen(fd);
  htmlLink(fd, NULL, url, self->label);

  /* childs */
  rgRewind(self->childs);
  if (!isEmptyRing(self->childs)) {
    htmlUlOpen(fd);
    while((category = rgNext(self->childs)) != NULL) {
      if (category->show && !htmlCategoryMenu(fd, category, depth+1)) 
	goto error;
    }
    htmlUlClose(fd);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "htmlCategoryMenu fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : serializeHtmlIndexHeader
 * Description: This template will be used by apache SSI
 * Synopsis   : static int serializeHtmlIndexHeader(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlIndexHeader(Collection* coll)
{ 
  int rc = FALSE;
  CatalogTree* self = NULL;
  Category* category = NULL;
  Role* role = NULL;
  FILE* fd = stdout;
  char* path = NULL;
  char url[512];

  if (!(self = coll->catalogTree)) goto error;

  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/indexHeader.shtml"))) goto error;
  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if (!htmlMainHead(fd, "Index")) goto error;
  if (!htmlLeftPageHead(fd, "index")) goto error;

  htmlPOpen(fd);
  getDocListUri(url, "<!--#echo var='HOME' -->/index", 1);
  htmlLink(fd, NULL, url, _("All documents"));
  htmlPClose(fd);

  // categories
  if (!isEmptyRing(self->categories)) {
    htmlPOpen(fd);
    if (!fprintf(fd, _("\nClasses:\n"))) goto error;
    htmlUlOpen(fd);
    rgRewind(self->categories);
    while((category = rgNext(self->categories)) != NULL) {
      if (category->show && isEmptyRing(category->fathers)) {
	htmlLiOpen(fd);
	if (!htmlCategoryMenu(fd, category, 0)) goto error;
	htmlLiClose(fd);
      }
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // roles
  if (!isEmptyRing(self->roles)) {
    htmlPOpen(fd);

    getHumListUri(url, "<!--#echo var='HOME' -->/index", 1);
    htmlLink(fd, NULL, url, _("Parteners:"));

    htmlUlOpen(fd);
    rgRewind(self->roles);
    while ((role = rgNext(self->roles)) != NULL) {
      getRoleListUri(url, "<!--#echo var='HOME' -->/index", role->id, 1);
      htmlLiOpen(fd);
      htmlLink(fd, NULL, url, role->label);
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // master url (other are relatives ones)
  if (!sprintf(url, "https://%s/~%s", coll->masterHost, coll->masterUser)) 
    goto error;

  if (!htmlLeftPageTail(fd)) goto error;
  if (!htmlRightHead(fd, url)) goto error;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlIndexHeader fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlIndex
 * Description: Generate HTML for index directory
 * Synopsis   : int serializeHtmlIndex(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeHtmlIndex(Collection* coll)
{ 
  int rc = FALSE;
  CatalogTree* self = NULL;
  Category *category = NULL;
  Document *document = NULL;
  Human *human = NULL;
  Role *role = NULL;
  AVLNode *node = NULL;
  char tmp[128];
  char *path1 = NULL;
  char *path2 = NULL;
  int nbDoc = 0;
  int nbHum = 0;

  checkCollection(coll);
  if (!(self = coll->catalogTree)) goto error;
  logEmit(LOG_DEBUG, "serializeHtmlIndex: %s collection", coll->label);

  nbDoc = avl_count(self->documents);
  nbHum = avl_count(self->humans);

  // directories
  if (!(path1 = createString(coll->htmlIndexDir))) goto error;
  if (!(path1 = catString(path1, "/roles"))) goto error;
  if (mkdir(path1, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }
  path1 = destroyString(path1);
  if (!(path1 = createString(coll->htmlIndexDir))) goto error;
  if (!(path1 = catString(path1, "/categories"))) goto error;
  if (mkdir(path1, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }

  // documents directories
  path1 = destroyString(path1);
  if (!getDocumentUri(tmp, "/", -1)) goto error;
  if (!(path1 = createString(coll->htmlIndexDir))) goto error;
  if (!(path1 = catString(path1, tmp))) goto error;
  if (mkdir(path1, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }

  if (nbDoc > 0) {
    logEmit(LOG_INFO, "have %i documents", nbDoc);
    if (!htmlMakeDirs(path1, nbDoc)) goto error;
  }

  // humans directories
  path1 = destroyString(path1);
  if (!getHumanUri(tmp, "/", -1)) goto error;
  if (!(path1 = createString(coll->htmlIndexDir))) goto error;
  if (!(path1 = catString(path1, tmp))) goto error;
  if (mkdir(path1, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }
  
  if (nbHum > 0) {
    logEmit(LOG_INFO, "have %i humans", nbHum);
    if (!htmlMakeDirs(path1, nbHum)) goto error;
  }

  // documents
  if (nbDoc > 0) {
    for(node = self->documents->head; node; node = node->next) {
      document = (Document*)node->item;
      if (!serializeHtmlDocument(coll, document)) goto error;
    }
  }

  // humans
  if (nbHum > 0) {
    for(node = self->humans->head; node; node = node->next) {
      human = (Human*)node->item;
      if (!serializeHtmlHuman(coll, human)) goto error;
    }
  }

  // roles
  if (!isEmptyRing(self->roles)) {
    if (!rgSort(self->roles, cmpRole)) goto error;
    while((role = rgNext(self->roles)) != NULL) {
      if (!serializeHtmlRole(coll, role)) goto error;
    }
  }
  
  // categories
  if (!isEmptyRing(self->categories)) {
    if (!rgSort(self->categories, cmpCategory)) goto error;
    while((category = rgNext(self->categories)) != NULL) {
      if (!serializeHtmlCategory(coll, category)) goto error;
    }
  }
  
  // others
  if (!serializeHtmlDocLists(coll, self)) goto error;
  if (!serializeHtmlHumLists(coll, self)) goto error;
  if (!serializeHtmlIndexHeader(coll)) goto error;
  if (!serializeHtmlMainIndex(coll)) goto error;
  
  rc = TRUE;  
 error:
  path1 = destroyString(path1);
  path2 = destroyString(path2);
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "../parser/confFile.tab.h"
#include "../parser/serverFile.tab.h"
#include "../parser/catalogFile.tab.h"
#include "../parser/extractFile.tab.h"
#include "extractHtml.h"
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
  mdtxUsage(programName);

  mdtxOptions();
  //fprintf(stderr, "  ---\n");
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
  char* options = MDTX_SHORT_OPTIONS;
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {0, 0, 0, 0}
  };
       
  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll = mdtxGetCollection("coll1"))) goto error;
  if (!loadCollection(coll, CTLG)) goto error;
  if (!serializeHtmlIndex(coll)) goto error;
  if (!releaseCollection(coll, CTLG)) goto error;
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
