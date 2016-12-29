/*=======================================================================
 * Project: MediaTeX
 * Module : commonHtml

* HTML serializer common fonctions

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
 * Function   : getItemUri
 * Description: compute ending path for an item (not a list)
 * Synopsis   : int catItemUri(char* buf, int id)
 *              int id = number use to compute page location
 *              char* suffix = ".shtml" or "/header.shtml"
 * Output     : char* buf = where to write the Uri
 *              TRUE on success
 * Note       : item id begin at 0
 =======================================================================*/
int getItemUri(char* buf, int id, char* suffix) 
{
  int rc = FALSE;

  rc = (sprintf(buf, "/%0*i/%0*i%s",
		MAX_NUM_DIR_SIZE+1, id / MAX_FILES_PER_DIR,
		MAX_NUM_DIR_SIZE,   id % MAX_FILES_PER_DIR,
		suffix) > 0);
  return rc;
}

/*=======================================================================
 * Function   : getListUri
 * Description: compute ending path for a list (not an item)
 * Synopsis   : int catListUri(char* buf, int id)
 *              int id = number use to compute page location
 * Output     : char* buf = where to write the Uri
 *              TRUE on success
 * Note       : list id begin at 1
 =======================================================================*/
int getListUri(char* buf, int id) 
{
  int rc = FALSE;

  rc = (sprintf(buf, "/%0*i/%0*i.shtml",
		MAX_NUM_DIR_SIZE+1, (id-1) / MAX_FILES_PER_DIR,
		MAX_NUM_DIR_SIZE,   (id-1) % MAX_FILES_PER_DIR) > 0);
  return rc;
}

/*=======================================================================
 * Function   : htmlMakeDirs
 * Description: build sub directories
 * Synopsis   : int htmlMakeDirs(char* path, int max)
 * Input      : char* path : path to the directory
 *              int max : number of directories to create
 * Output     : TRUE on success
 =======================================================================*/
int htmlMakeDirs(char* path, int max)
{
  int rc = FALSE;
  char* string = 0;
  char* number = 0;
  int i = 0, j = 0;

  i = strlen(path);
  if (!(string = createSizedString(i + MAX_FILES_PER_DIR + 2, path))) 
    goto error;

  number = string + i;
  for (i=0, j=0; i<max; i+=MAX_FILES_PER_DIR, ++j) {
    if (!sprintf(number, "/%0*i", MAX_NUM_DIR_SIZE+1, j)) goto error;
    if (mkdir(string, 0755)) {
      if (errno != EEXIST) {
	logMain(LOG_ERR, "mkdir fails: %s", strerror(errno));
	goto error;
      }
    }
  }

  rc = TRUE;
 error:
  string = destroyString(string);
  return rc;
}

/*=======================================================================
 * Function   : getRelativeUri
 * Description: Get relative path to pages
 * Synopsis   : int getRelativeUri(char* buf, char* prefix, char* suffix) 
 *              char* suffix
 *              char* prefix
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
int getRelativeUri(char* buf, char* prefix, char* suffix) 
{
  int rc = FALSE;

  if (!prefix || prefix[0] == (char)0) {
    rc = (sprintf(buf, "%s", suffix) > 0);
  }
  else {
    if (prefix[0] == '/' && prefix[1] == (char)0) {
      rc = (sprintf(buf, "/%s", suffix) > 0);
    }
    else {
      rc = (sprintf(buf, "%s/%s", prefix, suffix) > 0);
    }
  }

  return rc;
}

/*=======================================================================
 * Function   : getArchiveUri1
 * Description: Get relative path to archive page
 * Synopsis   : int getArchiveUri1(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
int getArchiveUri1(char* buf, char* path, int id) 
{
  int rc = FALSE;

  rc = getRelativeUri(buf, path, "archives");
  if (rc && id >= 0) 
    rc = getItemUri(buf + strlen(buf), id, ".shtml");

  return rc;
}

/*=======================================================================
 * Function   : getArchiveUri2
 * Description: Get relative path to an archive directory
 * Synopsis   : int getArchiveUri2(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
int getArchiveUri2(char* buf, char* path, int id)
{
  int rc = FALSE;

  rc = getArchiveUri1(buf, path, -1);
  if (rc) rc = getItemUri(buf + strlen(buf), id, "");

  return rc;
}

/*=======================================================================
 * Function   : getArchiveListUri
 * Description: Get relative path to archive list page
 * Synopsis   : int getArchiveListUri(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
int 
getContentListUri(char* buf, char* path, int archId, int listId) 
{
  int rc = FALSE;

  rc = getArchiveUri2(buf, path, archId);
  if (rc) rc = getListUri(buf + strlen(buf), listId);

  return rc;
}

/*=======================================================================
 * Function   : getArchiveUri
 * Description: Get relative path to archive page
 * Synopsis   : int getArchiveUri(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
int getArchiveUri(char* buf, char* path, Archive* archive) 
{
  int rc = FALSE;

  if (archive->toContainer &&
      avl_count(archive->toContainer->childs) > MAX_INDEX_PER_PAGE) {
    rc = getContentListUri(buf, path, archive->id, 1);
  }
  else {
    rc = getArchiveUri1(buf, path, archive->id);
  }
 
  return rc;
}

/*=======================================================================
 * Function   : getDocumentUri
 * Description: Get relative path to document page
 * Synopsis   : int getDocumentUri(char* buf, char* path, int id) 
 *              char* path = path to use as prefix
 *              int id = number use to compute page location
 * Output     : char* buf = the Uri
 *              TRUE on success
 =======================================================================*/
int getDocumentUri(char* buf, char* path, int id) 
{
  int rc = FALSE;
  
  rc = getRelativeUri(buf, path, "documents");
  if (rc && id >= 0) rc = getItemUri(buf + strlen(buf), id, ".shtml");

  return rc;
}

/*=======================================================================
 * Function   : getArchiveScore
 * Description: print score into buffer and return it
 * Synopsis   : getArchiveScore(Archive* self) 
 *              Archive* self: related archive
 * Output     : char* buf = the static string buffer
 =======================================================================*/
char* getArchiveScore(Archive* self)
{
  static char score[8];

  if (self->incInherency) {
    sprintf(score, "(%s)", "---");
  }
  else {
    sprintf(score, "(%.2f)", self->extractScore);
  }

  return score;
}

/*=======================================================================
 * Function   : getContainerScore
 * Description: print score into buffer and return it
 * Synopsis   : getContainerScore(Container* self) 
 *              Container* self: related container
 * Output     : char* buf = the static string buffer
 =======================================================================*/
char* getContainerScore(Container* self)
{
  static char score[8];

  if (self->incInherency) {
    sprintf(score, "(%s)", "---");
  }
  else {
    sprintf(score, "(%.2f)", self->score);
  }

  return score;
}


/*=======================================================================
 * Function   : serializeHtmlListBar
 * Description: Latexalize the Archive list bar.
 * Synopsis   : int serializeHtmlBar(Collection* coll, 
 *                                   FILE* fd, int i, int n,
 *                                   int (*callback)(char*, char*, int))
 * Input      : Collection* coll
 *              FILE* fd
 *              int n = current archive list
 *              int N = last archive list
 *              callback = getUri function
 *              int z = bonus arg to pass to callback
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeHtmlListBar(Collection* coll, FILE* fd, int n, int N)
{
  int rc = FALSE;
  char url[128] = "..";
  char* ptr = url+2;
  int D = 0;
  int d = 0;
  int p = 0;
  int x = 0;
  int c = 0;

  logMain(LOG_DEBUG, "serializeHtmlScoreListBar %i/%i", n, N);
  if (n<1 || n>N) goto error;
  if (N == 1) goto end; // do not display the bar

  if (!fprintf(fd, "%s",
	       "<TABLE CELLPADDING=3>\n"
	       "<TR><TD ALIGN='CENTER'>\n")) goto error;

  if (!fprintf(fd, _("\nPage %i/%i"), n, N)) goto error;

  if (!fprintf(fd, "%s", 
	       "<TD ALIGN='CENTER'>\n"
	       "<TABLE CELLPADDING=1>\n"
	       "<TR><TD ALIGN='CENTER'>\n")) goto error;

  // [ : first page
  if (n == 1) {
    if (!fprintf(fd, "%s", "[")) goto error;
  } else {
    if (!getListUri(ptr, 1)) goto error;
    htmlLink(fd, 0, url, "[");
  }
  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // - : previous page
  if (n == 1) {
    if (!fprintf(fd, "%s", "-")) goto error;
  } else {
    if (!getListUri(ptr, n-1)) goto error;;
    htmlLink(fd, 0, url, "-");
  }
  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // | : middle page
  if (n == (N+1)>>1) {
    if (!fprintf(fd, "%s", "|")) goto error;
  } else {
    if (!getListUri(ptr, (N+1)>>1)) goto error;
    htmlLink(fd, 0, url, "|");
  }
  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // + : next page
  if (n == N) {
    if (!fprintf(fd, "%s", "+")) goto error;
  } else {
    if (!getListUri(ptr, n+1)) goto error;
    htmlLink(fd, 0, url, "+");
  }
  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // ] : last page
  if (n == N) {
    if (!fprintf(fd, "%s", "]")) goto error;
  } else {
    if (!getListUri(ptr, N)) goto error;;
    htmlLink(fd, 0, url, "]");
  }

  if (!fprintf(fd, "%s", 
	       "</TR></TABLE>\n"
	       "<TD ALIGN='CENTER'/>\n")) goto error;

  // comput D the maximum deep in B-tree (ie: position of hight bit to 1)
  for (D=1, x=2; x-1 < N; x<<=1, ++D);
  
  // compute d, the current deep in B-tree (ie: position of low bit to 1)
  for (d=1, x=n; !(x&1); x>>=1, ++d);
  
  // compute p, the position to father in B-tree (0: left, 1 right)
  p = (x>>1)&1;
  
  if (!fprintf(fd, _("\nDeep %i/%i"), d, D)) goto error;

  if (!fprintf(fd, "%s", 
	       "<TD ALIGN='CENTER'>\n"
	       "<TABLE CELLPADDING=1>\n"
	       "<TR><TD ALIGN='CENTER'>\n")) goto error;

  // < : left childreen in B-tree
  if (d == 1) {
    if (!fprintf(fd, "%s", "<")) goto error;
  } else {
    if (!getListUri(ptr, n - (1<<(d-2)))) goto error;
    htmlLink(fd, 0, url, "<");
  }

  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // ! : father in B-tree
  if (d == D) {
    if (!fprintf(fd, "%s", "!")) goto error;
  } else {
    p = -1 * ( 2*p -1 ); // +1: left, -1: right
    c = n + p * (1<<(d-1));

    // if the upper node does not exist, go to first left deeper node
    if (c > N) c = n - (1<<(d-1));

    if (!getListUri(ptr, c)) goto error;
    htmlLink(fd, 0, url, "!");
  }

  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // > : right childreen in B-tree
  if (d == 1 || n == N) {
    if (!fprintf(fd, "%s", ">")) goto error;
  } else {
    // divide normal step by 2 until the node exist
    for (x=d; n + (1<<(x-2)) > N; --x);
    if (!getListUri(ptr, n + (1<<(x-2)))) goto error;
    htmlLink(fd, 0, url, ">");
  }

  if (!fprintf(fd, "%s",
 	       "</TR></TABLE>\n"
	       "</TR></TABLE>\n")) goto error;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "serializeHtmlListBar fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : htmlAssoCarac
 * Description: HTML for AssoCarac.
 * Synopsis   : int htmlAssoCarac(FILE* fd, AssoCarac* self)
 * Input      : FILE* fd = where to latexalize
 *              AssoCarac* self = what to latexalize
 * Output     : TRUE on success
 =======================================================================*/
int 
htmlAssoCarac(FILE* fd, AssoCarac* self)
{
  int rc = FALSE;

  if(self == 0) goto error;
  logMain(LOG_DEBUG, "htmlAssoCarac: %s=%s", 
	  self->carac->label, self->value);

  htmlLiOpen(fd);
  if (!fprintf(fd, "%s = %s", self->carac->label, self->value))
    goto error;
  htmlLiClose(fd); 

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "htmlAssoCarac fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlCacheHeader
 * Description: This template will be used by apache SSI
 * Synopsis   : int serializeHtmlCacheHeader(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlCacheHeader(Collection* coll)
{ 
  int rc = TRUE;
  ServerTree* self = 0;
  Server *server = 0;
  Server* localhost = 0;
  RGIT* curr = 0;
  FILE* fd = stdout; 
  char* path = 0;
  char url[512];
  int itIs = FALSE;

  if (!(self = coll->serverTree)) goto error;
  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/cacheHeader.shtml"))) goto error;
  logMain(LOG_DEBUG, "serialize %s", path);
  if (!(localhost = getLocalHost(coll))) goto error;
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if (!htmlMainHead(fd, _("Cache"))) goto error;
  if (!htmlLeftPageHead(fd, "cache")) goto error;

  // server links
  if (!isEmptyRing(self->servers)) {
    if (!rgSort(self->servers, cmpServer)) goto error;
    htmlUlOpen(fd);

    rgRewind(self->servers);
    while ((server = rgNext_r(self->servers, &curr))) {
      if (!isReachable(coll, localhost, server, &itIs)) goto error;
      if (!itIs) continue;

      strcpy(url, server->url);
      strcpy(url + strlen(url), "/cache"); 
      htmlLiOpen(fd);
      htmlLink(fd, 0, url, server->host);
      htmlLiClose(fd);      
    }
    htmlUlClose(fd);
  }

  // upload link
  if (!allowedUser(env.confLabel, &itIs, 33)) goto error;
  if (itIs) {
    strcpy(url, localhost->url);
    strcpy(url + strlen(url), "/cgi/put.shtml");
    htmlLink(fd, 0, url, "Upload");
  }
  
  if (!htmlLeftPageTail(fd)) goto error;
  if (!htmlRightHead(fd)) goto error;
  
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "serializeHtmlCacheHeader fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlCgiHeader
 * Description: This template will be used by apache SSI
 * Synopsis   : int serializeHtmlCgiHeader(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlCgiHeader(Collection* coll)
{ 
  int rc = TRUE;
  Configuration* conf = 0;
  ServerTree* self = 0;
  Server* localhost = 0;
  Server *server = 0;
  RGIT* curr = 0;
  FILE* fd = stdout;
  char* path = 0;
  char url[512];
  int itIs = FALSE;

  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/cgiHeader.shtml"))) goto error;
  logMain(LOG_DEBUG, "serialize %s", path);
  if (!(conf = getConfiguration())) goto error;
  if (!(self = coll->serverTree)) goto error;
  if (!(localhost = getLocalHost(coll))) goto error;
  
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if (!htmlMainHeadBasic(fd, _("Cache"), localhost->url))
    goto error;
  if (!htmlLeftPageHeadBasic(fd, "cache", localhost->url))
    goto error;

  // upload link
  if (!allowedUser(env.confLabel, &itIs, 33)) goto error;
  if (itIs) {
    strcpy(url, localhost->url);
    strcpy(url + strlen(url), "/cgi/put.shtml");
    htmlLink(fd, 0, url, "Upload");
  }

  // server links
  if (!isEmptyRing(self->servers)) {
    if (!rgSort(self->servers, cmpServer)) goto error;
    htmlUlOpen(fd);

    rgRewind(self->servers);
    while ((server = rgNext_r(self->servers, &curr))) {
      if (!isReachable(coll, localhost, server, &itIs)) goto error;
      if (!itIs) continue;

      strcpy(url, server->url);
      strcpy(url + strlen(url), "/cache");
      htmlLiOpen(fd);
      htmlLink(fd, 0, url, server->host);
      htmlLiClose(fd);      
    }
    htmlUlClose(fd);
  }
  
  if (!htmlLeftPageTail(fd)) goto error;
  if (!htmlRightHeadBasic(fd, localhost->url)) goto error;
  
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "serializeHtmlCgiHeader fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlGitHeader
 * Description: This template will be used by cgit
 * Synopsis   : int serializeHtmlGitHeader(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlGitHeader(Collection* coll)
{ 
  int rc = TRUE;
  Configuration* conf = 0;
  ServerTree* self = 0;
  Server* localhost = 0;
  Server *server = 0;
  RGIT* curr = 0;
  char* path = 0;
  char url[512];
  FILE* fd = stdout; 
  int itIs = FALSE;

  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/gitHeader.html"))) goto error;
  logMain(LOG_DEBUG, "serialize %s", path);
  if (!(conf = getConfiguration())) goto error;
  if (!(self = coll->serverTree)) goto error;
  if (!(localhost = getLocalHost(coll))) goto error;
  
  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if (!htmlMainHeadBasic(fd, _("Version"), localhost->url))
    goto error;
  if (!htmlLeftPageHeadBasic(fd, "cgi/cgit.cgi/.git/", localhost->url))
    goto error;

  if (!isEmptyRing(self->servers)) {
    if (!rgSort(self->servers, cmpServer)) goto error;
    htmlUlOpen(fd);

    rgRewind(self->servers);
    while ((server = rgNext_r(self->servers, &curr))) {
      if (!isReachable(coll, localhost, server, &itIs)) goto error;
      if (!itIs) continue;

      strcpy(url, server->url);
      strcpy(url + strlen(url), "/cgi/cgit.cgi/.git/"); 
      htmlLiOpen(fd);
      htmlLink(fd, 0, url, server->host);
      htmlLiClose(fd);      
    }
    htmlUlClose(fd);
  }
  
  if (!htmlLeftPageTail(fd)) goto error;
  if (!htmlRightHeadBasic(fd, localhost->url)) goto error;
  
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "serializeHtmlGitHeader fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlAllBottom
 * Description: This template will be used by apache SSI
 * Synopsis   : int serializeHtmlAllBottom(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlFooter(Collection* coll)
{ 
  int rc = TRUE;
  char* path = 0;
  FILE* fd = stdout;
  time_t now = 0;
  struct tm date;
  char string[11];

  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/footer.html"))) goto error;
  logMain(LOG_DEBUG, "serialize %s", path);

  if (!env.dryRun && (fd = fopen(path, "w")) == 0) {
    logMain(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if ((now = currentTime()) == -1) goto error;
  if (localtime_r(&now, &date) == (struct tm*)0) {
    logMain(LOG_ERR, "localtime_r returns on error");
    goto error;
  }

  sprintf(string, "%s on %04i-%02i-%02i",
	  getConfiguration()->host,
	  date.tm_year + 1900, date.tm_mon+1, date.tm_mday);
	  
  if (!htmlMainTail(fd, string)) goto error;
  
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "serializeHtmlAllBottom fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlCacheHtaccess
 * Description: Set apache SSI variables
 * Synopsis   : int serializeHtmlCacheHtaccess(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlCacheHtaccess(Collection* coll)
{ 
  int rc = TRUE;
  ServerTree* self = 0;
  char* pathIn = 0;
  char* pathOut = 0;
  FILE* fdIn = 0;
  FILE* fdOut = stdout;
  char buffer[256];
  int len = 0;
 
  if (!(self = coll->serverTree)) goto error;
  if (!(pathIn = createString(coll->gitDir))) goto error;
  if (!(pathIn = catString(pathIn, "/apache2/cache.htaccess"))) goto error;
  if (!(pathOut = createString(coll->cacheDir))) goto error;
  if (!(pathOut = catString(pathOut, "/.htaccess"))) goto error;
  logMain(LOG_DEBUG, "serialize %s", pathOut);

  if (!(fdIn = fopen(pathIn, "r"))) {
    logMisc(LOG_ERR, "fopen %s fails: %s", pathIn, strerror(errno));
    goto error;
  }
  
  if (!env.dryRun && !(fdOut = fopen(pathOut, "w"))) {
    logMain(LOG_ERR, "fopen %s fails: %s", pathOut, strerror(errno)); 
    goto error;
  }  
  
  if (!fprintf(fdOut,
	       "# fancy index for cache\n"
	       "Options +Indexes\n"
	       "SetEnv HOME /~%s\n"
	       "HeaderName"
	       " /mediatex/%s/home/%s/public_html/cacheHeader.shtml\n"
	       "ReadmeName"
	       " /mediatex/%s/home/%s/public_html/footer.html\n\n"
	       "# bellow comes from ~%s/git/apache2/cache.htaccess\n\n",
	       coll->user,
	       env.confLabel, coll->user,
	       env.confLabel, coll->user,
	       coll->user))
      goto error;

  // concat user's cache.htaccess content
  while ((len = fread(buffer, 1, 256, fdIn)) > 0) {
    fwrite(buffer, 1, len, fdOut);
  }
  
  fclose(fdIn);
  if (!env.dryRun) {
    fclose(fdOut);
  } else {
    fflush(stdout);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "serializeHtmlCacheHtaccess fails");
  }
  pathOut = destroyString(pathOut);
  pathIn = destroyString(pathIn);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlCache
 * Description: Generate HTML for cache directory
 * Synopsis   : int serializeHtmlCache(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeHtmlCache(Collection* coll)
{ 
  int rc = FALSE;

  checkCollection(coll);
  logMain(LOG_DEBUG, "serializeHtmlCache: %s collection", coll->label);

  if (!serializeHtmlCacheHeader(coll)) goto error;
  if (!serializeHtmlCgiHeader(coll)) goto error;
  if (!serializeHtmlGitHeader(coll)) goto error;
  if (!serializeHtmlFooter(coll)) goto error;
  if (!serializeHtmlCacheHtaccess(coll)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "serializeHtmlCache fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
