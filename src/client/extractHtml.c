/*=======================================================================
 * Version: $Id: extractHtml.c,v 1.2 2014/11/13 16:36:18 nroche Exp $
 * Project: MediaTeX
 * Module : extraction tree latex serializer
 *
 * Extraction tree latex serializer interface

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
#include "../common/extractScore.h"
#include "commonHtml.h"
#include "extractHtml.h"

/*=======================================================================
 * Function   : getArchiveListUri
 * Description: Get relative path to archive list page
 * Synopsis   : int getArchiveListUri(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
static 
int getArchiveListUri(char* buf, char* path, int id) 
{
  int rc = FALSE;

  rc = getRelativeUri(buf, path, "lists");
  if (rc && id > 0) rc = getListUri(buf + strlen(buf), id);

  return rc;
}

/*=======================================================================
 * Function   : getBadListUri
 * Description: Get relative path to bad score archive's list page
 * Synopsis   : int getBadListUri(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
static 
int getBadListUri(char* buf, char* path, int id) 
{
  int rc = FALSE;

  rc = getRelativeUri(buf, path, "badLists");
  if (rc && id > 0) rc = getListUri(buf + strlen(buf), id);

  return rc;
}


/*=======================================================================
 * Function   : htmlFromAsso
 * Description: HTML for FromAsso.
 * Synopsis   : static int serializeHtmlFromAsso(FromAsso* self, FILE* fd)
 * Input      : FromAsso* self = what to latexalize
 *              FILE* fd = where to latexalize
 * Output     : TRUE on success
 =======================================================================*/
static int 
htmlFromAsso(FromAsso* self, FILE* fd, int isHeader)
{
  int rc = FALSE;
  Container* container = NULL;
  Archive* archive = NULL;
  int many = FALSE;
  char url[64];
  char label[16];
  char score[8];
  int i = 0;

  if(self == NULL) goto error;
  logEmit(LOG_DEBUG, "%s", "htmlFromAsso");

  container = self->container;
  many = (container->parents->nbItems > 1);
  htmlLiOpen(fd);

  htmlItalic(fd, self->path?self->path:"??");
  if (!fprintf(fd, _(" from %s "), strEType(container->type))) goto error;
  if (many) fprintf(fd, "(%.2f)\n", container->score);

  // do not sort !
  if (many) htmlUlOpen(fd);
  
  while((archive = rgNext(container->parents)) != NULL) {
    if (isHeader) {
      getArchiveUri(url, "../../../..", archive);
    } else {
      getArchiveUri(url, "../..", archive);
    }

    if (many) htmlLiOpen(fd);

    if (many) {
      if (!sprintf(label, _("part %i "), ++i)) goto error;
      htmlItalic(fd, label);
    }

    if (!sprintf(score, " (%5.2f)", archive->extractScore)) goto error;    
    htmlLink(fd, NULL, url, score);    
    if (many) htmlLiClose(fd);
  }

  if (many) htmlUlClose(fd);
  htmlLiClose(fd); 
  if (!fprintf(fd, "\n")) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "htmlFromAsso fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : htmlContainer
 * Description: HTML for Container.
 * Synopsis   : int htmlContainer(Container* self, FILE* fd)
 * Input      : Container* self = what to latexalize
 *              FILE* fd = where to latexalize
 * Output     : TRUE on success
 =======================================================================*/
int
htmlContainer(Container* self, FILE* fd)
{
  int rc = FALSE;
  FromAsso* asso = NULL;
  AVLNode *node = NULL;
  int many = FALSE;
  char url [128];
  char score[8];

  if(self == NULL) goto error;
  logEmit(LOG_DEBUG, "%s", "htmlContainer");

  //many = (self->childs->nbItems > 1);
  many = (avl_count(self->childs) > 1);
  htmlLiOpen(fd);

  fprintf(fd, "%s ", strEType(self->type));
  if (many) fprintf(fd, "(%5.2f)", self->score);

  // do not sort !
  if (many) htmlUlOpen(fd);

  for(node = self->childs->head; node; node = node->next) {
    asso = node->item;

    getArchiveUri(url, "../..", asso->archive);
    if (many) htmlLiOpen(fd);
    if (!sprintf(score, "(%5.2f)", asso->archive->extractScore)) goto error;
    htmlLink(fd, NULL, url, score);
    if (!fprintf(fd, "%s", " ")) goto error;
    htmlItalic(fd, asso->path);
    if (many) htmlLiClose(fd);
  }

  if (many) htmlUlClose(fd);
  htmlLiClose(fd);
  if (!fprintf(fd, "\n")) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "htmlContainer fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : serializeHtmlContentList
 * Description: Latexalize aa archive content list.
 * Synopsis   : int serializeHtmlContentList(Collection* coll,
 *                                                       int i, int n)
 * Input      : Collection* coll
 *              int i = current archive list
 *              int n = last archive list
 * Output     : TRUE on success
 =======================================================================*/
static int
serializeHtmlContentList(Collection* coll, AVLNode **node,
			 int archId, int i, int n)
{
  int rc = FALSE;
  FromAsso* asso = NULL;
  Archive* archive = NULL;
  FILE *fd = stdout;
  char *path = NULL;
  char url[128];
  char text[128];
  char score[8];
  int j = 0;

  getContentListUri(url, "/", archId, i);
  if (!(path = createString(coll->htmlScoreDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
    goto error;
  }

  fprintf(fd, "<!--#include virtual='../header.shtml' -->");
  htmlPOpen(fd);
  htmlUlOpen(fd);

  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;

  for (j = 0; j < MAX_INDEX_PER_PAGE && *node; ++j) {
    asso = (*node)->item;
    archive = asso->archive; // the content

    getArchiveUri(url, "../../../..", archive);
    if (!sprintf(text, "%s:%lli ", archive->hash,
		 (long long int)archive->size)) goto error;
    if (!sprintf(score, "(%5.2f)", archive->extractScore)) goto error;

    htmlLiOpen(fd);
    htmlVerb(fd, text);
    htmlLink(fd, NULL, url, score);
    htmlLiClose(fd);

    *node = (*node)->next;
  }

  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;

  htmlUlClose(fd);
  htmlPClose(fd);
  htmlSSIFooter(fd, "../../../../..");

  ++env.progBar.cur;
  rc = TRUE;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlContentList fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlScoreArchive
 * Description: Latexalize a Archive.
 * Synopsis   : int serializeHtmlArchive(Collection* coll, Archive* self)
 * Input      : Collection* coll
 *              Archive* self = what to latexalize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlScoreArchive(Collection* coll, Archive* self)
{
  int rc = FALSE;
  Configuration* conf = NULL;
  AssoCarac *assoCarac = NULL;
  Document* document = NULL;
  FromAsso* asso = NULL;
  Image* image = NULL;
  AVLNode *node = NULL;
  FILE *fd = stdout;
  char *path = NULL;
  char url[512];
  char text[128];
  int isHeader = FALSE;
  char mainHome[] = "../..";
  char headerHome[] = "../../../..";
  char* home = mainHome;
  int nb = 0, n = 0, i = 0;

  checkArchive(self);
  if (!(conf = getConfiguration())) goto error;

  // have to manage 2 cases :
  // - when use as main page
  // - when included from content list as header

  // if we need lists for the contents or not
  isHeader = (self->toContainer &&
  	      (nb = avl_count(self->toContainer->childs))
  	      > MAX_INDEX_PER_PAGE);

  if (isHeader) {
    home = headerHome;

    // make dir archive/0000/000
    getArchiveUri2(url, "/", self->id);
    if (!(path = createString(coll->htmlScoreDir))) goto error;
    if (!(path = catString(path, url))) goto error;
    if (mkdir(path, 0755)) {
      if (errno != EEXIST) {
    	logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
    	goto error;
      }
    }
  }
  
  // serialize the archive shtml file. May be
  // - archive/0000/000.shtml or
  // - archive/0000/000/header.shtml
  if (!isHeader) {
    getArchiveUri1(url, "/", self->id);
  } else {
    getArchiveUri2(url, "/", self->id);
    sprintf(url + strlen(url), "%s", "/header.shtml");
    path = destroyString(path);
  }
  if (!(path = createString(coll->htmlScoreDir))) goto error;
  if (!(path = catString(path, url))) goto error;
  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno));
    goto error;
  }
  
  if (isHeader) {
    htmlSSIHeader2(fd, "../../../..", "score");
  } else {
    htmlSSIHeader(fd, "../../..", "score");
  }

  if (!sprintf(url,
  	       "%s/../cgi/get.cgi?hash=%s&size=%lli", home,
  	       self->hash, (long long int)self->size)) goto error;

  if (!sprintf(text, "%s:%lli", self->hash, (long long int)self->size))
    goto error;

  htmlPOpen(fd);
  htmlBold(fd, _("Record:"));
  htmlVerb(fd, text);
  htmlBr(fd);
  if (!sprintf(text, "%s/../icons", home)) goto error;
  htmlImage(fd, text, "floppy-icon.png", url);
  if (!fprintf(fd, " (%.2f)", self->extractScore)) goto error;

  /* latexalize caracs */
  if (!isEmptyRing(self->assoCaracs)) {
    rgRewind(self->assoCaracs);
    htmlUlOpen(fd);

    while ((assoCarac = rgNext(self->assoCaracs)) != NULL) {
      if (!htmlAssoCarac(fd, assoCarac)) goto error;
    }

    htmlUlClose(fd);
  }
  htmlPClose(fd);

  // catalog
  htmlPOpen(fd);
  if (!fprintf(fd,
  	       _("\nCatalog reference as documents:\n")))
    goto error;
  htmlUlOpen(fd);
  if (!isEmptyRing(self->documents)) {
    if (!rgSort(self->documents, cmpDocument)) goto error;
    while((document = rgNext(self->documents)) != NULL) {
      if (!sprintf(text, "%s/../index", home)) goto error;
      getDocumentUri(url, text, document->id);

      htmlLiOpen(fd);
      htmlLink(fd, NULL, url, document->label);
      htmlLiClose(fd);
    }
  }
  else {
    htmlLiOpen(fd);
    if (!fprintf(fd, _("Not in catalog\n"))) goto error;
    htmlLiClose(fd);
  }
  htmlUlClose(fd);
  htmlPClose(fd);

  // servers
  htmlPOpen(fd);
  if (!fprintf(fd, _("\nProvided by servers:\n"))) goto error;
  htmlUlOpen(fd);
  if (!isEmptyRing(self->images)) {
    if (!rgSort(self->images, cmpImage)) goto error;
    while((image = rgNext(self->images)) != NULL) {
      if (!sprintf(text, "%s/servers/srv_%s.shtml", home,
  		   image->server->fingerPrint)) goto error;

      htmlLiOpen(fd);
      htmlLink(fd, NULL, text, image->server->host);
      if (!fprintf(fd, " (%.2f)", image->score)) goto error;
      htmlLiClose(fd);
    }
  }
  else {
    htmlLiOpen(fd);
    if (!fprintf(fd, _("Is not an image\n"))) goto error;
    htmlLiClose(fd);
  }
  htmlUlClose(fd);
  htmlPClose(fd);
 
  // from
  htmlPOpen(fd);
  if (!fprintf(fd, _("\nComes from:\n"))) goto error;
  htmlUlOpen(fd);
  if (!isEmptyRing(self->fromContainers)) {
    // already sorted
    rgRewind(self->fromContainers);
    while((asso = rgNext(self->fromContainers)) != NULL) {
      htmlFromAsso(asso, fd, isHeader);
    }
  }
  else {
    htmlLiOpen(fd);
    if (!fprintf(fd, _("External source"))) goto error;
    htmlLiClose(fd);
  }
  htmlUlClose(fd);
  htmlPClose(fd);

  // to
  htmlPOpen(fd);
  if (!fprintf(fd, _("\nContent: %i\n"), nb)) goto error;
  htmlUlOpen(fd);

  if (isHeader) {
    // - when included from content list as header
    path = destroyString(path);
    getArchiveUri2(url, "/", self->id);
    if (!(path = createString(coll->htmlScoreDir))) goto error;
    if (!(path = catString(path, url))) goto error;

    // make content list directories: archive/0000/000/0000
    n = (nb - 1) / MAX_INDEX_PER_PAGE +1;
    if (!htmlMakeDirs(path, n)) goto error;
    
    // serialize content lists
    node = self->toContainer->childs->head;
    for (i=1 ; i<=n; ++i) {
      if (!serializeHtmlContentList(coll, &node, self->id, i, n))
  	goto error;
    }
  }
  else {
    // - when use as main page
    if (self->toContainer) {
      htmlContainer(self->toContainer, fd);
    } else {
      htmlLiOpen(fd);
      if (!fprintf(fd, _("No content\n"))) goto error;
      htmlLiClose(fd);
    }
  }

  htmlUlClose(fd);
  htmlPClose(fd);

  if (!isHeader) {
    htmlSSIFooter(fd, "../../..");
  }
  
  ++env.progBar.cur;
  rc = TRUE;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlScoreArchive fails");
  }
  path = destroyString(path);
  return rc;
}

/*=======================================================================
 * Function   : serializeHtmlServer
 * Description: HTML page for Server
 * Synopsis   : int serializeHtmlServer(Collection* coll, Server* server)
 * Input      : Collection* coll = context
 *              Server* server = server to serialize
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeHtmlServer(Collection* coll, Server* server)
{ 
  int rc = FALSE;
  FILE* fd = stdout;
  char* path = NULL;
  char* string = NULL;
  Image* image = NULL;
  RGIT* curr = NULL;
  char url[128];
  char text[128];
  char score[8];

  checkServer(server);
  if (!(path = createString(coll->htmlScoreDir))) goto error;
  if (!(path = catString(path, "/servers/srv_"))) goto error;
  if (!(path = catString(path, server->fingerPrint))) goto error;
  if (!(path = catString(path, ".shtml"))) goto error;
  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if (!sprintf(text, _("Server%s "),  
	       !strncmp(server->host, coll->masterHost, MAX_SIZE_HOST)?
	       _(" master"):"")) goto error;

  htmlSSIHeader(fd, "../..", "score");

  htmlPOpen(fd);
  htmlBold(fd, text);
  if (!htmlCaps(fd, server->host)) goto error;
  if (!fprintf(fd, " (%.2f)\n", server->score)) goto error;
  htmlBr(fd);
  if (!isEmptyString(server->comment)) {
    htmlItalic(fd, server->comment);  
    htmlBr(fd);
  }
  htmlPClose(fd);

  htmlPOpen(fd);
  htmlUlOpen(fd);
  htmlLiOpen(fd);
  if (!fprintf(fd, _("fingerprint: "))) goto error;
  htmlVerb(fd, server->fingerPrint);
  htmlLiClose(fd);
  htmlLiOpen(fd);
  if (!fprintf(fd, _("mdtx port: "))) goto error;
  if (!sprintf(text, "%i", server->mdtxPort)) goto error;
  htmlVerb(fd, text);
  htmlLiClose(fd);
  htmlLiOpen(fd);
  if (!sprintf(text, "%i", server->sshPort)) goto error;
  if (!fprintf(fd, _("ssh port: "))) goto error;
  htmlVerb(fd, text);
  htmlLiClose(fd);
  htmlUlClose(fd);
  htmlPClose(fd);

  // networks
  if (!isEmptyRing(server->networks)) {
    if (!rgSort(server->networks, cmpString)) goto error;
    htmlPOpen(fd);
    if (!fprintf(fd, _("On networks:\n"))) goto error;
    htmlUlOpen(fd);
				    
    curr = NULL;
    while((string = rgNext_r(server->networks, &curr))) {    
      htmlLiOpen(fd);
      if (!fprintf(fd, "%s\n", string)) goto error;
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // gateways
  if (!isEmptyRing(server->gateways)) {
    if (!rgSort(server->gateways, cmpString)) goto error;
    htmlPOpen(fd);
    if (!fprintf(fd, _("Gateway for:\n"))) goto error;
    htmlUlOpen(fd);
				    
    curr = NULL;
    while((string = rgNext_r(server->gateways, &curr))) {    
      htmlLiOpen(fd);
      if (!fprintf(fd, "%s\n", string)) goto error;
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  // images
  if (!isEmptyRing(server->images)) {
    if (!rgSort(server->images, cmpImage)) goto error;
    htmlPOpen(fd);
    if (!fprintf(fd, _("Provide images: %i\n"), server->images->nbItems)) 
      goto error;
    htmlUlOpen(fd);
				    
    curr = NULL;
    while((image = rgNext_r(server->images, &curr))) {    
      getArchiveUri(url, "..", image->archive);
      if (!sprintf(text, "%s:%lli ", image->archive->hash, 
		   (long long int)image->archive->size)) goto error;
      if (!sprintf(score, "(%5.2f)", image->score)) goto error;

      htmlLiOpen(fd);
      htmlVerb(fd, text);
      htmlLink(fd, NULL, url, score);
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
    htmlPClose(fd);
  }

  htmlSSIFooter(fd, "../..");

  rc = TRUE;
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlServer fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlArchiveList
 * Description: Latexalize an Archive list.
 * Synopsis   : int serializeHtmlArchiveList(Collection* coll, 
 *                                                       int i, int n)
 * Input      : Collection* coll
 *              int i = current archive list
 *              int n = last archive list
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlArchiveList(Collection* coll, AVLNode **node, int i, int n)
{
  int rc = FALSE;
  Archive* archive = NULL;
  FILE *fd = stdout;
  char *path = NULL;
  char url[128];
  char text[128];
  char score[8];
  int j = 0;

  getArchiveListUri(url, "/", i);
  if (!(path = createString(coll->htmlScoreDir))) goto error;
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
    archive = (Archive*)(*node)->item;

    getArchiveUri(url, "../..", archive);
    if (!sprintf(text, "%s:%lli ", archive->hash, 
		 (long long int)archive->size)) goto error;
    if (!sprintf(score, "(%5.2f)", archive->extractScore)) goto error;

    htmlLiOpen(fd);
    htmlVerb(fd, text);
    htmlLink(fd, NULL, url, score);
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
    logEmit(LOG_ERR, "%s", "serializeHtmlArchiveList fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlArchiveLists
 * Description: HTML index page
 * Synopsis   : int 
 * Input      : ExtractTree* self = context
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlArchiveLists(Collection* coll)
{ 
  int rc = FALSE;
  AVLNode *node = NULL;
  FILE* fd = stdout;
  char tmp[128];
  char* path = NULL;
  char* path2 = NULL;
  int nb = 0;
  int i = 0;
  int n = 0;

  logEmit(LOG_DEBUG, "%s", "serializeHtmsScoreLists");

  nb = avl_count(coll->archives);
  if (!getArchiveListUri(tmp, "/", 0)) goto error;
  if (!(path = createString(coll->htmlScoreDir))) goto error;
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

  htmlSSIHeader2(fd, "../..", "score");
  htmlPOpen(fd);
  htmlBold(fd, _("All archives"));
  if (!fprintf(fd, " : %i", nb)) goto error;
  htmlBr(fd);
  htmlPClose(fd);

  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }

  // get the total number of lists of archives
  n = (nb - 1) / MAX_INDEX_PER_PAGE +1;
  logEmit(LOG_INFO, "have %i archive, so %i lists", nb, n);

  // build lists sub-directories (group by MAX_FILES_PER_DIR)
  if (!htmlMakeDirs(path, n)) goto error;
  
  // serialize lists
  node = coll->archives->head;
  for (i=1 ; i<=n; ++i) {
    if (!serializeHtmlArchiveList(coll, &node, i, n)) goto error;
  }

  // empty list if needed
  if (n == 0) {
    if (!htmlMakeDirs(path, 1)) goto error;
    if (!serializeHtmlArchiveList(coll, NULL, 1, 0)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlScoreScoreLists fails");
  }
  path = destroyString(path);
  path2 = destroyString(path2);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlBadList
 * Description: Latexalize an Document list.
 * Synopsis   : int serializeHtmlDocList(Collection* coll, int i, int n)
 * Input      : Collection* coll
 *              int i = current document list
 *              int n = last document list
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlBadList(Collection* coll, RG* ring, int i, int n)
{
  int rc = FALSE;
  Archive* archive = NULL;
  FILE *fd = stdout;
  char *path = NULL;
  char url[128];
  char text[128];
  char score[8];
  int j = 0;

  checkCollection(coll);

  getBadListUri(url, "/", i);
  if (!(path = createString(coll->htmlScoreDir))) goto error;
  if (!(path = catString(path, url))) goto error;

  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  fprintf(fd, "<!--#include virtual='../header.shtml' -->");
  if (!ring) goto end; // empty page if needed

  htmlPOpen(fd);
  if (!serializeHtmlListBar(coll, fd, i, n)) goto error;

  htmlUlOpen(fd);
  archive = rgCurrent(ring);
  for (j = 0; j < MAX_INDEX_PER_PAGE && archive; ++j) {

    getArchiveUri(url, "../..", archive);
    if (!sprintf(text, "%s:%lli ", archive->hash,
		 (long long int)archive->size)) goto error;
    if (!sprintf(score, "(%5.2f)", archive->extractScore)) goto error;

    htmlLiOpen(fd);
    htmlVerb(fd, text);
    htmlLink(fd, NULL, url, score);
    htmlLiClose(fd);

    archive = rgNext(ring);
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
    logEmit(LOG_ERR, "%s", "serializeHtmlBadList fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlBadLists
 * Description: HTML index page
 * Synopsis   : int 
 * Input      : ExtractTree* self = context
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlBadLists(Collection* coll)
{ 
  int rc = FALSE;
  RG* badArchives = NULL;
  FILE* fd = stdout;
  char tmp[128];
  char* text = NULL;
  char* path = NULL;
  char* path2 = NULL;
  off_t badSize = 0;
  int nb = 0;
  int i = 0;
  int n = 0;

  logEmit(LOG_DEBUG, "%s", "serializeHtmsBadLists");

  // put bad archives into a new ring  
  if (!(badArchives = createRing())) goto error;
  if (!(text = getExtractStatus(coll, &badSize, &badArchives))) goto error;
  nb = badArchives->nbItems;

  if (!getBadListUri(tmp, "/", 0)) goto error;
  if (!(path = createString(coll->htmlScoreDir))) goto error;
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
  
  htmlSSIHeader2(fd, "../..", "score");
  htmlPOpen(fd);
  htmlBold(fd, _("Bad archives"));
  if (!fprintf(fd, " : %i", nb)) goto error;
  htmlBr(fd);
  htmlItalic(fd, text);
  htmlBr(fd);
  htmlPClose(fd);
  
  if (badSize > 0) {
    if (badSize >= GIGA) {
      if (!fprintf(fd, "%lli Go\n", badSize / GIGA)) goto error;
      goto next;
    }
    if (badSize >= MEGA) {
      if (!fprintf(fd, "%lli Mo\n", badSize / MEGA)) goto error;
      goto next;
    }
    if (badSize >= KILO) {
      if (!fprintf(fd, "%lli Ko\n", badSize / KILO)) goto error;
      goto next;
    }
    if (!fprintf(fd, "%lli o\n", badSize)) goto error;
  next:
    if (!fprintf(fd, "%s", _("should be burned into supports:")))
      goto error;
  }

  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }

  // sort this new ring on scores
  rgSort(badArchives, cmpArchiveScore);

  // get the total number of lists of documents
  n = (nb - 1) / MAX_INDEX_PER_PAGE +1;
  logEmit(LOG_INFO, "have %i bad archives, so %i lists", nb, n);

  // build lists sub-directories (group by MAX_FILES_PER_DIR)
  if (!htmlMakeDirs(path, n)) goto error;
  
  // serialize lists
  rgHead(badArchives); // set current archive to the first one
  for (i=1 ; i<=n; ++i) {
    if (!serializeHtmlBadList(coll, badArchives, i, n)) goto error;
  }

  // empty list if needed
  if (n == 0) {
    if (!htmlMakeDirs(path, 1)) goto error;
    if (!serializeHtmlBadList(coll, NULL, 1, 0)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlBadLists fails");
  }
  path = destroyString(path);
  path2 = destroyString(path2);
  text = destroyString(text);
  badArchives = destroyOnlyRing(badArchives);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmsScoreIndex
 * Description: HTML index page
 * Synopsis   : int serializeHtmsScoreIndex(Collection* coll)
 * Input      : ExtractTree* self = what to latexalize
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmsScoreIndex(Collection* coll)
{ 
  int rc = FALSE;
  FILE* fd = stdout;
  char* path = NULL;
  ServerTree* serverTree = NULL;
  Archive* archive = NULL;
  char url[128];
  char text[128];
  char score[8];

  if (!(serverTree = coll->serverTree)) goto error;

  if (!(path = createString(coll->htmlScoreDir))) goto error;
  if (!(path = catString(path, "/index.shtml"))) goto error;
  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  htmlSSIHeader(fd, "..", _("score"));

  // score parameter of the collection
  htmlPOpen(fd);
  if (!fprintf(fd, _("\nScore parameters:\n"))) goto error;
  htmlUlOpen(fd);
  htmlLiOpen(fd);
  printLapsTime(fd, "%-10s:", "suppTTL",  serverTree->scoreParam.suppTTL);
  htmlLiClose(fd);
  htmlLiOpen(fd);
  fprintf(fd, "%-10s: %.2f\n", "maxScore", serverTree->scoreParam.maxScore);
  htmlLiClose(fd);
  htmlLiOpen(fd);
  fprintf(fd, "%-10s: %.2f\n", "badScore", serverTree->scoreParam.badScore);
  htmlLiClose(fd);
  htmlLiOpen(fd);
  fprintf(fd, "%-10s: %.2f\n", "powSupp",  serverTree->scoreParam.powSupp);
  htmlLiClose(fd);
  htmlLiOpen(fd);
  fprintf(fd, "%-10s: %.2f\n", "factSupp", serverTree->scoreParam.factSupp);
  htmlLiClose(fd);
  htmlLiOpen(fd);
  fprintf(fd,  "%-10s: %i", "minGeoDup", serverTree->minGeoDup);
  htmlLiClose(fd);
  htmlUlClose(fd);
  htmlPClose(fd);

  // images
  htmlPOpen(fd);
  if (!fprintf(fd, _("\nAll images: %i\n"), 
	       serverTree->archives->nbItems)) goto error;
  htmlUlOpen(fd);
  if (!isEmptyRing(serverTree->archives)) {
    if (!rgSort(serverTree->archives, cmpArchive)) goto error;
    while((archive = rgNext(serverTree->archives)) != NULL) {
      getArchiveUri(url, "", archive);
      if (!sprintf(text, "%s:%lli ", archive->hash, 
		   (long long int)archive->size)) goto error;
      if (!sprintf(score, "(%5.2f)", archive->extractScore)) goto error;

      htmlLiOpen(fd);
      htmlVerb(fd, text);
      htmlLink(fd, NULL, url, score);

      htmlLiClose(fd);
    }
  }
  else {
    htmlLiOpen(fd);
    if (!fprintf(fd, _("No image\n"))) goto error;
    htmlLiClose(fd);
  }
  htmlUlClose(fd);
  htmlPClose(fd);

  htmlSSIFooter(fd, "..");

  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmsScoreIndex fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlScoreHeader
 * Description: This template will be used by apache SSI
 * Synopsis   : static int serializeHtmlScoreHeader(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlScoreHeader(Collection* coll)
{ 
  int rc = FALSE;
  ServerTree* self = NULL;
  Server* server = NULL;
  FILE* fd = stdout;
  char* path = NULL;
  char url[512];

  if (!(coll->extractTree)) goto error;
  if (!(self = coll->serverTree)) goto error;

  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/scoreHeader.shtml"))) goto error;
  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if (!htmlMainHead(fd, _("Score"))) goto error;
  if (!htmlLeftPageHead(fd, _("score"))) goto error;

  // global score
  if (!fprintf(fd, "(%.2f)\n", coll->extractTree->score)) goto error;
  htmlBr(fd);

  // archive lists
  getArchiveListUri(url, "<!--#echo var='HOME' -->/score", 1);
  htmlLink(fd, NULL, url, _("All archives"));
  htmlBr(fd);

  // archive with bad score lists
  getBadListUri(url, "<!--#echo var='HOME' -->/score", 1);
  htmlLink(fd, NULL, url, _("Bad archives"));
  htmlBr(fd);

  // each server
  if (!isEmptyRing(self->servers)) {
    // sort already done
    htmlUlOpen(fd);

    rgRewind(self->servers);
    while ((server = rgNext(self->servers)) != NULL) {
      if (!sprintf(url, 
		   "<!--#echo var='HOME' -->/score/servers/srv_%s.shtml", 
		   server->fingerPrint)) goto error;
    
      htmlLiOpen(fd);
      htmlLink(fd, NULL, url, server->host);
      htmlLiClose(fd);
    }
    htmlUlClose(fd);
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
    logEmit(LOG_ERR, "%s", "serializeHtmlScoreHeader fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlScore 
 * Description: Generate HTML for score directory
 * Synopsis   : int serializeHtmlScore(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeHtmlScore(Collection* coll)
{ 
  int rc = FALSE;
  ServerTree* self = NULL;
  Server* server = NULL;
  Archive *archive = NULL;
  AVLNode *node = NULL;
  char tmp[128];
  char *path1 = NULL;
  char *path2 = NULL;
  int nb = 0;

  checkCollection(coll);
  if (!(self = coll->serverTree)) goto error;
  logEmit(LOG_DEBUG, "serializeHtmlScore: %s collection", coll->label);
 
  // servers directories
  if (!(path1 = createString(coll->htmlScoreDir))) goto error;
  if (!(path1 = catString(path1, "/servers"))) goto error;
  if (mkdir(path1, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
      goto error;
    }
  }

  // servers
  if (!isEmptyRing(self->servers)) {
    if (!rgSort(self->servers, cmpServer)) goto error;
    while((server = rgNext(self->servers)) != NULL) {
      if (!serializeHtmlServer(coll, server)) goto error;
    }
  }

  // archives directories
  path1 = destroyString(path1);
  if (!getArchiveUri1(tmp, "/", -1)) goto error;
  if (!(path1 = createString(coll->htmlScoreDir))) goto error;
  if (!(path1 = catString(path1, tmp))) goto error;
  if (mkdir(path1, 0755)) {
    if (errno != EEXIST) {
      logEmit(LOG_ERR, "mkdir %s fails: %s", path1, strerror(errno));
      goto error;
    }
  }

  if ((nb = avl_count(coll->archives))) {
    logEmit(LOG_INFO, "have %i archives", nb);
    if (!htmlMakeDirs(path1, nb)) goto error;
  }

  // archives
  if (nb) {
    for(node = coll->archives->head; node; node = node->next) {
      archive = (Archive*)node->item;
      if (!serializeHtmlScoreArchive(coll, archive)) goto error;
    }
  }

  // others
  if (!serializeHtmlArchiveLists(coll)) goto error;
  if (!serializeHtmlBadLists(coll)) goto error;
  if (!serializeHtmsScoreIndex(coll)) goto error;
  if (!serializeHtmlScoreHeader(coll)) goto error;
  
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlScore fails");
  }
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
  if (!computeExtractScore(coll)) goto error; 
  if (!loadCollection(coll, CTLG|SERV|EXTR)) goto error;
  if (!serializeHtmlScore(coll)) goto error;
  if (!releaseCollection(coll, CTLG|SERV|EXTR)) goto error;
  /************************************************************************/

  rc = TRUE;
 error:
  freeConfiguration();
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

