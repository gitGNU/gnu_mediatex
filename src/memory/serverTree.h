/*=======================================================================
 * Version: $Id: serverTree.h,v 1.9 2015/09/17 18:53:47 nroche Exp $
 * Project: MediaTeX
 * Module : server tree
 *
 * Server producer interface

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

#ifndef MISC_MEMORY_SERVER_H
#define MISC_MEMORY_SERVER_H 1

#include "mediatex-types.h"
//#include <netinet/in.h>

/* 
   There at more 1 ISO image by archive and server.
   Images are listed by servers and archives.
   Archives matching one or more images are listed by the serverTree
 */
struct Image {
  Archive* archive; // id
  Server* server;   // id
  float score;
};

struct Server {
  char  fingerPrint[MAX_SIZE_MD5+1]; // destruction id
  char* label; // ie "MDTX2"
  char* user;  // ie "MDTX2-COLL"

  char* comment;
  char  host[MAX_SIZE_HOST+1];
  int   mdtxPort;
  int   sshPort;
  int   wwwPort;
  RG*   networks;
  RG*   gateways;

  char* userKey; // creation id
  char* hostKey;

  RG*   images;  // related images to free if we remove this server
  RG*   records; // related record to free if we remove this server

  //struct in_addr ipv4; // manage by connect.c
  struct sockaddr_in address; // manage by connect.c
  float score;                // manage by extractHtml.c
  time_t lastCommit;

  // manage by cache.c
  off_t  cacheSize; // maximum size for cache
  time_t cacheTTL;  // time to live for target files in cache
  time_t queryTTL;  // time to live for queries in memory

  int isLocalhost; // = (fingerPrint == coll->userFingerPrint)
};

struct ServerTree {
  char aesKey[MAX_SIZE_AES+1];
  Server* master;

  RG* servers;
  RG* archives; // only archives that match an image
  // note: there is only one image per archive in the server context,
  // but several in the serverTree context, up to one by server

  ScoreParam scoreParam; // parameter use to compute score
  int    minGeoDup;      // number of remote copies expected
  time_t uploadTTL;      // incoming content do not implies a bad score
  time_t serverTTL;      // used to check lastCommit
};

Image* createImage(void);
Image* destroyImage(Image* self);
int cmpImage(const void *p1, const void *p2);
int cmpImageScore(const void *p1, const void *p2);
int serializeImage(Image* self, FILE* fd);

Server* createServer(void);
Server* destroyServer(Server* self);
int cmpServer(const void *p1, const void *p2);
int serializeServer(Server* self, FILE* fd);

ServerTree* createServerTree(void);
ServerTree* destroyServerTree(ServerTree* self);
int serializeServerTree(Collection* coll);

/* API */

Image* getImage(Collection* coll, Server* server, Archive* archive);
Image* addImage(Collection* coll, Server* server, Archive* archive);
int delImage(Collection* coll, Image* Image);

Server* getServer(Collection* coll, char* fingerPrint);
Server* addServer(Collection* coll, char* fingerPrint);
int delServer(Collection* coll, Server* Server);

int diseaseServer(Collection* coll, Server* server);
int diseaseServerTree(Collection* coll);

#endif /* MISC_MEMORY_SERVER_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
