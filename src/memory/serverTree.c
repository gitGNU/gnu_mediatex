/*=======================================================================
 * Version: $Id: serverTree.c,v 1.2 2014/11/13 16:36:30 nroche Exp $
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
#include "../misc/log.h"
#include "serverTree.h"


/*=======================================================================
 * Function   : createImage 
 * Description: Create, by memory allocation an image file (ISO file)
 * Synopsis   : Image* createImage(void)
 * Input      : N/A
 * Output     : The address of the create empty image file
 =======================================================================*/
Image* 
createImage(void)
{
  Image* rc = NULL;

  rc = (Image*)malloc(sizeof(Image));
  if(rc == NULL)
    goto error;
   
  memset(rc, 0, sizeof(Image));

  return(rc);
 error:
  logEmit(LOG_ERR, "%s", "malloc: cannot create an Image");
  return(rc);
}

/*=======================================================================
 * Function   : destroyImage 
 * Description: Destroy an image file by freeing all the allocate memory.
 * Synopsis   : void destroyImage(Image* self)
 * Input      : Image* self = the address of the image file to
 *              destroy.
 * Output     : Nil address of an image FILE
 =======================================================================*/
Image* 
destroyImage(Image* self)
{
  if(self == NULL) goto error;
  free(self);
 error:
  return (Image*)0;
}

/*=======================================================================
 * Function   : serializeImage 
 * Description: 2012/01/30
 *              Serialize a configuration.
 * Synopsis   :
 *              int serializeImage(Image* self, FILE* fd)
 * Input      :
 *              Image* self = what to serialize
 *              FILE* fd     = where to serialize
 * Output     :
 *               0 = success
 *              -1 = error
 =======================================================================*/
int 
serializeImage(Image* self, FILE* fd)
{
  int rc = FALSE;

  if(self == NULL) {
    logEmit(LOG_ERR, "%s", "empty image");
    goto error;
  }

  fprintf(fd, "%s:%llu=%.2f", 
	  self->archive->hash, 
	  (long long unsigned int)self->archive->size, 
	  self->score);

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "serializeImage fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : cmpImage
 * Description: Compare 2 image files
 * Synopsis   : int cmpImage(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2: the image to compare
 * Output     : like strcmp
 =======================================================================*/
int 
cmpImage(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Image* 
   */
  
  Image* a1 = *((Image**)p1);
  Image* a2 = *((Image**)p2);

  rc = cmpArchive(&a1->archive, &a2->archive);
  if (!rc) rc = cmpServer(&a1->server, &a2->server);
  return rc;
}

/*=======================================================================
 * Function   : cmpImageScore
 * Description: Compare 2 image files
 * Synopsis   : int cmpImageScore(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2: the image to compare
 * Output     : like strcmp
 * Note       : order by decreasing scores
 =======================================================================*/
int 
cmpImageScore(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Image* 
   */
  
  Image* a1 = *((Image**)p1);
  Image* a2 = *((Image**)p2);

  rc = cmpArchive(&a1->archive, &a2->archive);
  if (!rc && a1->score < a2->score) rc = +1;
  if (!rc && a1->score > a2->score) rc = -1;
  //if (!rc) rc = cmpServer(&a1->server, &a2->server);
  return rc;
}


/*=======================================================================
 * Function   : createServer 
 * Description: Create, by memory allocation a Server configuration 
 *              projection.
 * Synopsis   : Server* createServer(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
Server* 
createServer(void)
{
  Server* rc = NULL;

  rc = (Server*)malloc(sizeof(Server));
  if(rc == NULL) goto error;
   
  memset(rc, 0, sizeof(Server));

  // note: we do not set default ports (to allow to change them)
  rc->score = -1;
  rc->address.sin_addr.s_addr = LOCALHOST; // localhost by default
  rc->scoreParam = (ScoreParam)DEFAULT_SCORE_PARAM;
  rc->minGeoDup = DEFAULT_MIN_GEO;

  if ((rc->networks = createRing()) == NULL) goto error;
  if ((rc->gateways = createRing()) == NULL) goto error;
  if ((rc->images = createRing()) == NULL) goto error;
  if ((rc->records = createRing()) == NULL) goto error;

  return rc;
 error:
  logEmit(LOG_ERR, "%s", "malloc: cannot create Server");
  return destroyServer(rc);
}

/*=======================================================================
 * Function   : destroyServer 
 * Description: Destroy a configuration by freeing all the allocate memory
 * Synopsis   : void destroyServer(Server* self)
 * Input      : Server* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a configuration.
 =======================================================================*/
Server* 
destroyServer(Server* self)
{
  if(self == NULL) goto error;
  self->label = destroyString(self->label);
  self->user = destroyString(self->user);
  self->comment = destroyString(self->comment);
  self->mdtxPort = 0;
  self->sshPort = 0;
  self->userKey = destroyString(self->userKey);
  self->hostKey = destroyString(self->hostKey);
  
  // do not delete objets (owned by serverTree or recordTree)
  self->records = destroyOnlyRing(self->records);
  self->networks = destroyOnlyRing(self->networks);
  self->gateways = destroyOnlyRing(self->gateways);

  self->images = 
    destroyRing(self->images, (void*(*)(void*)) destroyImage);

  free(self);
 error:
  return (Server*)0;
}

/*=======================================================================
 * Function   : serializeServer 
 * Description: Serialize a configuration.
 * Synopsis   : int serializeServer(Server* self, FILE* fd)
 * Input      : Server* self = what to serialize
 *              FILE* fd     = where to serialize
 * Output     :  0 = success
 *              -1 = error
 =======================================================================*/
int 
serializeServer(Server* self, FILE* fd)
{
  int rc = FALSE;
  Image* image = NULL;
  char* string = NULL;
  RGIT* curr = NULL;

  if(self == NULL) {
    logEmit(LOG_ERR, "%s", "cannot serialize empty Server");
    goto error;
  }
  
  fprintf(fd, "\nServer %s\n", self->fingerPrint);
  if (!isEmptyString(self->comment)) {
    fprintf(fd, "\t%-9s \"%s\"\n", "comment", self->comment);
  }
  if (!isEmptyString(self->label)) {
    fprintf(fd, "\t%-9s %s\n", "label", self->label);
  }
  if (!isEmptyString(self->host)) {
    fprintf(fd, "\t%-9s %s\n", "host", self->host);
  }
  if (self->mdtxPort) {
    fprintf(fd, "\t%-9s %i\n", "mdtxPort", self->mdtxPort);
  }
  if (self->sshPort) {
    fprintf(fd, "\t%-9s %i\n", "sshPort", self->sshPort);
  }

  if (!isEmptyRing(self->networks)) {
    fprintf(fd, "\t%-9s ", "networks");
    if (!rgSort(self->networks, cmpString)) goto error;
    curr = NULL;
    if (!(string = rgNext_r(self->networks, &curr))) goto error;
    fprintf(fd, "%s", string);
    while ((string = rgNext_r(self->networks, &curr))) {
      fprintf(fd, ", %s", string);
    }
    fprintf(fd, "\n");
  }
  
  if (!isEmptyRing(self->gateways)) {
    fprintf(fd, "\t%-9s ", "gateways");
    if (!rgSort(self->gateways, cmpString)) goto error;
    curr = NULL;
    if (!(string = rgNext_r(self->gateways, &curr))) goto error;
    fprintf(fd, "%s", string);
    while ((string = rgNext_r(self->gateways, &curr))) {
      fprintf(fd, ", %s", string);
    }
    fprintf(fd, "\n");
  }

  /* cache parameters (take care, this use macro) */
  if (self->cacheSize != 0) {
    printCacheSize(fd, "\t%-9s", "cacheSize", self->cacheSize);
  }
  if (self->cacheTTL != 0) {
    printLapsTime(fd, "\t%-9s", "cacheTTL", self->cacheTTL);
  }
  if (self->queryTTL != 0) {
    printLapsTime(fd, "\t%-9s", "queryTTL", self->queryTTL);
  }

  // list of images
  if (!isEmptyRing(self->images)) {
    if (!rgSort(self->images, cmpImage)) goto error;
    curr = NULL;
    if (!(image = rgNext_r(self->images, &curr))) goto error;
    fprintf(fd, "%s", "\tprovide\t  ");
    do {
      if (!serializeImage(image, fd)) goto error;
      image = rgNext_r(self->images, &curr);
      if (image != NULL) fprintf(fd, ",\n\t\t  ");
    }
    while(image != NULL);
    fprintf(fd, "\n");
  } 

  fprintf(fd, "%s", "#\tkeys:\n");
  fprintf(fd, "\t%-9s \"%s\"\n", "userKey", self->userKey);
  if (!isEmptyString(self->hostKey))
    fprintf(fd, "\t%-9s \"%s\"\n", "hostKey", self->hostKey);

  fprintf (fd, "end\n");
  rc = TRUE;
 error:
  return(rc);
}

/*=======================================================================
 * Function   : cmpServer
 * Description: compare 2 servers
 * Synopsis   : int cmpServer(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2: the servers to compare
 * Output     : like strcmp
 =======================================================================*/
int 
cmpServer(const void *p1, const void *p2)
{
  int rc = 0;

  /* p1 and p2 are pointers on &items
   * and items are suposed to be Image* 
   */
  
  Server* a1 = *((Server**)p1);
  Server* a2 = *((Server**)p2);

  rc = strcmp(a1->fingerPrint, a2->fingerPrint);
  return rc;
}


/*=======================================================================
 * Function   : createServerTree 
 * Description: Create, by memory allocation a ServerTree
 *              configuration projection.
 * Synopsis   : ServerTree* createServerTree(void)
 * Input      : N/A
 * Output     : The address of the create empty configuration.
 =======================================================================*/
ServerTree* 
createServerTree(void)
{
  ServerTree* rc = NULL;
  ScoreParam defaultScoreParam = DEFAULT_SCORE_PARAM;

  rc = (ServerTree*)malloc(sizeof(ServerTree));
  if(rc == NULL)
    goto error;
    
  memset(rc, 0, sizeof(ServerTree));
  strncpy(rc->aesKey, "01234567890abcdef", MAX_SIZE_AES);

  rc->scoreParam = defaultScoreParam;
  rc->minGeoDup = DEFAULT_MIN_GEO;

  if ((rc->servers = createRing()) == NULL) goto error;
  if ((rc->archives = createRing()) == NULL) goto error;
  
  return rc;
 error:
  logEmit(LOG_ERR, "%s", "malloc: cannot create a serverTree");
  return destroyServerTree(rc);
}

/*=======================================================================
 * Function   : destroyServerTree 
 * Description: Destroy a configuration by freeing all the allocate memory.
 * Synopsis   : void destroyServerTree(ServerTree* self)
 * Input      : ServerTree* self = the address of the configuration to
 *              destroy.
 * Output     : Nil address of a configuration.
 =======================================================================*/
ServerTree* 
destroyServerTree(ServerTree* self)
{
  if(self == NULL) goto error;
  // do not delete archives objets 
  self->archives = destroyOnlyRing(self->archives);
  self->servers = destroyRing(self->servers, 
				  (void*(*)(void*)) destroyServer);
  free(self);
 error:
  return (ServerTree*)0;
}

/*=======================================================================
 * Function   : serializeServerTree
 * Description: Serialize a server tree
 * Synopsis   : int serializeServerTree(Collection* coll)
 * Input      : Collection coll* = what to serialize
 * Output     : TRUE on success
 =======================================================================*/
int 
serializeServerTree(Collection* coll)
{ 
  int rc = FALSE;
  char* path = NULL;
  FILE* fd = stdout; 
  ServerTree *self = NULL;
  Server *server = NULL;
  RGIT* curr = NULL;
  int uid = getuid();

  checkCollection(coll);
  if(!(self = coll->serverTree)) goto error;
  logEmit(LOG_DEBUG, "serialize %s server tree", coll->label);

  // we neeed to use the home cvs collection directory
  if (!(coll->memoryState & EXPANDED)) {
    logEmit(LOG_ERR, "%s", "collection must be expanded first");
    goto error;
  }

  if (!becomeUser(coll->user, TRUE)) goto error;

  // output file
  if (env.dryRun == FALSE) path = coll->serversDB;
  logEmit(LOG_INFO, "Serializing the server tree file: %s", 
	  path?path:"stdout");
  if (path != NULL && *path != (char)0) {
    if ((fd = fopen(path, "w")) == NULL) {
      logEmit(LOG_ERR, "fdopen %s fails: %s", path, strerror(errno));
      fd = stdout;
      goto error;
    }
    if (!lock(fileno(fd), F_WRLCK)) goto error;
  }

  fprintf(fd, "# This file is managed by MediaTeX software.\n");

  // do not add the CVS version as it implies a commit evry time we
  // rebuild the file (because we de not parse the version id)
  //fprintf(fd, "# Version: $" "Id" "$\n");
  
  if (!self->master) {
    logEmit(LOG_ERR, "loose master server for %s collection", 
	    coll->label);
    goto error;
  }

  // tel which is the master collection server
  fprintf(fd, "# MediaTeX %s@%s:%i servers list:\n\n", 
	  self->master->user, self->master->host, 
	  self->master->sshPort);
  fprintf(fd, "%-10s %s\n", "master", self->master->fingerPrint);
  
  fprintf(fd, "%-10s %s\n", "collKey", self->aesKey);

  fprintf(fd, "\n# score parameters\n");
  printLapsTime(fd, "%-10s", "suppTTL",  self->scoreParam.suppTTL);
  fprintf(fd, "%-10s %.2f\n", "maxScore", self->scoreParam.maxScore);
  fprintf(fd, "%-10s %.2f\n", "badScore", self->scoreParam.badScore);
  fprintf(fd, "%-10s %.2f\n", "powSupp",  self->scoreParam.powSupp);
  fprintf(fd, "%-10s %.2f\n", "factSupp", self->scoreParam.factSupp);
  fprintf(fd,  "%-10s %i", "minGeoDup", self->minGeoDup);
  fprintf(fd, "\n");

  if (!isEmptyRing(self->servers)) {
    if (!rgSort(self->servers, cmpServer)) goto error;
    while((server = rgNext_r(self->servers, &curr)) != NULL) {
      if (!serializeServer(server, fd)) goto error;
    }
  }

  // emacs mode
  fprintf(fd, "\n# Local Variables:\n"
	  "# mode: conf\n"
	  "# mode: font-lock\n"
	  "# End:\n");

  fflush(fd);
  rc = TRUE;
 error:
  if (fd != stdout) {
    if (!unLock(fileno(fd))) rc = FALSE;
    if (fclose(fd)) {
      logEmit(LOG_ERR, "fclose fails: %s", strerror(errno));
      rc = FALSE;
    }
  }
  if (!logoutUser(uid)) rc = FALSE;
  return rc;
}


/*=======================================================================
 * Function   : getImage
 * Description: Find an image
 * Synopsis   : Image* getImage(Collection* coll, 
 *                              Server* server, Archive* archive)
 * Input      : Collection* coll : where to find
 *              Server* server : id
 *              Archive* archive : id
 * Output     : Image* : the Image we have found
 =======================================================================*/
Image* 
getImage(Collection* coll, Server* server, Archive* archive)
{
  Image* rc = NULL;
  RGIT* curr = NULL;

  checkCollection(coll);
  checkServer(server);
  logEmit(LOG_DEBUG, "getImage %s, %s:%i, %s:%lli",
	  coll, server->host, server->mdtxPort, archive->hash,
	  (long long int)archive->size);
  
  // look for image
  while((rc = rgNext_r(server->images, &curr)) != NULL)
    if (!cmpArchive(&rc->archive, &archive)) break;

 error:
  return rc;
}

/*=======================================================================
 * Function   : addImage
 * Description: Add an image if not already there
 * Synopsis   : Image* getImage(Collection* coll, 
 *                              Server* server, Archive* archive)
 * Input      : Collection* coll : where to add
 *              Server* server : id
 *              Archive* archive : id
 * Output     : Image* : the Image we have found/added
 =======================================================================*/
Image* 
addImage(Collection* coll, Server* server, Archive* archive)
{
  Image* rc = NULL;
  Image* image = NULL;

  checkCollection(coll);
  checkServer(server);
  logEmit(LOG_DEBUG, "addImage %s, %s:%i, %s:%lli",
	  coll, server->host, server->mdtxPort, archive->hash,
	  (long long int)archive->size);

  // already there
  if ((image = getImage(coll, server, archive))) goto end;

  // add new one if not already there
  if ((image = createImage()) == NULL) goto error;
  image->archive = archive;
  image->server = server;

  // add image to server
  if (!rgInsert(server->images, image)) goto error;
    
  // add image to archive
  if (!rgInsert(archive->images, image)) goto error;

  // add archive to serverTree, if not already there
  if (!rgHaveItem(coll->serverTree->archives, archive)) {
    if (!rgInsert(coll->serverTree->archives, archive)) goto error;
  }

 end:
  rc = image;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to add an image");
    if (image) delImage(coll, image);
  }
  return rc;
}

/*=======================================================================
 * Function   : delImage
 * Description: Del an image if not already there
 * Synopsis   : void delImage(Collection* coll, Image* image)
 * Input      : Collection* coll : where to del
 *              Image* image : the image to del
 * Output     : TRUE on success
 * Note       :
 *     Non sens: seul delArchive et delServer peuvent l'appeler
 *     or delServer => O(n2) pour rien.
 =======================================================================*/
int
delImage(Collection* coll, Image* image)
{
  int rc = FALSE;
  RGIT* curr = NULL;
 
  checkCollection(coll);
  checkImage(image);
  logEmit(LOG_DEBUG, "delImage %s, %s:%i, %s:%lli",
	  coll, image->server->host, image->server->mdtxPort, 
	  image->archive->hash, (long long int)image->archive->size);
  logEmit(LOG_DEBUG, "%s", "delImage");

  // delete image from server
  if ((curr = rgHaveItem(image->server->images, image))) {
    rgRemove_r(image->server->images, &curr);
  }

  // delete image from archive
  if ((curr = rgHaveItem(image->archive->images, image->archive))) {
    rgRemove_r(image->archive->images, &curr);
  }

  // delete archive from serverTree (if last related image)
  if (isEmptyRing(image->archive->images)) {
    if ((curr = rgHaveItem(coll->serverTree->archives, image->archive))) {
      rgRemove_r(coll->serverTree->archives, &curr);
    }
  }

  // free the image
  image = destroyImage(image);
  rc = TRUE;
 error:
 if (!rc) {
    logEmit(LOG_ERR, "%s", "delImage fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : isReachable
 * Description: test if "from" server may use gateways to reach "to" 
 *              server
 * Synopsis   : int isReachable(Collection* coll, 
 *                                         Server* from, Server* to)
 * Input      : Collection* coll : where to add
 *              Server* server = the proxy server 
 *              Server* proxy  = the proxy client
 * Output     : TRUE on success
 * Note       : this function is not symmetric
 =======================================================================*/
int 
isReachable(Collection* coll, Server* from, Server* to)
{
  int rc = FALSE;
  int loop = TRUE;
  RG* reach = NULL;
  void* it = NULL;
  RG* tmp = NULL;
  //RG* tmp2 = NULL;
  RGIT* curr = NULL;
  Server* server = NULL;
  int nbReachable = 0;

  logEmit(LOG_DEBUG, "isReachable from %s to %s",
	  from->fingerPrint, to->fingerPrint);

  // networks we can reach
  if (!(reach = createRing())) goto error;
  while((it = rgNext_r(from->networks, &curr))) {
    if (!rgInsert(reach, it)) goto error;
  }
  nbReachable = reach->nbItems;

  // loop until "to" is located under a reachable network
  /* printf("\n"); */
  while (loop) {

    /* printf("looping...\n"); */
    /* printf("we now we can reach:"); */
    /* while((it = rgNext(reach))) printf(" %s", (char*)it); */
    /* printf ("\n"); */

    /* printf("we want to reach on of them:"); */
    /* while((it = rgNext(to->networks))) printf(" %s", (char*)it); */
    /* printf ("\n"); */

    // exit when reachable
    if (!(tmp = rgInter(reach, to->networks))) goto error;
    if (!isEmptyRing(tmp)) {
      rc = TRUE;
      tmp = destroyOnlyRing(tmp);
      /* printf("... ok reachable\n"); */
      goto end;
    }
    tmp = destroyOnlyRing(tmp);

    // look for a new gateway (juste one)
    loop = FALSE;
    curr = NULL; 
    while(!loop && (server = rgNext_r(coll->serverTree->servers, &curr))) {
      if (!(tmp = rgInter(reach, server->gateways))) goto error;
      if (!isEmptyRing(tmp)) {

	/* printf("new reachable networks:"); */
	/* while((it = rgNext(server->networks))) printf(" %s", (char*)it); */
	/* printf ("\n");      */

	// we find one so we now we can reach its networks
	tmp = destroyOnlyRing(tmp);
	if (!(tmp = rgUnion(reach, server->networks))) goto error;
	reach = destroyOnlyRing(reach);
	reach = tmp;
	tmp = NULL;

	// but is there new networks ?
	loop = (reach->nbItems > nbReachable);
      }
      tmp = destroyOnlyRing(tmp);
    }
  }

  /* printf("... not reachable\n"); */
 end:
  logEmit(LOG_INFO, "%s may %sreach %s",
	  from->fingerPrint, rc?"":"not ", to->fingerPrint);
  reach = destroyOnlyRing(reach);
  return rc;
 error:
  logEmit(LOG_DEBUG, "%s", "isReachable fails");
  reach = destroyOnlyRing(reach);
  return FALSE;
}


/*=======================================================================
 * Function   : getServer
 * Description: Find an server
 * Synopsis   : Server* getServer(Collection* coll, 
                                    char* hash, off_t size)
 * Input      : Collection* coll : where to find
 *              char* fingerPrint : the user key fingerprint 
 * Output     : Server* : the Server we have found
 =======================================================================*/
Server* 
getServer(Collection* coll, char* fingerPrint)
{
  Server* rc = NULL;
  RGIT* curr = NULL;

  checkCollection(coll);
  checkLabel(fingerPrint);
  logEmit(LOG_DEBUG, "getServer %s", fingerPrint);
  
  // look for server
  while((rc = rgNext_r(coll->serverTree->servers, &curr)) != NULL)
    if (!strncmp(rc->fingerPrint, fingerPrint, MAX_SIZE_HASH)) break;

 error:
  return rc;
}

/*=======================================================================
 * Function   : addServer
 * Description: Add an server if not already there
 * Synopsis   : Server* getServer(Collection* coll, 
 *                                  char* hash, off_t size)
 * Input      : Collection* coll : where to add
 *              char* fingerPrint : the user key fingerprint 
 * Output     : Server* : the Server we have found/added
 =======================================================================*/
Server* 
addServer(Collection* coll, char* fingerPrint)
{
  Server* rc = NULL;
  Server* server = NULL;

  checkCollection(coll);
  checkLabel(fingerPrint);
  logEmit(LOG_DEBUG, "addServer %s", fingerPrint);
  
  // already there
  if ((server = getServer(coll, fingerPrint))) goto end;

  // add new one if not already there
  if (!(server = createServer())) goto error;
  strncpy(server->fingerPrint, fingerPrint, MAX_SIZE_HASH);
  server->fingerPrint[MAX_SIZE_HASH] = (char)0; // developpement code
  if (!rgInsert(coll->serverTree->servers, server)) goto error;

  // shorter than a function even if not used a lot
  server->isLocalhost = 
    !strncmp(fingerPrint, coll->userFingerPrint, MAX_SIZE_HASH);

 end:
  rc = server;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "addServer fails");
    server = destroyServer(server);
  }
  return rc;
}

/*=======================================================================
 * Function   : delServer
 * Description: Del an server if not already there
 * Synopsis   : void delServer(Collection* coll, Server* server)
 * Input      : Collection* coll : where to del
 *              Server* server : the server to del
 * Output     : TRUE on success
 =======================================================================*/
int
delServer(Collection* coll, Server* server)
{
  int rc = FALSE;
  Image* img = NULL;
  Record* rec = NULL;
  RGIT* curr = NULL;

  checkCollection(coll);
  checkServer(server);
  logEmit(LOG_DEBUG, "delServer %s", server->fingerPrint);

  // delete from coll if localhost 
  if (coll->localhost == server) coll->localhost = NULL;

  // delete images own by the this server
  while ((img = rgHead(server->images)) != NULL)
    if (!delImage(coll, img)) goto error;

  // delete records
  while((rec = rgHead(server->records)) != NULL)
    if (!delRecord(coll, rec)) goto error;

  // delete server from collection ring
  if ((curr = rgHaveItem(coll->serverTree->servers, server))) {
    rgRemove_r(coll->serverTree->servers, &curr);
  }

  // free the server
  server = destroyServer(server);
  rc = TRUE;
 error:
  return rc;
}

/*=======================================================================
 * Function   : diseaseServer
 * Description: Disease an server if not already there
 * Synopsis   : void diseaseServer(Collection* coll, Server* server)
 * Input      : Collection* coll : where to disease
 *              Server* server : the server to disease
 * Output     : TRUE on success
 =======================================================================*/
int
diseaseServer(Collection* coll, Server* server)
{
  int rc = FALSE;
  Image* image = NULL;
  RGIT* curr = NULL;

  checkCollection(coll);
  checkServer(server);
  logEmit(LOG_DEBUG, "disease %s server", server->fingerPrint);

  // delete images own by the this server
  while ((image = rgHead(server->images)) != NULL) {

    // delete archive from serverTree
    if ((curr = rgHaveItem(coll->serverTree->archives, image->archive))) {
      rgRemove_r(coll->serverTree->archives, &curr);
    }

    // delete image from archive
    if ((curr = rgHaveItem(image->archive->images, image))) {
      rgRemove_r(image->archive->images, &curr);
    }

    // try to disease archive
    if (!diseaseArchive(coll, image->archive)) goto error;    

    // delete image from server
    curr = server->images->head;
    rgRemove_r(server->images, &curr);
    
    // free the image
    destroyImage(image);
  }
  
  // delete networks and gateways
  rgDelete(server->networks);
  rgDelete(server->gateways);

  // disease fields
  server->label = destroyString(server->label);
  server->user = destroyString(server->user);
  server->comment = destroyString(server->comment);
  server->mdtxPort = 0;
  server->sshPort = 0;
  server->userKey = destroyString(server->userKey);
  server->hostKey = destroyString(server->hostKey);
  server->host[0] = (char)0;
  server->cacheSize = 0;
  server->cacheTTL = 0;
  server->queryTTL = 0;
  
  rc = TRUE;
 error:
  if (!rc) logEmit(LOG_ERR, "%s", "diseaseServer fails");
  return rc;
}

/*=======================================================================
 * Function   : diseaseServerTree
 * Description: Disease an serverTree if not already there
 * Synopsis   : void diseaseServerTree(Collection* coll)
 * Input      : Collection* coll : where to disease
 * Output     : TRUE on success
 =======================================================================*/
int
diseaseServerTree(Collection* coll)
{
  int rc = FALSE;
  ServerTree *self = NULL;
  Server *server = NULL;
  RGIT* curr = NULL;
 
  checkCollection(coll);
  if((self = coll->serverTree) == NULL) goto error;
  logEmit(LOG_DEBUG, "disease %s servers", coll->label);

  // disease servers
  curr = NULL;
  while((server = rgNext_r(self->servers, &curr)) != NULL) {
    if (!diseaseServer(coll, server)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) logEmit(LOG_ERR, "%s", "diseaseServerTree fails");
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
 * Description: Unit test for serverTree module.
 * Synopsis   : utconfTree
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = NULL;
  Server* server1 = NULL;
  Server* server2 = NULL;
  Server* server3 = NULL;
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
  if (!createExempleServerTree(coll)) goto error;
  
  // test serializing
  if (!serializeServerTree(coll)) {
    logEmit(LOG_ERR, "%s", "sorry, cannot serialize the exemple");
    goto error;
  }

  if (!(server1 = getServer(coll, "746d6ceeb76e05cfa2dea92a1c5753cd"))
      || !(server2 = getServer(coll, "6b18ed0194b0fbadd08e0a13cccda00e"))
      || !(server3 = getServer(coll, "bedac32422739d7eced624ba20f5912e")))
    goto error;
  if (rgShareItems(server1->networks, server3->networks)) goto error;

  // test reachability
  if (!isReachable(coll, server1, server2)) goto error;
  if (isReachable(coll, server1, server3)) goto error;
  if (!isReachable(coll, server2, server1)) goto error;
  if (!isReachable(coll, server2, server3)) goto error;
  if (!isReachable(coll, server3, server1)) goto error;
  if (!isReachable(coll, server3, server2)) goto error;

  // test disease
  if (!diseaseServer(coll, server1)) goto error;
  if (!diseaseServer(coll, server2)) goto error;
  if (!diseaseServer(coll, server3)) goto error;
  env.dryRun = TRUE;
  if (!serializeServerTree(coll)) goto error;
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
