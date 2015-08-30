/*=======================================================================
 * Version: $Id: utFunc.c,v 1.14 2015/08/30 17:07:57 nroche Exp $
 * Project: MediaTeX
 * Module : utFunc
 *
 * Functions building memory modules's configuration used by unit tests
 * (please look at utconfTree.c for some explanation on the topology)

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

#include "../src/mediatex-config.h"
#include "memory/utFunc.h"

// share by supports and collection
/*static char supportNames[3][3][MAX_SIZE_STRING] = { */
/*   {"SUPP11_logo.png", "SUPP12_logo.png", "SUPP13_logo.png"},  */
/*   {"SUPP21_logo.part1", "SUPP22_logo.part1", "SUPP23_logo.part1"},  */
/*   {"SUPP31_logo.part2", "SUPP32_logo.part2", "SUPP33_logo.part2"} */
/* }; */

static char supportNames[3][3][MAX_SIZE_STRING];

/*=======================================================================
 * Function   : createExempleSupportTreeNames 
 * Description: crete supports names
 * Synopsis   : int createExempleSupportTreeNames
 * Input      : char* inputPath: absolute directory path to use
 * Output     : TRUE on success
 =======================================================================*/
int
createExempleSupportTreeNames(char* inputPath)
{
  int rc = FALSE;
  int i=0, j=0, l=0, l2=0;
  char* miscPath = 0;
  char* absolutePath = 0;
  int isThere = 0;

  char supportEndingNames[3][3][64] = {
    // server1             server2              server3
    {"SUPP11_logo.png",   "SUPP12_logo.png",   "/logo.png"},
    {"SUPP21_logo.part1", "/logoP1.iso",       "SUPP23_logo.part1"},
    {"/logoP2.iso",       "SUPP32_logo.part2", "SUPP33_logo.part2"}
  };

  logMemory(LOG_DEBUG, "createExempleSupportTreeNames");

  if (isEmptyString(inputPath)) {
    logMemory(LOG_ERR, 
	      "you have to provide an absolute path using -d option");
    goto error;
  }

  if (!(miscPath = createString(inputPath))) goto error;
  if (!(miscPath = catString(miscPath, "/../misc/"))) goto error;
  if (!(absolutePath = getAbsolutePath(miscPath))) goto error;
  l = strlen(absolutePath);

  for (i=0; i<3; ++i) {
    for (j=0; j<3; ++j) {
      l2 = 0;
      if (*supportEndingNames[i][j] == '/') {
	strcpy(supportNames[i][j], absolutePath);
	l2 = l;
      }
      
      strcpy(supportNames[i][j] + l2, supportEndingNames[i][j]);
      
      if (l2) {
	if (!callAccess(supportNames[i][j], &isThere)) goto error;
	if (!isThere) {
	  logMemory(LOG_ERR, "cannot find %s support file",
		    supportNames[i][j]);
	  goto error;
	}
      }
    }
  }	     
    
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "createExempleSupportTreeNames fails");
  }
  destroyString(absolutePath);
  destroyString(miscPath);
  return rc;
}

/*=======================================================================
 * Function   : createExempleSupportTree 
 * Description: add 9 supports to configuration
 * Synopsis   : int createExempleSupportTree(char* inputPath)
 * Input      : char* inputPath: absolute directory path to use
 * Output     : TRUE on success
 * Note       : As we use addSupport, we must use a Configuration
 .........................................................................
 * See        : utsupportTree, utsupportFile, utupgrade, utmotd
 *
 * Add 9 supports from 1970 to 2010 in order to check scores
 * - logo.png: 3 supports (1970, 1996, 1998)
 * - logo.part1: 3 supports (2000, 2002, 2004)
 * - logo.part2: 3 supports (2006, 2008, 2010)
 =======================================================================*/
int
createExempleSupportTree(char* inputPath)
{
  Configuration* conf = 0;  
  Support *supp = 0;
  int i=0, j=0, k=0;

  /*
    for i in $(seq 1994 2 2010); do \
    date -d "$i-01-01 00:00:00 +00:00" +"%s"; \
    done
  */

  time_t firstSeens[] = {
    0,
    820454400,
    883612800,
    946684800,
    1009843200,
    1072915200,
    1136073600,
    1199145600,
    1262304000
  };

  time_t lastChecks[] = {
    157766400,
    1262304000,
    1262304000,
    1262304000,
    1262304000,
    1262304000,
    1262304000,
    1262304000,
    1262304000
  };

  time_t lastSeens[] = {
    1262304000,
    1262304000,
    1262304000,
    1262304000,
    1262304000,
    1262304000,
    1262304000,
    1262304000,
    1262304000
  };

  char quickHashs[][33] = {
    "022a34b2f9b893fba5774237e1aa80ea",
    "022a34b2f9b893fba5774237e1aa80ea",
    "022a34b2f9b893fba5774237e1aa80ea",
    "de5008799752552b7963a2670dc5eb18",
    "de5008799752552b7963a2670dc5eb18",
    "de5008799752552b7963a2670dc5eb18",
    "0a7ecd447ef2acb3b5c6e4c550e6636f",
    "0a7ecd447ef2acb3b5c6e4c550e6636f",
    "0a7ecd447ef2acb3b5c6e4c550e6636f"
  };

  char fullHashs[][33] = {
    "022a34b2f9b893fba5774237e1aa80ea",
    "022a34b2f9b893fba5774237e1aa80ea",
    "022a34b2f9b893fba5774237e1aa80ea",
    "de5008799752552b7963a2670dc5eb18",
    "de5008799752552b7963a2670dc5eb18",
    "de5008799752552b7963a2670dc5eb18",
    "0a7ecd447ef2acb3b5c6e4c550e6636f",
    "0a7ecd447ef2acb3b5c6e4c550e6636f",
    "0a7ecd447ef2acb3b5c6e4c550e6636f"
  };

  // maximum size is 2305843009213693952ULL =2^61 (2^40=1TeraBytes)
  off_t sizes[] = {
    24075ULL,
    24075ULL,
    24075ULL,
    391168ULL, 
    391168ULL, 
    391168ULL, 
    374784ULL, 
    374784ULL, 
    374784ULL
  };
 
  char status[][10] = {
    "??", 
    "??", 
    "??", 
    "??", 
    "??", 
    "??", 
    "??", 
    "??", 
    "??"
  };

  if (!createExempleSupportTreeNames(inputPath)) goto error;

  if ((conf = getConfiguration()) == 0) {
    logMemory(LOG_ERR, "cannot load configuration");
    goto error;
  }

  for (i=0; i<3; ++i) {
    for (j=0; j<3; ++j) {
      k = i*3+j;
      if (!(supp = addSupport(supportNames[i][j]))) goto error;
      supp->firstSeen = firstSeens[k];
      supp->lastCheck = lastChecks[k];
      supp->lastSeen = lastSeens[k];
      strncpy(supp->quickHash, quickHashs[k], 32);
      strncpy(supp->fullHash, fullHashs[k], 32);
      supp->size = sizes[k];
      strncpy(supp->status, status[k], 9);    
    }
  }

  return TRUE;
 error:
  return FALSE;
}

/*=======================================================================
 * Function   : createExempleExtractTree 
 * Description: quick build for tests
 * Synopsis   : int createExempleExtractTree
 * Input      : N/A
 * Output     : N/A
 -------------------------------------------------------------------------
 * See        : utextractTree, utextractFile, utextractScore,
 *              utextractHtml, utcache, utextract
 *
 * Add extraction rules for all containers,
 * and add 2 uploaded file (INC container)
 =======================================================================*/
int createExempleExtractTree(Collection* coll)
{
  Archive *iso1, *iso2, *catP1, *catP2, *tgz, *logo, *xpm, 
    *tbz, *afio, *zip, *rar1, *rar2,
    *tar, *cpio, *gzip, *bzip;
  Container* container = 0;
  FromAsso* asso = 0;
  time_t time = 0;
  struct tm date;
  char dateString[32];

  // documentTree
  if (coll->extractTree == 0) goto error;

  // records
  if ((iso1 = 
       addArchive(coll, "de5008799752552b7963a2670dc5eb18", 391168))
      == 0) goto error;
  if ((iso2 = 
       addArchive(coll, "0a7ecd447ef2acb3b5c6e4c550e6636f", 374784))
      == 0) goto error;
  if ((catP1 = 
       addArchive(coll, "1a167d608e76a6a4a8b16d168580873c", 20480))
      == 0) goto error;
  if ((catP2 = 
       addArchive(coll, "c0c055a0829982bd646e2fafff01aaa6", 4066))
      == 0) goto error;
  if ((tgz = 
       addArchive(coll, "0387eee9820fa224525ff8b2e0dfa9be", 24546))
      == 0) goto error;
  if ((logo = 
       addArchive(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075))
      == 0) goto error;
  if ((xpm = 
       addArchive(coll, "b281449c229bcc4a3556cdcc0d3ebcec", 815))
      == 0) goto error;
  if ((tbz = 
       addArchive(coll, "b5810aba99ed0d15f1f0a073e646d5bd", 25045))
      == 0) goto error;
  if ((afio = 
       addArchive(coll, "65f1464142f405dbbeeea4830c95ddd3", 25600))
      == 0) goto error;
  if ((zip = 
       addArchive(coll, "d79581b67ec1932fd276dcf3b6d6db9a", 24733))
      == 0) goto error;
  if ((rar1 = 
       addArchive(coll, "7808473c7db9a9e3493a6b61f44f8805", 20480))
      == 0) goto error;
  if ((rar2 = 
       addArchive(coll, "db8b80896f7202b656913f99d438075c", 4090))
      == 0) goto error;
  if ((tar = 
       addArchive(coll, "f54fba7d2e070d48051ac3f8f1c2eb98", 30720))
      == 0) goto error;
  if ((cpio = 
       addArchive(coll, "b34bb9bf9ae4ec5b4a5bc2ab3e2a18c5", 25088))
      == 0) goto error;
  if ((gzip = 
       addArchive(coll, "3a04277dd1f43740a5fe17fd0ae9a5aa", 24457))
      == 0) goto error;
  if ((bzip = 
       addArchive(coll, "fa2b0b536bf61f61617528c474e0bf61", 24973))
      == 0) goto error;

  // ISO 1
  if (!(container = addContainer(coll, ISO, iso1))) goto error;
  if (!(asso = addFromAsso(coll, catP1, container, 
			   "logoP1.cat"))) goto error;

  // ISO 2
  if (!(container = addContainer(coll, ISO, iso2))) goto error;
  if (!(asso = addFromAsso(coll, catP2, container,
			   "logoP2.cat"))) goto error;

  // CAT
  if (!(container = addContainer(coll, CAT, catP1))) goto error;
  if (!addFromArchive(coll, container, catP2)) goto error;
  if (!(asso = addFromAsso(coll, tgz, container,
			   "logo.tgz"))) goto error;

  // TGZ
  if (!(container = addContainer(coll, TGZ, tgz))) goto error;
  if (!(asso = addFromAsso(coll, logo, container,
			   "logo/logo.png"))) goto error;
  if (!(asso = addFromAsso(coll, xpm, container,
			   "logo/logo.xpm"))) goto error;

  // TBZ
  if (!(container = addContainer(coll, TBZ, tbz))) goto error;
  if (!(asso = addFromAsso(coll, logo, container,
			   "logo/logo.png"))) goto error;
  if (!(asso = addFromAsso(coll, xpm, container,
			   "logo/logo.xpm"))) goto error;

  // AFIO
  if (!(container = addContainer(coll, AFIO, afio))) goto error;
  if (!(asso = addFromAsso(coll, logo, container,
			   "logo/logo.png"))) goto error;
  if (!(asso = addFromAsso(coll, xpm, container,
			   "logo/logo.xpm"))) goto error;

  // ZIP
  if (!(container = addContainer(coll, ZIP, zip))) goto error;
  if (!(asso = addFromAsso(coll, logo, container,
			   "logo/logo.png"))) goto error;
  if (!(asso = addFromAsso(coll, xpm, container,
			   "logo/logo.xpm"))) goto error;

  // RAR
  if (!(container = addContainer(coll, RAR, rar1))) goto error;
  if (!addFromArchive(coll, container, rar2)) goto error;
  if (!(asso = addFromAsso(coll, logo, container,
			   "logo/logo.png"))) goto error;
  if (!(asso = addFromAsso(coll, xpm, container,
			   "logo/logo.xpm"))) goto error;

  // TAR
  if (!(container = addContainer(coll, TAR, tar))) goto error;
  if (!(asso = addFromAsso(coll, logo, container,
			   "logo/logo.png"))) goto error;
  if (!(asso = addFromAsso(coll, xpm, container,
			   "logo/logo.xpm"))) goto error;

  // CPIO
  if (!(container = addContainer(coll, CPIO, cpio))) goto error;
  if (!(asso = addFromAsso(coll, logo, container,
			   "logo/logo.png"))) goto error;
  if (!(asso = addFromAsso(coll, xpm, container,
			   "logo/logo.xpm"))) goto error;

  // GZIP
  if (!(container = addContainer(coll, GZIP, gzip))) goto error;
  if (!(asso = addFromAsso(coll, cpio, container,
			   "logo.cpio"))) goto error;

  // BZIP
  if (!(container = addContainer(coll, BZIP, bzip))) goto error;
  if (!(asso = addFromAsso(coll, cpio, container,
			   "logo.cpio"))) goto error;

  // INC
  if (!(time = currentTime())) goto error;
  if (localtime_r(&time, &date) == (struct tm*)0) {
    logMemory(LOG_ERR, "localtime_r returns on error");
    goto error;
  }
  sprintf(dateString, "%04i-%02i-%02i,%02i:%02i:%02i", 
	  date.tm_year + 1900, date.tm_mon+1, date.tm_mday,
	  date.tm_hour, date.tm_min, date.tm_sec);
  if (!(container = coll->extractTree->incoming)) goto error;
  if (!(asso = addFromAsso(coll, iso1, container, dateString))) goto error;
  if (!(asso = addFromAsso(coll, iso2, container, "1994-01-01,00:00:00"))) 
    goto error;

  // this one should be automatically removed
  if (!(asso = addFromAsso(coll, logo, container, dateString))) goto error;

  return TRUE;
 error:
  logMemory(LOG_ERR, 
	  "sorry, cannot build the extract exemple... gdb is your friend");
  return FALSE;
}

/*=======================================================================
 * Function   : createExempleCatalogTree 
 * Description: quick build for tests
 * Synopsis   : int createExempleCatalogTree(Collection* coll)
 * Input      : Collection* coll
 * Output     : N/A
 -------------------------------------------------------------------------
 * See        : utcatalogTree, utcatalogFile, utcatalogHtml
 *
 * Add information about logo.png
 =======================================================================*/
int 
createExempleCatalogTree(Collection* coll)
{
  int rc = TRUE;
  AssoCarac* assoCarac = 0;
  AssoRole* assoRole = 0;
  Carac* carac = 0;
  Role* role = 0;
  Category* category = 0;
  Category* father = 0;
  Human* human = 0;
  Document* document = 0;
  Archive* archive = 0;

  // catalogTree
  rc=rc&& (coll->catalogTree);

  // category 1
  rc=rc&& (category = addCategory(coll, "media", TRUE));
  // + carac
  rc=rc&& (carac = addCarac(coll, "topo"));
  rc=rc&& (assoCarac = addAssoCarac(coll, carac, CATE, category, 
				    "Welcome !"));
  
  // category 2
  rc=rc&& (category = addCategory(coll, "image", TRUE));
  rc=rc&& (father = addCategory(coll, "media", FALSE));
  rc=rc&& addCategoryLink(coll, father, category);

  // category 3
  rc=rc&& (category = addCategory(coll, "drawing", TRUE));
  rc=rc&& (father = addCategory(coll, "image", FALSE));
  rc=rc&& addCategoryLink(coll, father, category);

  // category 4
  rc=rc&& (category = addCategory(coll, "animal", TRUE));
  
  // category 5
  rc=rc&& (category = addCategory(coll, "\\\"hand\\\"", FALSE));
  rc=rc&& (father = addCategory(coll, "drawing", FALSE));
  rc=rc&& addCategoryLink(coll, father, category);
  rc=rc&& (father = addCategory(coll, "animal", FALSE));
  rc=rc&& addCategoryLink(coll, father, category);

  // human
  rc=rc&& (human = addHuman(coll, "Me", ""));
  // + caracs
  rc=rc&& (carac = addCarac(coll, "another thing?"));
  rc=rc&& (assoCarac = addAssoCarac(coll, carac, HUM, human, "no"));
  // + categories
  rc=rc&& (category = addCategory(coll, "drawing", FALSE));
  rc=rc&& addHumanToCategory(coll, human, category);
  
  // document
  rc=rc&& (document = addDocument(coll, "panthere"));
  // + caracs
  rc=rc&& (carac = addCarac(coll, "info"));
  rc=rc&& (assoCarac = 
	   addAssoCarac(coll, carac, DOC, document, 
			"[P]erenial [A]rchive [N]etwork [There]"));

  // + archives
  // archive 1
  rc=rc&& (archive = 
	   addArchive(coll, 
		      "022a34b2f9b893fba5774237e1aa80ea", 24075));
  // + caracs
  rc=rc&& (carac = addCarac(coll, "format"));
  rc=rc&& (assoCarac = addAssoCarac(coll, carac, ARCH, archive, "PNG"));
  rc=rc&& addArchiveToDocument(coll, archive, document);
  
  // archive 2
  rc=rc&& (archive = 
	   addArchive(coll, 
		      "b281449c229bcc4a3556cdcc0d3ebcec", 815));
  // + caracs
  rc=rc&& (carac = addCarac(coll, "format"));
  rc=rc&& (assoCarac = addAssoCarac(coll, carac, ARCH, archive, "XPM"));
  rc=rc&& (carac = addCarac(coll, "licence"));
  rc=rc&& (assoCarac = addAssoCarac(coll, carac, ARCH, archive, "GPLv3"));
  rc=rc&& addArchiveToDocument(coll, archive, document);

  // + roles
  rc=rc&& (human = addHuman(coll, "Me", ""));
  rc=rc&& (role = addRole(coll, "designer"));
  rc=rc&& (assoRole = addAssoRole(coll, role, human, document));

  // + categories
  rc=rc&& (category = addCategory(coll, "drawing", FALSE));
  rc=rc&& addDocumentToCategory(coll, document, category);
  rc=rc&& (category = addCategory(coll, "animal", FALSE));
  rc=rc&& addDocumentToCategory(coll, document, category);
  rc=rc&& (category = addCategory(coll, "hand", FALSE));
  rc=rc&& addDocumentToCategory(coll, document, category);
  
  if (!rc) {
    logMemory(LOG_ERR, "sorry, cannot build the catalog exemple");
    coll->catalogTree = destroyCatalogTree(coll->catalogTree);
  }
  return rc;
}

/*=======================================================================
 * Function   : createExempleConfiguration
 * Description: quick build for other module's tests
 * Synopsis   : int buildTestConfiguration(char* inputPath)
 * Input      : char* inputPath: absolute directory path to use
 * Output     : TRUE on success
 * Note       : Networks and Gateways are defined into utconfTree.c,
 *              except for coll3, using the collection settings
 .........................................................................
 * See        : utconfTree, utconfFile, utserverTree, utnotify
 *              All unit test use mdtx1 configuration except
 *              memory/confTree and server/notify that use mdtx2
 *              Unit test mainly use coll1 except server's unit test
 *              that use coll3, and client/upload that use coll2?!
 =======================================================================*/
int 
createExempleConfiguration(char* inputPath)
{
  int rc = FALSE;
  Configuration* self = 0;
  Collection* coll = 0;
  Support* supp = 0;
  RGIT* curr = 0;
  char* string = 0;
  char host[] = "hostX.mediatex.org";
  int i = 0, j = 0;

  logMemory(LOG_DEBUG, "build the configuration exemple.");

  if (!createExempleSupportTreeNames(inputPath)) goto error;
  if (!(self = getConfiguration())) goto error;
  host[4] = env.confLabel[4]; // same host number as conf label number
  strncpy(self->host, host, MAX_SIZE_HOST);
  if (!(self->comment = createString(host))) goto error;
  if (!(self->comment = catString(self->comment, " configuration file")))
    goto error;
  
  // needed for cgi unit test (wich connect serv1 and serv2)
  if (env.confLabel[4] == '1') strcpy(self->host, "localhost");

  /* test1 collection */
  if (!(coll = addCollection("coll1"))) goto error;
  if (!(coll->masterLabel = createString("mdtx1"))) goto error;
  strncpy(coll->masterHost, "host1.mediatex.org", MAX_SIZE_HOST);
  coll = 0;

  /* test2 collection */
  if (!(coll = addCollection("coll2"))) goto error;
  if (!(coll->masterLabel = createString("mdtx2"))) goto error;
  strncpy(coll->masterHost, "host2.mediatex.org", MAX_SIZE_HOST);
  coll->cacheSize = 200*MEGA;
  coll->cacheTTL = 1*MONTH;
  coll->queryTTL = 15*DAY;
  coll = 0;
  
  /* test3 collection */
  if (!(coll = addCollection("coll3"))) goto error;
  // (non sens: mdtx3 as a collection master on private network,
  //  but nevermind)
  if (!(coll->masterLabel = createString("mdtx3"))) goto error;
  strncpy(coll->masterHost, "host3.mediatex.org", MAX_SIZE_HOST);
  coll->masterPort = 33;
  
  /* share 9 supports with collection 1 (this function is call 3 times) */
  if (!(coll = getCollection("coll1"))) goto error;
  i = env.confLabel[4] - '1';
  for (j=0; j<3; ++j) {
    if (!(supp = addSupport(supportNames[j][i]))) goto error;
    if (!addSupportToCollection(supp, coll)) goto error;
  }

  // overwrite networks on collection coll3
  if (!(coll = getCollection("coll3"))) goto error;
  if (!(string = addNetwork("private2"))) goto error;
  if (!rgInsert(coll->networks, string)) goto error;

  // build strings and check paths
  if (!expandConfiguration()) goto error;

  // get host fingerprint
  if (!populateConfiguration()) goto error;

  // get the localhost fingerprints for the collections
  while((coll = rgNext_r(self->collections, &curr))) {
    if (!populateCollection(coll)) goto error;
  }

  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, 
	    "sorry, cannot build the configuration exemple.");  
    freeConfiguration();
  }
  return rc;
}

/*=======================================================================
 * Function   : createExempleServerTree
 * Description: quick build for tests
 * Synopsis   : int createExempleServerTree(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 ------------------------------------------------------------------------
 * See        : utserverTree, utserverFile, utextractScore
 =======================================================================*/
int 
createExempleServerTree(Collection* coll)
{
  Server *server = 0;
  Image* image = 0;
  Archive* archive = 0;
  char* string = 0;
  int i=0, j=0;

  char* labels[] = {
    "mdtx1", "mdtx2", "mdtx3"};
  char* comments[] = {
    "this is server1", "this is server2", "this is server3"};
  char hosts[][MAX_SIZE_HOST+1] = {
    "localhost", "127.0.0.1", "mediatex.org"};
  int mdtxPorts[] = {11111, 6560, 33333};
  int sshPorts[] = {22, 22, 2222};
  int wwwPorts[] = {443, 443, 4443};

  // fingerprints related to confTree
  char serverId[][MAX_SIZE_HASH+1] = {
    "746d6ceeb76e05cfa2dea92a1c5753cd",
    "6b18ed0194b0fbadd08e0a13cccda00e",
    "bedac32422739d7eced624ba20f5912e"};

  // networks
  char networks[][3][32] = {
    {"www", "\0"},
    {"www", "private", "\0"}, 
    {"private", "\0"}
  };
 
  // gateways
  char gateways[][3][32] = {
    {"\0"}, 
    {"private", "\0"}, 
    {"\0"}
  };

  // 3 images on 3 servers
  char* hashs[][3] = {
    {"de5008799752552b7963a2670dc5eb18",
     "\0", "\0"},
    {"022a34b2f9b893fba5774237e1aa80ea",
     "de5008799752552b7963a2670dc5eb18",
     "\0"},
    {"022a34b2f9b893fba5774237e1aa80ea",
     "de5008799752552b7963a2670dc5eb18",
     "0a7ecd447ef2acb3b5c6e4c550e6636f"}
  };
  off_t sizes[][3] = {
    {391168ULL, 0, 0},
    {24075ULL, 391168ULL, 0},
    {24075ULL, 391168ULL, 374784ULL}
  };
  float scores[][3] = {
    {2, 0, 0},
    {7.5, 10, 0},
    {9, 9, 7.5}
  };

  char* userKeys[] = {
    "ssh-dss AAAAB3NzaC1kc3MAAACBANTUb46yvsV3bL8AX7oSg48oyN2MPqVCLWL8GiGADVzX71dZmYiqFeTJ5v3vBgk/7rEiaUzfAIZ0ebXhKB1TSc17Vqbdx2DfHm3/IbgBgrBe47VAkbPX+iW2n1zC9fBY5bc/819V0Szf7qvUyzZUkJ/9Vsi79Ikbq7rWu8r3VBeVAAAAFQDTyYPXMe1NzSrUHagG4BLlEQEx7QAAAIEAmYWtuhy0uuoLsoZu96ECnD6Se+buik4WhEi6xuAMPe+/RR7z3IHTh7Xvg56PyfqlFNReFnlc1s23ZZMmWsSAtNYFWqGrir/rddfisDm9Esk5aqPuwi8ToEFikGpthbJZYHjz4PpB2jfWDyLax2HpzFx4vmd1LcrCCM5PtuJ1sEAAAACBALPb55K2CZT3etbLgJ0rfoYDMnutNl38kK5rxf5Ps7Qfq2Uayv1BSKh8arOoMi94qoGKmBDLmwy99raNL4UhVly6YcnOPZ1yhVMfSGy2B1bFHIxh4qhxulm6VsVcPxIQpfwjQCRPImo57jkIjVk49o+K6DY7pDuy60MOmkXPr/sC mdtx@host",
    "ssh-dss AAAAB3NzaC1kc3MAAACBANTUb46yvsV3bL8AX7oSg48oyN2MPqVCLWL8GiGADVzX71dZmYiqFeTJ5v3vBgk/7rEiaUzfAIZ0ebXhKB1TSc17Vqbdx2DfHm3/IbgBgrBe47VAkbPX+iW2n1zC9fBY5bc/819V0Szf7qvUyzZUkJ/9Vsi79Ikbq7rWu8r3VBeVAAAAFQDTyYPXMe1NzSrUHagG4BLlEQEx7QAAAIEAmYWtuhy0uuoLsoZu96ECnD6Se+buik4WhEi6xuAMPe+/RR7z3IHTh7Xvg56PyfqlFNReFnlc1s23ZZMmWsSAtNYFWqGrir/rddfisDm9Esk5aqPuwi8ToEFikGpthbJZYHjz4PpB2jfWDyLax2HpzFx4vmd1LcrCCM5PtuJ1sEAAAACBALPb55K2CZT3etbLgJ0rfoYDMnutNl38kK5rxf5Ps7Qfq2Uayv1BSKh8arOoMi94qoGKmBDLmwy99raNL4UhVly6YcnOPZ1yhVMfSGy2B1bFHIxh4qhxulm6VsVcPxIQpfwjQCRPImo57jkIjVk49o+K6DY7pDuy60MOmkXPr/sC mdtx@host",
    "ssh-dss AAAAB3NzaC1kc3MAAACBANTUb46yvsV3bL8AX7oSg48oyN2MPqVCLWL8GiGADVzX71dZmYiqFeTJ5v3vBgk/7rEiaUzfAIZ0ebXhKB1TSc17Vqbdx2DfHm3/IbgBgrBe47VAkbPX+iW2n1zC9fBY5bc/819V0Szf7qvUyzZUkJ/9Vsi79Ikbq7rWu8r3VBeVAAAAFQDTyYPXMe1NzSrUHagG4BLlEQEx7QAAAIEAmYWtuhy0uuoLsoZu96ECnD6Se+buik4WhEi6xuAMPe+/RR7z3IHTh7Xvg56PyfqlFNReFnlc1s23ZZMmWsSAtNYFWqGrir/rddfisDm9Esk5aqPuwi8ToEFikGpthbJZYHjz4PpB2jfWDyLax2HpzFx4vmd1LcrCCM5PtuJ1sEAAAACBALPb55K2CZT3etbLgJ0rfoYDMnutNl38kK5rxf5Ps7Qfq2Uayv1BSKh8arOoMi94qoGKmBDLmwy99raNL4UhVly6YcnOPZ1yhVMfSGy2B1bFHIxh4qhxulm6VsVcPxIQpfwjQCRPImo57jkIjVk49o+K6DY7pDuy60MOmkXPr/sC mdtx@host"};
  
  char* hostKeys[] = {
    "ssh-dss AAAAB3NzaC1kc3MAAACBANS+fcEeUJYXUH+58diSH8FsHQlj/3xjSVmEf5Od2ip87ro6ZSn3nJI1RR+ALqcLGn/K9fccl4Bj4Oq+M/6iMIL7ZQ6cAwzox9E0w0AjwQg+17WjWIfQNhycnuCCLcg6ThVobZ30RrNFcmwcXKKXP2CRS+eT6Ex8wC42uOEfMvQfAAAAFQDJYtWp9ZaxF1Ej/H0uu8Sqx+9dkQAAAIEAtrLXsPDqCvRwnGtusl1rlzjJCHYMJFZ5eAFDRCdNNUpGqHaHZpw9v8OIt6Qtvx6eVn97opVJ71FG9Xxpl17sIegh8/gprZZlK8ct9Lfs2YFOxcXv+i2r1dVHZwUkwSiYUE59Af71d+Os9VNStTj/U/GXtVNMS7NyqKoaWfFlItgAAACAevPvahBRDZbsjcfhf736q5vyEZbHau5s3VfSQci1u0rIekDhymTyul3KwM7aOFq3f9Y0SfMFKTfQJq45yYCMFZcwnvJSqDdUZVHffKcGcyj3N3mnIGMiU/3mbKH+xbCPn8f+HURembC9S4X+wtPHTHqhCWj2LAUDOgzdSvJ/C9o= root@squeeze",
    "ssh-dss AAAAB3NzaC1kc3MAAACBANS+fcEeUJYXUH+58diSH8FsHQlj/3xjSVmEf5Od2ip87ro6ZSn3nJI1RR+ALqcLGn/K9fccl4Bj4Oq+M/6iMIL7ZQ6cAwzox9E0w0AjwQg+17WjWIfQNhycnuCCLcg6ThVobZ30RrNFcmwcXKKXP2CRS+eT6Ex8wC42uOEfMvQfAAAAFQDJYtWp9ZaxF1Ej/H0uu8Sqx+9dkQAAAIEAtrLXsPDqCvRwnGtusl1rlzjJCHYMJFZ5eAFDRCdNNUpGqHaHZpw9v8OIt6Qtvx6eVn97opVJ71FG9Xxpl17sIegh8/gprZZlK8ct9Lfs2YFOxcXv+i2r1dVHZwUkwSiYUE59Af71d+Os9VNStTj/U/GXtVNMS7NyqKoaWfFlItgAAACAevPvahBRDZbsjcfhf736q5vyEZbHau5s3VfSQci1u0rIekDhymTyul3KwM7aOFq3f9Y0SfMFKTfQJq45yYCMFZcwnvJSqDdUZVHffKcGcyj3N3mnIGMiU/3mbKH+xbCPn8f+HURembC9S4X+wtPHTHqhCWj2LAUDOgzdSvJ/C9o= root@squeeze",
    "ssh-dss AAAAB3NzaC1kc3MAAACBANS+fcEeUJYXUH+58diSH8FsHQlj/3xjSVmEf5Od2ip87ro6ZSn3nJI1RR+ALqcLGn/K9fccl4Bj4Oq+M/6iMIL7ZQ6cAwzox9E0w0AjwQg+17WjWIfQNhycnuCCLcg6ThVobZ30RrNFcmwcXKKXP2CRS+eT6Ex8wC42uOEfMvQfAAAAFQDJYtWp9ZaxF1Ej/H0uu8Sqx+9dkQAAAIEAtrLXsPDqCvRwnGtusl1rlzjJCHYMJFZ5eAFDRCdNNUpGqHaHZpw9v8OIt6Qtvx6eVn97opVJ71FG9Xxpl17sIegh8/gprZZlK8ct9Lfs2YFOxcXv+i2r1dVHZwUkwSiYUE59Af71d+Os9VNStTj/U/GXtVNMS7NyqKoaWfFlItgAAACAevPvahBRDZbsjcfhf736q5vyEZbHau5s3VfSQci1u0rIekDhymTyul3KwM7aOFq3f9Y0SfMFKTfQJq45yYCMFZcwnvJSqDdUZVHffKcGcyj3N3mnIGMiU/3mbKH+xbCPn8f+HURembC9S4X+wtPHTHqhCWj2LAUDOgzdSvJ/C9o= root@squeeze"};

  if (!(coll->serverTree)) goto error;
  if (!(coll->serverTree->master = addServer(coll, serverId[1]))) 
    goto error;
  strncpy(coll->serverTree->aesKey, "1234567890abcdef", MAX_SIZE_AES);

  for (i=0; i<3; ++i) {
    if (!(server = addServer(coll, serverId[i]))) goto error;
    if (!(server->label = createString(labels[i]))) goto error;
    if (!(server->user = createString(labels[i])) ||
	!(server->user = catString(server->user, "-")) ||
	!(server->user = catString(server->user, coll->label))) goto error;
    strncpy(server->host, hosts[i], MAX_SIZE_HOST);
    if (!(server->comment = createString(comments[i]))) goto error;
    server->mdtxPort = mdtxPorts[i];
    server->sshPort = sshPorts[i];
    server->wwwPort = wwwPorts[i];

    // keys
    if (!(server->userKey = createString(userKeys[i]))) goto error;
    if (!(server->hostKey = createString(hostKeys[i]))) goto error;

    server->cacheSize = DEFAULT_CACHE_SIZE*(i+1);
    server->cacheTTL = DEFAULT_TTL_CACHE*(i+1);
    server->queryTTL = DEFAULT_TTL_QUERY*(i+1);

    // networks
    for (j=0; j<3; ++j) {
      if (*networks[i][j] == (char)0) break;
      if (!(string = addNetwork(networks[i][j]))) goto error;
      if (!rgInsert(server->networks, string)) goto error;
    }

    // gateways
    for (j=0; j<3; ++j) {
      if (*gateways[i][j] == (char)0) break;
      if (!(string = addNetwork(gateways[i][j]))) goto error;
      if (!rgInsert(server->gateways, string)) goto error;
    }

    // shared images
    for (j=0; j<3; ++j) {
      if (*hashs[i][j] == (char)0) break;
      if (!(archive = addArchive(coll, hashs[i][j], sizes[i][j]))) 
	goto error;
      if (!(image = addImage(coll, server, archive))) goto error;
      image->score = scores[i][j];
    }
  }

  return TRUE;
 error:
  logMemory(LOG_ERR, "sorry, cannot build the configuration exemple.");
  coll->serverTree = destroyServerTree(coll->serverTree);
  return FALSE;
}

/*=======================================================================
 * Function   : createExempleRecordTree
 * Description: create a record tree for tests
 * Synopsis   : int createExempleRecordTree
 * Input      : N/A
 * Output     : TRUE on success
 ------------------------------------------------------------------------
 * Used by    : utrecordTree.c, utcacheTree.c, utopenClose.c, utmotd.c
 *              utnotify.c
 =======================================================================*/
RecordTree*
createExempleRecordTree(Collection* coll)
{
  RecordTree* rc = 0;
  Record *record = 0;
  Archive* archive = 0;
  Server* server = 0;
  char* extra = 0;
  int i=0;
  
  // build all record type :
  // FINAL_SUPPLY
  // MALLOC_SUPPLY
  // LOCAL_SUPPLY with good score
  // LOCAL_SUPPLY with bad score
  // REMOTE_SUPPLY
  // FINAL_DEMAND
  // LOCAL_DEMAND
  // REMOTE_DEMAND

  char fingerPrints[][MAX_SIZE_HASH+1] = {
    "746d6ceeb76e05cfa2dea92a1c5753cd",
    "746d6ceeb76e05cfa2dea92a1c5753cd",
    "746d6ceeb76e05cfa2dea92a1c5753cd",
    "746d6ceeb76e05cfa2dea92a1c5753cd",
    "7af51aceb06864e690fa6a9e00000001",
    "746d6ceeb76e05cfa2dea92a1c5753cd",
    "746d6ceeb76e05cfa2dea92a1c5753cd",
    "7af51aceb06864e690fa6a9e00000002"};

  Type types[] = {
    SUPPLY, SUPPLY, SUPPLY, SUPPLY, SUPPLY, 
    DEMAND, DEMAND, DEMAND};

  time_t dates[] ={
    1, 2, 3, 4, 5, 6, 7, 8};
  
  char* hashs[] = {
    "022a34b2f9b893fba5774237e1aa80ea",
    "022a34b2f9b893fba5774237e1aa80ea",
    "022a34b2f9b893fba5774237e1aa80ea",
    "0a7ecd447ef2acb3b5c6e4c550e6636f",
    "022a34b2f9b893fba5774237e1aa80ea",
    "022a34b2f9b893fba5774237e1aa80ea",
    "022a34b2f9b893fba5774237e1aa80ea",
    "022a34b2f9b893fba5774237e1aa80ea"};
  
  off_t sizes[] = {
    24075ULL, 24075ULL, 24075ULL, 374784ULL, 24075ULL,
    24075ULL, 24075ULL, 24075ULL};

  char* extras[] = {
    "/logo.png", "!malloc", "logo.png", "logoP2.iso", "logo.png",
    "test@test.com", "!wanted", "!wanted"};

  if (!(rc = createRecordTree())) goto error;
  rc->collection = coll;
  rc->messageType = DISK;

  for (i=0; i<7; ++i) {
    if (!(server = addServer(coll, fingerPrints[i]))) goto error;
    if (!(archive = addArchive(coll, hashs[i], sizes[i]))) goto error;
    if (!(extra = createString(extras[i]))) goto error;
    if (!(record = addRecord(coll, server, archive, types[i], extra)))
      goto error;

    record->date = dates[i];

    // add the record to the tree
    if (!rgInsert(rc->records, record)) goto error;
  }
 
  return rc;
 error:
  rc = destroyRecordTree(rc);
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
