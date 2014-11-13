/*=======================================================================
 * Version: $Id: commonHtml.c,v 1.2 2014/11/13 16:36:17 nroche Exp $
 * Project: MediaTeX
 * Module : serverTree

* Server producer interface

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
#include "../common/openClose.h"
#include "commonHtml.h"

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
  char* string = NULL;
  char* number = NULL;
  int i = 0, j = 0;

  i = strlen(path);
  if (!(string = createSizedString(i + MAX_FILES_PER_DIR + 2, path))) 
    goto error;

  number = string + i;
  for (i=0, j=0; i<max; i+=MAX_FILES_PER_DIR, ++j) {
    if (!sprintf(number, "/%0*i", MAX_NUM_DIR_SIZE+1, j)) goto error;
    if (mkdir(string, 0755)) {
      if (errno != EEXIST) {
	logEmit(LOG_ERR, "mkdir fails: %s", strerror(errno));
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
 * Function   : getArchiveHeaderUri
 * Description: Get relative path to archive page
 * Synopsis   : int getArchiveHeaderUri(char* buf, char* path, int id) 
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
 * Synopsis   : int getContentDir(char* buf, char* path, int id) 
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

  logEmit(LOG_DEBUG, "serializeHtmlScoreListBar %i/%i", n, N);
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
    htmlLink(fd, NULL, url, "[");
  }
  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // - : previous page
  if (n == 1) {
    if (!fprintf(fd, "%s", "-")) goto error;
  } else {
    if (!getListUri(ptr, n-1)) goto error;;
    htmlLink(fd, NULL, url, "-");
  }
  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // | : middle page
  if (n == (N+1)>>1) {
    if (!fprintf(fd, "%s", "|")) goto error;
  } else {
    if (!getListUri(ptr, (N+1)>>1)) goto error;
    htmlLink(fd, NULL, url, "|");
  }
  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // + : next page
  if (n == N) {
    if (!fprintf(fd, "%s", "+")) goto error;
  } else {
    if (!getListUri(ptr, n+1)) goto error;
    htmlLink(fd, NULL, url, "+");
  }
  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // ] : last page
  if (n == N) {
    if (!fprintf(fd, "%s", "]")) goto error;
  } else {
    if (!getListUri(ptr, N)) goto error;;
    htmlLink(fd, NULL, url, "]");
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
    htmlLink(fd, NULL, url, "<");
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
    htmlLink(fd, NULL, url, "!");
  }

  if (!fprintf(fd, "%s", "<TD ALIGN='CENTER'>\n")) goto error;

  // > : right childreen in B-tree
  if (d == 1 || n == N) {
    if (!fprintf(fd, "%s", ">")) goto error;
  } else {
    // divide normal step by 2 until the node exist
    for (x=d; n + (1<<(x-2)) > N; --x);
    if (!getListUri(ptr, n + (1<<(x-2)))) goto error;
    htmlLink(fd, NULL, url, ">");
  }

  if (!fprintf(fd, "%s",
 	       "</TR></TABLE>\n"
	       "</TR></TABLE>\n")) goto error;

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlListBar fails");
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

  if(self == NULL) goto error;
  logEmit(LOG_DEBUG, "htmlAssoCarac: %s=%s", 
	  self->carac->label, self->value);

  htmlLiOpen(fd);
  if (!fprintf(fd, "%s = %s", self->carac->label, self->value))
    goto error;
  htmlLiClose(fd); 

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "htmlAssoCarac fails");
  }
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlCacheTop
 * Description: This template will be used by apache SSI
 * Synopsis   : int serializeHtmlCacheTop(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlCacheHeader(Collection* coll)
{ 
  int rc = TRUE;
  ServerTree* self = NULL;
  Server *server = NULL;
  RGIT* curr = NULL;
  char* path = NULL;
  char url[512];
  FILE* fd = stdout; 


  if (!(self = coll->serverTree)) goto error;
  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/cacheHeader.shtml"))) goto error;
  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if (!htmlMainHead(fd, _("Cache"))) goto error;
  if (!htmlLeftPageHead(fd, _("cache"))) goto error;

  if (!isEmptyRing(self->servers)) {
    if (!rgSort(self->servers, cmpServer)) goto error;
    htmlUlOpen(fd);

    rgRewind(self->servers);
    while ((server = rgNext_r(self->servers, &curr)) != NULL) {
      if (!sprintf(url, "https://%s/~%s/cache/", 
		   server->host, server->user)) goto error;

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
    logEmit(LOG_ERR, "%s", "serializeHtmlCacheHeader fails");
  }
  path = destroyString(path);
  return rc;
}


/*=======================================================================
 * Function   : serializeHtmlCgiTop
 * Description: This template will be used by apache SSI
 * Synopsis   : int serializeHtmlCgiTop(Collection* coll)
 * Input      : Collection* coll: input collection
 * Output     : TRUE on success
 =======================================================================*/
static int 
serializeHtmlCgiHeader(Collection* coll)
{ 
  int rc = TRUE;
  Configuration* conf = NULL;
  ServerTree* self = NULL;
  Server *server = NULL;
  RGIT* curr = NULL;
  char* path = NULL;
  char url1[512];
  char url2[512];
  FILE* fd = stdout; 

  if (!(conf = getConfiguration())) goto error;

  if (!(self = coll->serverTree)) goto error;
  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/cgiHeader.shtml"))) goto error;
  logEmit(LOG_DEBUG, "serialize %s", path);
  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if (!sprintf(url1, "https://%s/~%s", conf->host, coll->user))
    goto error;

  if (!htmlMainHeadBasic(fd, _("Cache"), url1)) goto error;
  if (!htmlLeftPageHeadBasic(fd, _("cache"), url1)) goto error;

  if (!isEmptyRing(self->servers)) {
    if (!rgSort(self->servers, cmpServer)) goto error;
    htmlUlOpen(fd);

    rgRewind(self->servers);
    while ((server = rgNext_r(self->servers, &curr)) != NULL) {
      if (!sprintf(url2, "https://%s/~%s/cache/", 
		   server->host, server->user)) goto error;

      htmlLiOpen(fd);
      htmlLink(fd, NULL, url2, server->host);
      htmlLiClose(fd);      
    }
    htmlUlClose(fd);
  }
  
  if (!sprintf(url2, "https://%s/~%s", coll->masterHost, coll->masterUser)) 
    goto error;

  if (!htmlLeftPageTail(fd)) goto error;
  if (!htmlRightHeadBasic(fd, url2, url1)) goto error;
  
  if (!env.dryRun) {
    fclose(fd);
  } else {
    fflush(stdout);
  }
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlCgiHeader fails");
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
  char* path = NULL;
  FILE* fd = stdout;
  time_t now = 0;
  struct tm date;
  char string[11];

  if (!(path = createString(coll->htmlDir))) goto error;
  if (!(path = catString(path, "/footer.html"))) goto error;
  logEmit(LOG_DEBUG, "serialize %s", path);

  if (!env.dryRun && (fd = fopen(path, "w")) == NULL) {
    logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno)); 
    goto error;
  }  

  if ((now = currentTime()) == -1) goto error;
  if (localtime_r(&now, &date) == (struct tm*)0) {
    logEmit(LOG_ERR, "%s", "localtime_r returns on error");
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
    logEmit(LOG_ERR, "%s", "serializeHtmlAllBottom fails");
  }
  path = destroyString(path);
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
  logEmit(LOG_DEBUG, "serializeHtmlCache: %s collection", coll->label);

  if (!serializeHtmlCacheHeader(coll)) goto error;
  if (!serializeHtmlCgiHeader(coll)) goto error;
  if (!serializeHtmlFooter(coll)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeHtmlCache fails");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
#include "../parser/confFile.tab.h"
#include "../parser/serverFile.tab.h"
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
 * Description: Unit test for serverTree module.
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
  if (!loadCollection(coll, SERV)) goto error;
  if (!serializeHtmlCache(coll)) goto error;
  if (!releaseCollection(coll, SERV)) goto error;
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
