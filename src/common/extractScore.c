/*=======================================================================
 * Version: $Id: extractScore.c,v 1.11 2015/08/13 21:14:33 nroche Exp $
 * Project: MediaTeX
 * Module : extractScore
 *
 * Compute scores based on extraction rules

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

int computeArchive(Archive* self, int depth);


/*=======================================================================
 * Function   : isNewIncoming
 * Description: state if an archive was newly upload
 * Synopsis   : static int isNewIncoming(Collection* coll, 
 *                                                    Archive* archive)
 * Input      : Collection* coll: to get uploadTTL parameter
 *              Archive* archive
 * Output     : TRUE on success
 =======================================================================*/
static int 
isNewIncoming(Collection* coll, Archive* archive)
{
  int rc = FALSE;

  if (!archive->uploadTime) goto end;
  if (currentTime() > archive->uploadTime + coll->serverTree->uploadTTL) {
    logCommon(LOG_WARNING, "forgotten incoming archive: %s:%lli", 
	      archive->hash, (long long int)archive->size); 
    goto end;
  }
  
  rc = TRUE;
 end:
  return rc;
}


/*=======================================================================
 * Function   : populateExtractTree
 * Description: Compute the uniq image's extract score from server.txt
 * Synopsis   : int populateExtractTree(Collection* coll)
 * Input      : ExtractTree* self = the ExtractTree to populate
 *              ServerTree* servers = the inputs
 * Output     : TRUE on success
 * Note       : This function MUST be called before the extractFile 
 *              parser
 =======================================================================*/
int 
populateExtractTree(Collection* coll)
{
  int rc = FALSE;
  ExtractTree* self = 0;
  ServerTree* serverTree = 0;
  Server* server = 0;
  Archive* archive = 0;
  Image* image = 0;
  int i = 0;

  checkCollection(coll);
  logCommon(LOG_DEBUG, "populateExtractTree: %s", coll->label);

  self = coll->extractTree;
  if(self == 0) {
    logCommon(LOG_DEBUG, "cannot populate empty ExtractTree");
    goto error;
  }
  
  serverTree = coll->serverTree;
  if(serverTree == 0) {
    logCommon(LOG_DEBUG, "%s",
 	    "cannot populate tree with an empty ServerTree");
    goto error;
  }

  // for each server
  rgRewind(serverTree->servers);
  while ((server = rgNext(serverTree->servers))) {
    
    // server->score = min (image's scores)
    server->score = -1;
    if (server->images) {
      rgRewind(server->images);
      while((image = rgNext(server->images))) {
	if (server->score == -1 || image->score < server->score) {
	  server->score = image->score;
	}
      }
    }
  }

  // for each archive related to images
  rgRewind(serverTree->archives);
  while ((archive = rgNext(serverTree->archives))) {

    // archive->imageScore = sum (image's scores)
    archive->imageScore = -1;
    if (archive->images) {
      archive->imageScore = 0;
      rgRewind(archive->images);

      logCommon(LOG_DEBUG, "local image score for %s:%lli",
	      archive->hash, archive->size);

      i=0;
      while((image = rgNext(archive->images))) {
	archive->imageScore += image->score;
	logCommon(LOG_DEBUG, "%c %5.2f", (i > 1)?'+':' ', image->score);
	++i;
      }
    }
    
    logCommon(LOG_DEBUG, "= %5.2f", archive->imageScore);

    // archive->imageScore /= minGeoDup
    archive->imageScore /= serverTree->minGeoDup;
    logCommon(LOG_DEBUG, "/ %i", serverTree->minGeoDup);

    // truncate it if more than maxScore
    if (archive->imageScore > coll->serverTree->scoreParam.maxScore) {
      archive->imageScore = coll->serverTree->scoreParam.maxScore;
      logCommon(LOG_DEBUG, "> %5.2f", coll->serverTree->scoreParam.maxScore);
    }

    logCommon(LOG_DEBUG, "-------", archive->imageScore);
    logCommon(LOG_DEBUG, "= %5.2f", archive->imageScore);
  }    

  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to populateExtractTree");
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
  Archive* archive = 0;

  checkContainer(self);
  if (self->score != -1) goto quit; // already computed
  if (self->type == INC) {
    logCommon(LOG_ERR, "internal error: INC container not expected");
    goto error;
  }

  logCommon(LOG_DEBUG, "%*s computeContainer %s/%s:%lli", 
	  depth, "", strEType(self->type),
	  self->parent->hash, (long long int)self->parent->size);

  // score = min ( content's scores )
  rgRewind(self->parents);
  while ((archive = rgNext(self->parents))) {
    if (!computeArchive(archive, depth+1)) goto error;
    if (self->score == -1 || self->score > archive->extractScore) {
      self->score = archive->extractScore;
    }
  }
  
 quit:
  logCommon(LOG_DEBUG, " %*s container %s/%s:%lli = %.2f", depth, "",
	  strEType(self->type), self->parent->hash, 
	  (long long int)self->parent->size, self->score);
  rc = TRUE;
 error:
 if (!rc) {
    logCommon(LOG_ERR, "fails to computeContainer");
  }
  return rc;
}


/*=======================================================================
 * Function   : computeArchive
 * Description: Compute content score (from container's scores)
 * Synopsis   : int computeArchive(Archive* self)
 * Input      : ExtractTree* self
 * Output     : TRUE on success
 =======================================================================*/
int 
computeArchive(Archive* self, int depth)
{
  int rc = FALSE;
  FromAsso* asso = 0;

  checkArchive(self);
  if (self->extractScore != -1) goto quit; // already computed
  logCommon(LOG_DEBUG, "%*s computeArchive: %s:%lli", 
	  depth, "", self->hash, self->size);

  // score = max (image's scores)
  self->extractScore = (self->imageScore > 0)?self->imageScore:0;

  // score = max (from container's scores)
  if (self->fromContainers) {
    rgRewind(self->fromContainers);
    while((asso = rgNext(self->fromContainers))) {
      if (!computeContainer(asso->container, depth+1)) goto error;
      if (self->extractScore < asso->container->score) {
	self->extractScore = asso->container->score;
      }
    }
  }

 quit:
  logCommon(LOG_DEBUG, " %*s archive %s:%lli = %.2f", depth, "",
	  self->hash, self->size, self->extractScore);
  rc = TRUE;
 error:
  if (!rc) {
    logCommon(LOG_ERR, "fails to computeArchive");
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
  ExtractTree* self = 0;
  Archive* archive = 0;
  AVLNode *node = 0;

  checkCollection(coll);
  self = coll->extractTree;
  if(self == 0) {
    logCommon(LOG_DEBUG, "cannot compute empty ExtractTree");
    rc = -1;
    goto error;
  }

  // already computed (and not diseased since)
  if (self->score != -1) {
    rc = TRUE;
    goto quit;
  }

  logCommon(LOG_DEBUG, "computeExtractScore: %s", coll->label);
  if (!loadCollection(coll, SERV|EXTR)) goto error;

  // copy scores from images to archives
  if (!populateExtractTree(coll)) goto error2;

  // compute archives score recursively
  self->score = 20.00;
  if (coll->archives) {
    for (node = coll->archives->head; node; node = node->next) {
      archive = (Archive*)node->item;
      if (!computeArchive(archive, 0)) goto error2;

      // new incomings are ignored into score computation
      if (isNewIncoming(coll, archive)) continue;

      // global score = min ( archive's score )
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
    logCommon(LOG_ERR, "fails to computeExtractScore");
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
 *              char* rc : status string or 0 on error
 * Note       : caller will have to free the status string 
 *              and the badArchives ring (but not the archives into)
 =======================================================================*/
char*
getExtractStatus(Collection* coll, off_t* badSize, RG** badArchives)
{
  char* rc = 0;
  ExtractTree* self = 0;
  Archive* archive = 0;
  AVLNode *node = 0;

  checkCollection(coll);
  if (!(self = coll->extractTree)) goto error;
  logCommon(LOG_DEBUG, "getExtractStatus: %s", coll->label);

  // already computed (and not diseased since)
  if (self->score == -1) {
    logCommon(LOG_ERR, "please call computeExtractScore first");
    goto error;
  }

  // put bad archives into a new ring
  *badSize = 0;
  for(node = coll->archives->head; node; node = node->next) {
    archive = (Archive*) node->item;
    
    // do not display a global bad score due to incomings
    if (isNewIncoming(coll, archive)) continue;

    if (archive->extractScore <= coll->serverTree->scoreParam.maxScore /2) {
      if (badArchives && !rgInsert(*badArchives, archive)) goto error;
      *badSize += archive->size;
    }
  }

  logCommon(LOG_DEBUG, "extraction score = %.2f", self->score);

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
  logCommon((self->score < 0)?LOG_ERR:
	  ((self->score == 0)?LOG_EMERG:
	   ((self->score < coll->serverTree->scoreParam.badScore)
	    ?LOG_ALERT:(self->score < 5)?LOG_CRIT:LOG_NOTICE)),
	  "%s collection: %s (%.2f)", coll->label, rc, self->score);
  if (*badSize >= MEGA) {
    // add \n to not disturb the progbar
    logCommon(LOG_ALERT, 
	    "%s collection: %lli Mo should be burned into supports\n",
	    coll->label, *badSize / MEGA);
  } 
  
 error:
  if (!rc) {
    logCommon(LOG_ERR, "getExtractStatus fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
