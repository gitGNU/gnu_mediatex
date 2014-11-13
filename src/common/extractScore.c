/*=======================================================================
 * Version: $Id: extractScore.c,v 1.2 2014/11/13 16:36:23 nroche Exp $
 * Project: MediaTeX
 * Module : common/extractScore
 *
 * Manage extraction scores

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
#include "../common/openClose.h"
#include "../common/extractScore.h"

#include <avl.h> 

int computeArchive(Archive* self, int depth);


/*=======================================================================
 * Function   : populateExtractTree
 * Description: Compute the uniq image's extract score from server.txt
 * Synopsis   : int populateExtractTree(Collection* coll)
 * Input      : ExtractTree* self = the ExtractTree to populate
 *              ServerTree* servers = the inputs
 * Output     : TRUE on success
 * Note       : This function MUST be called before the extractFile 
 *              parser!
 =======================================================================*/
int 
populateExtractTree(Collection* coll)
{
  int rc = FALSE;
  ExtractTree* self = NULL;
  ServerTree* serverTree = NULL;
  Server* server = NULL;
  Archive* archive = NULL;
  Image* image = NULL;
  int i = 0;

  checkCollection(coll);
  logEmit(LOG_DEBUG, "populateExtractTree: %s", coll->label);

  self = coll->extractTree;
  if(self == NULL) {
    logEmit(LOG_INFO, "%s", "cannot populate empty ExtractTree");
    goto error;
  }
  
  serverTree = coll->serverTree;
  if(serverTree == NULL) {
    logEmit(LOG_INFO, "%s",
 	    "cannot populate tree with an empty ServerTree");
    goto error;
  }

  // for each server
  rgRewind(serverTree->servers);
  while ((server = rgNext(serverTree->servers)) != NULL) {
    
    // server->score = min (image's scores)
    server->score = -1;
    if (server->images != NULL) {
      rgRewind(server->images);
      while((image = rgNext(server->images)) != NULL) {
	if (server->score == -1 || image->score < server->score) {
	  server->score = image->score;
	}
      }
    }
  }

  // for each archive related to images
  rgRewind(serverTree->archives);
  while ((archive = rgNext(serverTree->archives)) != NULL) {

    // archive->imageScore = sum (image's scores)
    archive->imageScore = -1;
    if (archive->images != NULL) {
      archive->imageScore = 0;
      rgRewind(archive->images);

      logEmit(LOG_INFO, "local image score for %s:%lli",
	      archive->hash, archive->size);

      i=0;
      while((image = rgNext(archive->images)) != NULL) {
	archive->imageScore += image->score;
	logEmit(LOG_INFO, "%c %5.2f", (i > 1)?'+':' ', image->score);
	++i;
      }
    }
    
    logEmit(LOG_INFO, "= %5.2f", archive->imageScore);

    // archive->imageScore /= minGeoDup
    archive->imageScore /= serverTree->minGeoDup;
    logEmit(LOG_INFO, "/ %i", serverTree->minGeoDup);

    // truncate it if more than maxScore
    if (archive->imageScore > coll->serverTree->scoreParam.maxScore) {
      archive->imageScore = coll->serverTree->scoreParam.maxScore;
      logEmit(LOG_INFO, "> %5.2f", coll->serverTree->scoreParam.maxScore);
    }

    logEmit(LOG_INFO, "-------", archive->imageScore);
    logEmit(LOG_INFO, "= %5.2f", archive->imageScore);
  }    

  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to populateExtractTree");
  }  
  return rc;
}


/*=======================================================================
 * Function   : computeContainer
 * Description: Compute container's scores (from its parents files)
 * Synopsis   : int computeContainer(Container* self)
 * Input      : ExtractTree* self
 * Output     : TRUE on success
 =======================================================================*/
int 
computeContainer(Container* self, int depth)
{
  int rc = FALSE;
  Archive* archive = NULL;

  checkContainer(self);
  if (self->score != -1) goto quit; // already computed
  logEmit(LOG_DEBUG, "%*s computeContainer %s/%s:%lli", 
	  depth, "", strEType(self->type),
	  self->parent->hash, (long long int)self->parent->size);

  // score = min ( content's scores )
  rgRewind(self->parents);
  while ((archive = rgNext(self->parents)) != NULL) {
    if (!computeArchive(archive, depth+1)) goto error;
    if (self->score == -1 || self->score > archive->extractScore) {
      self->score = archive->extractScore;
    }
  }
  
 quit:
  logEmit(LOG_INFO, " %*s container %s/%s:%lli = %.2f", depth, "",
	  strEType(self->type), self->parent->hash, 
	  (long long int)self->parent->size, self->score);
  rc = TRUE;
 error:
 if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to computeContainer");
  }
  return rc;
}


/*=======================================================================
 * Function   : computeArchive
 * Description: Compute content score (from container's scores)
 * Synopsis   : int computeArchive(Archive* self)
 * Input      : ExtractTree* self
 * Output     : ??
 =======================================================================*/
int 
computeArchive(Archive* self, int depth)
{
  int rc = FALSE;
  FromAsso* asso = NULL;

  checkArchive(self);
  if (self->extractScore != -1) goto quit; // already computed
  logEmit(LOG_DEBUG, "%*s computeArchive: %s:%lli", 
	  depth, "", self->hash, self->size);

  // score = max (image's scores)
  self->extractScore = (self->imageScore > 0)?self->imageScore:0;

  // score = max (from container's scores)
  if (self->fromContainers != NULL) {
    rgRewind(self->fromContainers);
    while((asso = rgNext(self->fromContainers)) != NULL) {
      if (!computeContainer(asso->container, depth+1)) goto error;
      if (self->extractScore < asso->container->score) {
	self->extractScore = asso->container->score;
      }
    }
  }

 quit:
  logEmit(LOG_INFO, " %*s archive %s:%lli = %.2f", depth, "",
	  self->hash, self->size, self->extractScore);
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "fails to computeArchive");
  }
  return rc;
}

/*=======================================================================
 * Function   : computeExtractScore
 * Description: Compute score for each archive
 * Synopsis   : int computeExtractScore(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int 
computeExtractScore(Collection* coll)
{
  int rc = FALSE;
  ExtractTree* self = NULL;
  Archive* archive = NULL;
  AVLNode *node = NULL;

  checkCollection(coll);
  self = coll->extractTree;
  if(self == NULL) {
    logEmit(LOG_INFO, "%s", "cannot compute empty ExtractTree");
    rc = -1;
    goto error;
  }

  // already computed (and not diseased since)
  if (self->score != -1) {
    rc = TRUE;
    goto quit;
  }

  logEmit(LOG_DEBUG, "computeExtractScore: %s", coll->label);
  if (!loadCollection(coll, SERV|EXTR)) goto error;

  // copy scores from images to archives
  if (!populateExtractTree(coll)) goto error2;

  // compute archives score recursively
  self->score = 20.00;
  if (coll->archives) {
    for (node = coll->archives->head; node; node = node->next) {
      archive = (Archive*)node->item;
      if (!computeArchive(archive, 0)) goto error2;

      // do not dumb score with outdated rules, only looks for
      // final contents and images
      //       imageScore  toContainer  toContainer->type
      // IMG       x          x            x      <--
      // REC      -1          x            REC
      // CONT     -1          x            x
      // content  -1          NULL         -      <--
      if (archive->imageScore == -1 && archive->toContainer) continue;

      /*
      // do not consider archives that do not figure into the
      // extraction rule meta-data file (from md5sums file for instance)
      //                                         fromContainer
      // IMG       x          x            x      *       <--
      // content  -1          NULL         -      +       <--
      // record   -1          NULL         -      0
      if (archive->imageScore == -1 && !archive->fromContainers->nbItems)
	continue;
      */

      if (archive->extractScore > -1) {
	if (self->score > archive->extractScore) 
	  self->score = archive->extractScore;
      }
    }
  }

  rc = TRUE;
 error2:
  if (!releaseCollection(coll, SERV|EXTR)) rc = FALSE;
 quit:
 error:
 if (!rc || self->score == -1) {
    logEmit(LOG_ERR, "%s", "fails to computeExtractScore");
  }
  return rc;
}

/*=======================================================================
 * Function   : getExtractStatus
 * Description: get global status and size of archives to burn
 * Synopsis   : char* getExtractStatus(Collection* coll, 
 *                                   size_t badSize, RG* badArchives)
 * Input      : Collection* coll
 *              RG** badArchives : will be used only if already allocated
 * Output     : size_t badSize: size of total files that should be burned
 *              RG* badArchives : ring of archives that should be burned
 *              char* rc : status string or NULL on error
 * Note       : caller will have to free the status string 
 *              and the badArchives ring (but not the archives into)
 =======================================================================*/
char*
getExtractStatus(Collection* coll, off_t* badSize, RG** badArchives)
{
  char* rc = NULL;
  ExtractTree* self = NULL;
  Archive* archive = NULL;
  AVLNode *node = NULL;

  checkCollection(coll);
  if (!(self = coll->extractTree)) goto error;
  logEmit(LOG_DEBUG, "getExtractStatus: %s", coll->label);

  // already computed (and not diseased since)
  if (self->score == -1) {
    logEmit(LOG_ERR, "%s", "please call computeExtractScore first");
    goto error;
  }

  // put bad archives into a new ring
  *badSize = 0;
  for(node = coll->archives->head; node; node = node->next) {
    archive = (Archive*) node->item;
    
    // only look for content files and REC containers (uploaded files)
    //       imageScore  toContainer  toContainer->type
    // IMG       x          x            x     
    // REC      -1          x            REC    <--
    // CONT     -1          x            x
    // content  -1          NULL         -      <--
    if (archive->toContainer && archive->toContainer->type != REC) 
      continue;

    /*
    // do not consider archives that do not figure into the
    // extraction rule meta-data file (from md5sums file for instance)
    //                                         fromContainer
    // REC      -1          x            REC    0       <--
    // content  -1          NULL         -      +       <--
    // record   -1          NULL         -      0
    if (!archive->toContainer && !archive->fromContainers->nbItems)
      continue;
    */

    if (archive->extractScore < 5) {
      if (badArchives && !rgInsert(*badArchives, archive)) goto error;
      *badSize += archive->size;
    }
  }

  logEmit(LOG_INFO, "extraction score = %.2f", self->score);

  if (self->score < 0) {
    // should never be reached
    rc = createString("error in score computation"); 
    goto next;
  }
  if (self->score == 0) {
    rc = createString(_("Perenniality lost"));
    goto next;
  }
  if (self->score < coll->serverTree->scoreParam.badScore) {
    rc = createString(_("Serious risk to loose perenniality"));
    goto next;
  }
  if (self->score < 5) {
    rc = createString(_("Bad perenniality"));
    goto next;
  }
  if (self->score < 7.5) {
    rc = createString(_("Good perenniality"));
    goto next;
  }
  rc = createString(_("Very good perenniality")); 

 next:
  if (!rc) goto error;

  // display an alert message if bad score
  logEmit((self->score < 0)?LOG_ERR:
	  ((self->score == 0)?LOG_EMERG:
	   ((self->score < coll->serverTree->scoreParam.badScore)
	    ?LOG_ALERT:(self->score < 5)?LOG_CRIT:LOG_NOTICE)),
	  "%s collection: %s (%.2f)", coll->label, rc, self->score);
  if (*badSize >= MEGA) {
    logEmit(LOG_ALERT, 
	    "%s collection: %lli Mo should be burned into supports",
	    coll->label, *badSize / MEGA);
  } 
  
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "getExtractStatus fails");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "../misc/command.h"
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
 * Description: Unit test for cache module.
 * Synopsis   : ./utupgrade
 * Input      : -i mediatex.conf
 * Output     : stdout
 =======================================================================*/
int 
main(int argc, char** argv)
{
  Collection* coll = NULL;
  char* mess = 0;
  off_t badSize = 0;
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
  if (!(mess = getExtractStatus(coll, &badSize, NULL))) goto error;
  /************************************************************************/
  
  mess = destroyString(mess);
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
