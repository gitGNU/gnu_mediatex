/*=======================================================================
 * Project: MediaTeX
 * Module: etcConf
 *
 * /etc configuration producer interface
 * Notes: object states
 1) malloc => created 
 2) malloc path => expanded
 3) parser => parsed
 4) malloc sub-trees => populated

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

#ifndef MDTX_MEMORY_CONF_H
#define MDTX_MEMORY_CONF_H 1

#include "mediatex-types.h"

// Object status :
// EXPANDED: path allocated and checked
// POULATED: key loaded (implies seuid)
typedef enum {EXPANDED=1, POPULATED=2} MemoryState;

// Load/Save data files
// Note: LOADED FileState implies POPULATED MemoryState
#undef CONF // openssl/ossl_typ.h:165:24: typedef struct conf_st CONF;
typedef enum {iCFG=0, iSUPP=1} ConfFileIdx;
typedef enum {CFG=1, SUPP=2} ConfFile;
typedef enum {iCTLG=0, iEXTR=1, iSERV=2, iCACH=3} CollFileIdx;
typedef enum {CTLG=1, EXTR=2, SERV=4, CACH=8} CollFile;
typedef enum {DISEASED=0, LOADED=1, MODIFIED=2} FileState;

// Motd policy : Most/All
typedef enum {mUNDEF=0, MOST=1, ALL=2} MotdPolicy;

struct Collection {

  /* from configuration file: mediatex.conf */
  // note: getconf LOGIN_NAME_MAX gives 256... I prefer to use malloc
  char   label[MAX_SIZE_COLL+1]; // ie: "COLL"
  char  *masterLabel;            // ie: "MDTX2
  char   masterHost[MAX_SIZE_HOST+1]; // gitbare server host name
  int    masterPort; // SSH server port, also used by CVS

  /* cache parameters */
  off_t  cacheSize; // maximum size for cache
  time_t cacheTTL;  // time to live for target files in cache
  time_t queryTTL;  // time to live for queries in memory
  MotdPolicy motdPolicy; // retrieve all images locally or not (default)

  /* objects */
  Server* localhost; // set by common/upgrade.c or getLocalHost()

  /* avl tree */
  AVLTree* archives; // all archives share by trees (Archive*)
  int maxId; // id used to serialize archives on html pages

  /* -rings */
  RG* networks;
  RG* gateways;
  RG *supports; // ring of support (Support*)

  /* ... expanded data ... */
  char  *user;       // ie: "MDTX-COLL""
  char  *masterUser; // ie: "MDTX2-COLL"

  /* shared parameters (used by servers.txt) */
  char *userKey;   // ~/.ssh/id_dsa.pub content
  char  userFingerPrint[MAX_SIZE_MD5+1];

  /* -load/save */
  MemoryState memoryState;
  FileState fileState[4];
  pthread_mutex_t mutex[4];
  int cptInUse[4];
  int toUpdate;
  int toCommit;

  /* computed paths */

  /* -directories */
  char *homeDir;    // home base directory for collection users
  char *cacheDir;   // where files are stored in filesystem
  char *extractDir; // use for extractions
  char *gitDir;     // git local copies's directory
  char *sshDir;     // .ssh directory
  char *htmlDir;      // ~/public_html directory
  char *htmlIndexDir; // HTML access index
  char *htmlCacheDir; // where files are browsed by apache
  char *htmlScoreDir; // HTML support index
  char *htmlCgiDir; // HTML support index

  /* -files */
  char *catalogDB;  // catalog.txt path
  char *serversDB;  // servers.txt path
  char *extractDB;  // extract.txt path
  char *md5sumsDB;  // {COLL}.md5 path
  char *sshAuthKeys;     // "~/.ssh/authorized_keys"
  char *sshConfig;       // "~/.ssh/config"
  char *sshKnownHosts;   // "~/.ssh/known_hosts"
  char *sshRsaPublicKey; // "~/.ssh/id_rsa.pub"
  char *sshDsaPublicKey; // "~/.ssh/id_dsa.pub"

  /* -urls */
  char *cgiUrl;     // url to local cgi script
  char *cacheUrl;   // base url to cache repository

  /* -parsed trees: */  
  ServerTree*  serverTree;
  ExtractTree* extractTree;
  CatalogTree* catalogTree;  
  CacheTree*   cacheTree;
};

struct Configuration {

  /* from configuration file: mediatex.conf */
  /* shared parameters (used by servers.txt) */
  char *comment;   // comment
  char  host[MAX_SIZE_HOST+1]; // hostname
  int   mdtxPort;  // listening port for incoming requests (daemon)
  int   sshPort;   // listening port for incoming ssh queries
  int   httpPort;   // listening port for incoming http web queries
  int   httpsPort;   // listening port for incoming https web queries
  off_t uploadRate; // average upload rate

  /* default values for collections */
  off_t  cacheSize; // maximum size for cache
  time_t cacheTTL;  // time to live for target files in cache
  time_t queryTTL;  // time to live for queries in memory

  /* local parameters */
  time_t checkTTL;  // time period between 2 md5 checks on supports
  time_t fileTTL;   // time period between 2 md5 checks on support files
  ScoreParam scoreParam; // parameter use to compute score
  MotdPolicy motdPolicy; // retrieve all images locally or not (default)

  /* keys (not parsed from the configuration file) */
  char* hostKey;  // /etc/ssh/ssh_host_dsa.pub content
  char  hostFingerPrint[MAX_SIZE_MD5+1];

  /* computed paths */
  char* gitBareDir;
  char* scriptsDir;
  char* homeDir;
  char* md5sumDir;
  char* jailDir;    // chroot jail
  char* cacheDir;
  char* extractDir;
  char* gitDir;
  char* mdtxGitDir;
  char* hostSshDir;

  /* -files */
  char* confFile;
  char* pidFile;
  char* sshRsaPublicKey; // "/etc/ssh/ssh_host_rsa.pub"
  char* sshDsaPublicKey; // "/etc/ssh/ssh_host_dsa.pub"
  char *supportDB;

  /* rings */
  RG* allNetworks;
  RG* networks;
  RG* gateways;
  RG* collections;
  RG* supports;

  /* -load/save */
  int sem; // lock to prevent concurent access on files
  MemoryState memoryState;
  FileState fileState[2];
  int toHup;
};


/* API */

int cmpCollection(const void *p1, const void *p2);
Collection* createCollection();
Collection* destroyCollection(Collection* self);
Collection* getCurrentCollection(void);
int expandCollection(Collection* self);
int populateCollection(Collection* self);
int populateCollections(void);
int DiseaseCollection(Collection* self);

Configuration* createConfiguration(void);
Configuration* createTestConfiguration(void);
Configuration* destroyConfiguration(Configuration* self);
int serializeConfiguration(Configuration* self);
Configuration* getConfiguration(void);
int expandConfiguration(void);
int populateConfiguration(void);
void freeConfiguration(void);

Collection* getCollection(char* label);
Collection* addCollection(char* label);
int delCollection(Collection* self);
int addSupportToCollection(Support* support, Collection* coll);
int delSupportFromCollection(Support* support, Collection* coll);

char* addNetwork(char *label);
int addNetworkToRing(RG* ring, char *label);
Server* getLocalHost(Collection* coll);
MotdPolicy getMotdPolicy(Collection* coll);
  
#endif /* MDTX_MEMORY_CONF_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
