/*=======================================================================
 * Version: $Id: catalogHtml.c,v 1.8 2015/08/07 17:50:27 nroche Exp $
 * Project: MediaTeX
 * Module : catalogHtml
 *
 * HTML catalog serializer

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
  AssoCarac *assoCarac = 0;
  char url[512];
  char score[8];
  char* path = "floppy-icon.png";
  RGIT* curr = 0;

  checkArchive(self);
  logMain(LOG_DEBUG, "htmlIndexArchive: %s:%lli", 
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
  htmlLink(fd, 0, url, score);

  /* latexalize caracs */
  if (!isEmptyRing(self->assoCaracs)) {
    rgRewind(self->assoCaracs);
    htmlUlOpen(fd);

    while ((assoCarac = rgNext(self->assoCaracs)) != 0) {
      if (!strcmp(assoCarac->carac->label, "icon")) continue;
      if (!htmlAssoCarac(fd, assoCarac)) goto error;
    }

    htmlUlClose(fd);
  }

  htmlLiClose(fd);
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "htmlIndexArchive fails");
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
  AssoRole* asso = 0;
  FILE *fd = stdout;
  char *path = 0;
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

  logMain(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
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
    htmlLink(fd, 0, url, text);
    htmlLiClose(fd);

    while ((asso = rgNext(assoRoles)) != 0 && humId == asso->human->id);
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
    logMain(LOG_ERR, "serializeHtmlRoleList fails");
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
  AssoRole* assoRole = 0;
  FILE* fd = stdout;
  char* path = 0;
  char* path2 = 0;
  char tmp[128];
  int humId = -1;
  int nbHum = 0;
  int i = 0, n = 0;

  if(self == 0) goto error;
  logMain(LOG_DEBUG, "serializeHtmlRole %i: %s", self->id, self->label);

  // serialize the role header (included by each list)
  if (!getRoleListUri(tmp, "/", self->id, 0)) goto error;
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, tmp))) goto error;

  // build the directory for this role
  if (mkdir(path, 0755)) {
    if (errno != EEXIST) {
      logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }

  // compute the number of human into this role
  if (!isEmptyRing(self->assos)) {
    if (!rgSort(self->assos, cmpAssoRole)) goto error; 

    rgRewind(self->assos);
    while ((assoRole = rgNext(self->assos)) != 0) {
      if (humId == assoRole->human->id) continue;
      humId = assoRole->human->id;
      ++nbHum;
    }
  }

  // serialize header file for this role
  if (!(path2 = createString(path))) goto error;
  if (!(path2 = catString(path2, "/header.shtml"))) goto error;
  logMain(LOG_DEBUG, "Serialize %s", path2); 
  if (!env.dryRun && (fd = fopen(path2, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path2, strerror(errno)); 
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
    logMain(LOG_DEBUG, "have %i human, so %i lists for role %s", 
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
    logMain(LOG_ERR, "serializeHtmlRole fails");
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
  Category* category = 0;
  AssoCarac *assoCarac = 0;
  AssoRole* assoRole = 0;
  Role*     role = 0;
  FILE* fd = stdout;
  char* path = 0;
  char url[128];
  char text[128];
  int nbCategory = -1;

  if(self == 0) goto error;
  logMain(LOG_DEBUG, "serializeHtmlHuman %i: %s %s", 
	  self->id, self->firstName, self->secondName);

  getHumanUri(url, "/", self->id);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logMain(LOG_DEBUG, "serialize: %s", path); 
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
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
    while ((category = rgNext(self->categories)) != 0) {
      getCateListUri(url, "../..", category->id, 1);
      fprintf(fd, "%s", (++nbCategory)?", ":""); // may return 0
      htmlLink(fd, 0, url, category->label);
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
    while((assoCarac = rgNext(self->assoCaracs)) != 0) {
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
    while ((assoRole = rgNext(self->assoRoles)) != 0) {

      if (!role || assoRole->role != role) {
	if (role) htmlUlClose(fd);
	role = assoRole->role;
	htmlLiOpen(fd);
	getRoleListUri(url, "../..", assoRole->role->id, 1);
	htmlLink(fd, 0, url, assoRole->role->label);
	htmlUlOpen(fd);
      }

      htmlLiOpen(fd);
      getDocumentUri(url, "../..", assoRole->document->id);
      if (!fprintf(fd, "%s", _("for "))) goto error;
      htmlLink(fd, 0, url, assoRole->document->label);
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
    logMain(LOG_ERR, "serializeHtmlHuman fails");
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
  Category* category = 0;
  AssoCarac  *assoCarac  = 0;
  AssoRole* assoRole = 0;
  Archive *archive = 0;
  FILE* fd = stdout;
  char* path = 0;
  char url[128];
  char text[128];
  int nbCategory = -1;

  if(self == 0) goto error;
  logMain(LOG_DEBUG, "serializeHtmlDocument %i: %s", self->id, self->label);

  getDocumentUri(url, "/", self->id);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logMain(LOG_DEBUG, "serialize: %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
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
    while ((category = rgNext(self->categories)) != 0) {
      getCateListUri(url, "../..", category->id, 1);
      fprintf(fd, "%s", (++nbCategory)?", ":""); // may return 0
      htmlLink(fd, 0, url, category->label);
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
    while((assoCarac = rgNext(self->assoCaracs)) != 0) {
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
    while ((assoRole = rgNext(self->assoRoles)) != 0) {
      htmlLiOpen(fd);

      getRoleListUri(url, "../..", assoRole->role->id, 1);
      htmlLink(fd, 0, url, assoRole->role->label);
      if (!fprintf(fd, "%s", " : ")) goto error;

      getHumanUri(url, "../..", assoRole->human->id);
      if (!sprintf(text, "%s %s", assoRole->human->firstName,
		   assoRole->human->secondName)) goto error;

      htmlLink(fd, 0, url, text);
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
    while ((archive = rgNext(self->archives)) != 0) {
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
    logMain(LOG_ERR, "serializeHtmlDocument fails");
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
  RG* documents = 0;
  Document* document = 0;
  FILE *fd = stdout;
  char *path = 0;
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

  logMain(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
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
    htmlLink(fd, 0, url, text);
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
    logMain(LOG_ERR, "serializeHtmlRoleList fails");
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
  AssoCarac  *assoCarac = 0;
  Human *human = 0;
  Category* cat = 0;
  FILE* fd = stdout;
  char* path = 0;
  char* path2 = 0;
  char tmp[128];
  char url[128];
  int nbDoc = 0, i = 0, n = 0;

  if(self == 0) goto error;
  logMain(LOG_DEBUG, "serializeHtmlCategory %i: %s",
  	  self->id, self->label);

  // serialize the category header (included by each list of doc)
  if (!getCateListUri(tmp, "/", self->id, 0)) goto error;
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, tmp))) goto error;

  // build the directory for this category
  if (mkdir(path, 0755)) {
    if (errno != EEXIST) {
      logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }

  // serialize header file for this role
  if (!(path2 = createString(path))) goto error;
  if (!(path2 = catString(path2, "/header.shtml"))) goto error;
  logMain(LOG_DEBUG, "Serialize %s", path2);
  if (!env.dryRun && (fd = fopen(path2, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path2, strerror(errno));
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
    while ((cat = rgNext(self->fathers)) != 0) {
      getCateListUri(url, "../../..", cat->id, 1);
      htmlLiOpen(fd);
      htmlLink(fd, 0, url, cat->label);
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
    while ((cat = rgNext(self->childs)) != 0) {
        getCateListUri(url, "../../..", cat->id, 1);
      htmlLiOpen(fd);
      htmlLink(fd, 0, url, cat->label);
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
    while((assoCarac = rgNext(self->assoCaracs)) != 0) {
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
    while ((human = rgNext(self->humans)) != 0) {
      getHumanUri(url, "../../..", human->id);
      if (!sprintf(tmp, "%s %s", human->firstName, human->secondName))
  	goto error;

      htmlLiOpen(fd);
      htmlLink(fd, 0, url, tmp);
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
    logMain(LOG_DEBUG, "have %i doc, so %i lists for category %s",
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
    logMain(LOG_ERR, "serializeHtmlCategory fails");
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
  Document* document = 0;
  FILE *fd = stdout;
  char *path = 0;
  char url[128];
  int j = 0;

  checkCollection(coll);

  getDocListUri(url, "/", i);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logMain(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
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
    htmlLink(fd, 0, url, document->label);
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
    logMain(LOG_ERR, "serializeHtmlDocList fails");
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
  AVLNode *node = 0;
  FILE* fd = stdout;
  char tmp[128];
  char* path = 0;
  char* path2 = 0;
  int nbDoc = 0;
  int i = 0;
  int n = 0;

  if(self == 0) goto error;
  logMain(LOG_DEBUG, "serializeHtmlDocLists");

  nbDoc = avl_count(self->documents);
  if (!getDocListUri(tmp, "/", 0)) goto error;
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, tmp))) goto error;

  // build the directory
  if (mkdir(path, 0755)) {
    if (errno != EEXIST) {
      logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }
  // serialize header file for the document list
  if (!(path2 = createString(path))) goto error;
  if (!(path2 = catString(path2, "/header.shtml"))) goto error;
  logMain(LOG_DEBUG, "Serialize %s", path2); 
  if (!env.dryRun && (fd = fopen(path2, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path2, strerror(errno)); 
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
  logMain(LOG_DEBUG, "have %i document lists", n);

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
    if (!serializeHtmlDocList(coll, 0, 1, 0)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "serializeHtmlDocLists fails");
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
  CatalogTree* self = 0;
  Human* human = 0;
  FILE *fd = stdout;
  char *path = 0;
  char url[128];
  char text[128];
  int j = 0;

  checkCollection(coll);
  if (!(self = coll->catalogTree)) goto error;

  getHumListUri(url, "/", i);
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logMain(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
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
    htmlLink(fd, 0, url, text);
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
    logMain(LOG_ERR, "serializeHtmlHumList fails");
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
  AVLNode *node = 0;
  FILE* fd = stdout;
  char tmp[128];
  char* path = 0;
  char* path2 = 0;
  int nbHum = 0;
  int i = 0;
  int n = 0;

  if(self == 0) goto error;
  logMain(LOG_DEBUG, "serializeHtmlDocLists");

  nbHum = avl_count(self->humans);
  if (!getHumListUri(tmp, "/", 0)) goto error;
  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, tmp))) goto error;

  // build the directory
  if (mkdir(path, 0755)) {
    if (errno != EEXIST) {
      logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }
  // serialize header file for this role
  if (!(path2 = createString(path))) goto error;
  if (!(path2 = catString(path2, "/header.shtml"))) goto error;
  logMain(LOG_DEBUG, "Serialize %s", path2); 
  if (!env.dryRun && (fd = fopen(path2, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path2, strerror(errno)); 
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
  logMain(LOG_DEBUG, "have %i human lists", n);
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
    logMain(LOG_ERR, "serializeHtmlHumLists fails");
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
  CatalogTree* self = 0;
  FILE* fd = stdout;
  char* path = 0;

  if (!(self = coll->catalogTree)) goto error;

  if (!(path = createString(coll->htmlIndexDir))) goto error;
  if (!(path = catString(path, "/index.shtml"))) goto error;
  logMain(LOG_DEBUG, "serialize: %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
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
    logMain(LOG_ERR, "serializeHtmlMainIndex fails");
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
  Category* category = 0;
  char url[128];

  if (self == 0) goto error;
  logMain(LOG_DEBUG, "htmlCategoryMenu %i", depth);

  getCateListUri(url, "<!--#echo var='HOME' -->/index", self->id, 1);
  if (depth>0) htmlLiOpen(fd);
  htmlLink(fd, 0, url, self->label);

  /* childs */
  rgRewind(self->childs);
  if (!isEmptyRing(self->childs)) {
    htmlUlOpen(fd);
    while((category = rgNext(self->childs)) != 0) {
      if (category->show && !htmlCategoryMenu(fd, category, depth+1)) 
	goto error;
    }
    htmlUlClose(fd);
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "htmlCategoryMenu fails");
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
  CatalogTree* self = 0;
  Category* category = 0;
  Role* role = 0;
  FILE* fd = stdout;
  char* path = 0;
  char url[512];

  if (!(self = coll->catalogTree)) goto error;

  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/indexHeader.shtml"))) goto error;
  logMain(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if (!htmlMainHead(fd, "Index")) goto error;
  if (!htmlLeftPageHead(fd, "index")) goto error;

  htmlPOpen(fd);
  getDocListUri(url, "<!--#echo var='HOME' -->/index", 1);
  htmlLink(fd, 0, url, _("All documents"));
  htmlPClose(fd);

  // categories
  if (!isEmptyRing(self->categories)) {
    htmlPOpen(fd);
    if (!fprintf(fd, _("\nClasses:\n"))) goto error;
    htmlUlOpen(fd);
    rgRewind(self->categories);
    while((category = rgNext(self->categories)) != 0) {
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
    htmlLink(fd, 0, url, _("Parteners:"));

    htmlUlOpen(fd);
    rgRewind(self->roles);
    while ((role = rgNext(self->roles)) != 0) {
      getRoleListUri(url, "<!--#echo var='HOME' -->/index", role->id, 1);
      htmlLiOpen(fd);
      htmlLink(fd, 0, url, role->label);
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // master url (other are relatives ones)
  if (!getServerUrl(coll->serverTree->master, "", url)) goto error;

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
    logMain(LOG_ERR, "serializeHtmlIndexHeader fails");
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
  CatalogTree* self = 0;
  Category *category = 0;
  Document *document = 0;
  Human *human = 0;
  Role *role = 0;
  AVLNode *node = 0;
  char tmp[128];
  char *path1 = 0;
  char *path2 = 0;
  int nbDoc = 0;
  int nbHum = 0;

  checkCollection(coll);
  if (!(self = coll->catalogTree)) goto error;
  logMain(LOG_DEBUG, "serializeHtmlIndex: %s collection", coll->label);

  nbDoc = avl_count(self->documents);
  nbHum = avl_count(self->humans);

  // directories
  if (!(path1 = createString(coll->htmlIndexDir))) goto error;
  if (!(path1 = catString(path1, "/roles"))) goto error;
  if (mkdir(path1, 0755)) {
    if (errno != EEXIST) {
      logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }
  path1 = destroyString(path1);
  if (!(path1 = createString(coll->htmlIndexDir))) goto error;
  if (!(path1 = catString(path1, "/categories"))) goto error;
  if (mkdir(path1, 0755)) {
    if (errno != EEXIST) {
      logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
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
      logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }

  if (nbDoc > 0) {
    logMain(LOG_DEBUG, "have %i documents", nbDoc);
    if (!htmlMakeDirs(path1, nbDoc)) goto error;
  }

  // humans directories
  path1 = destroyString(path1);
  if (!getHumanUri(tmp, "/", -1)) goto error;
  if (!(path1 = createString(coll->htmlIndexDir))) goto error;
  if (!(path1 = catString(path1, tmp))) goto error;
  if (mkdir(path1, 0755)) {
    if (errno != EEXIST) {
      logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }
  
  if (nbHum > 0) {
    logMain(LOG_DEBUG, "have %i humans", nbHum);
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
    while((role = rgNext(self->roles)) != 0) {
      if (!serializeHtmlRole(coll, role)) goto error;
    }
  }
  
  // categories
  if (!isEmptyRing(self->categories)) {
    if (!rgSort(self->categories, cmpCategory)) goto error;
    while((category = rgNext(self->categories)) != 0) {
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

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
