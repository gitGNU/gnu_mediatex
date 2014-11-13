/*=======================================================================
 * Version: $Id: have.c,v 1.2 2014/11/13 16:37:09 nroche Exp $
 * Project: MediaTeX
 * Module : server/have
 *
 * Manage extraction from removable device

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
#include "../misc/md5sum.h"
#include "../memory/confTree.h"
#include "../common/extractScore.h"
#include "../common/openClose.h"
#include "cache.h"
#include "deliver.h"
#include "threads.h"
#include "extract.h"

int haveArchive(ExtractData* data, Archive* archive);

/*=======================================================================
 * Function   : haveContainer
 * Description: Single container haveion
 * Synopsis   : int haveContainer
 * Input      : Collection* coll = context
 *              Container* container = what to have
 * Output     : int *found = TRUE if we ewtract something
 *              FALSE on error
 =======================================================================*/
int haveContainer(ExtractData* data, Container* container)
{
  int rc = FALSE;
  FromAsso* asso = NULL;
  AVLNode *node = NULL;
  int found = FALSE;

  checkCollection(data->coll);
  logEmit(LOG_DEBUG, "have a container %s/%s:%lli", 
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size);
 
  // look for wanted record depp into all childs
  for(node = container->childs->head; node; node = node->next) {
    asso = node->item;

    if (!haveArchive(data, asso->archive)) goto error;
    if (data->found) found = TRUE;
  }  

  data->found = found;
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "haveContainer fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : haveArchive
 * Description: Single archive haveion
 * Synopsis   : int haveArchive
 * Input      : Collection* coll = context
 *              Archive* archive = what to have
 * Output     : int *found = TRUE if haveed
 *              FALSE on error
 =======================================================================*/
int haveArchive(ExtractData* data, Archive* archive)
{
  int rc = FALSE;
  Record* record = NULL;
  RGIT* curr = NULL;
  int deliver = FALSE;

  logEmit(LOG_DEBUG, "have an archive: %s:%lli", 
	  archive->hash, archive->size);

  data->found = FALSE;
  checkCollection(data->coll);

  // exit condition
  if (archive->state == AVAILABLE) {
    data->found = TRUE;
    goto end;
  }

  // go deeper first
  if (archive->toContainer) {
    if (!haveContainer(data, archive->toContainer)) goto error;
  }

  // do not extract parent if wanted childs was already extracted
  //if (!data->found && archive->state == WANTED) {
  if (archive->state == WANTED) {

    // (same code as extractArchives)
    data->context = X_STEP;
    curr = NULL;
    while ((record = rgNext_r(archive->demands, &curr))) {
      if (getRecordType(record) == FINALE_DEMAND) {
	data->context = X_MAIN;
	break;
      }
    }

    logEmit(LOG_NOTICE, "have an archive to extract: %s:%lli", 
	    archive->hash, archive->size);
    data->target = archive;
    if (!extractArchive(data, archive))  {
      logEmit(LOG_WARNING, "%s", "need more place ?");
      goto end;
    }

    // (same code as extractArchives)
    if (data->found) {
      // adjuste to-keep date
      curr = NULL;
      while((record = rgNext_r(archive->demands, &curr)) != NULL) {
	if (!keepArchive(data->coll, archive, getRecordType(record)))
	  goto error;
	if (getRecordType(record) == FINALE_DEMAND &&
	    !(record->type & REMOVE)) deliver = TRUE;
      }

      if (deliver && !deliverMail(data->coll, archive)) goto error;
      deliver = FALSE;
    }
  }

 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "haveArchive fails");
  }
  return rc;
} 

/*=======================================================================
 * Function   : addFinalSupplies
 * Description: 
 * Synopsis   : 
 * Input      :
 * Output     : TRUE on success
 =======================================================================*/
static int 
addFinalSupplies(RecordTree* tree, Record** iso)
{
  int rc = FALSE;
  Record* record = NULL;
  RGIT* curr = NULL;
  off_t maxSize = 0;

  while((record = rgNext_r(tree->records, &curr))) {
    if (getRecordType(record) != FINALE_SUPPLY) {
      logEmit(LOG_ERR, "%s", "please provide final supplies");
      goto error;
    }

    if (!addCacheEntry(tree->collection, record)) goto error;

    // math the bigest record as the iso
    if (record->archive->size > maxSize) {
      maxSize = record->archive->size;
      *iso = record;
    }
  }
  rc = TRUE;
 error:
  return rc;
}

/*=======================================================================
 * Function   : delFinalSupplies
 * Description: 
 * Synopsis   : 
 * Input      :
 * Output     : TRUE on success
 =======================================================================*/
static int delFinalSupplies(RecordTree* tree)
{
  int rc = FALSE;
  Record* record = NULL;
  RGIT* curr = NULL;

  while((record = rgNext_r(tree->records, &curr))) {
    if (!delCacheEntry(tree->collection, record)) goto error;

    // consume records from the tree
    curr->it = NULL;
  }
  rc = TRUE;
 error:
  return rc;
}

/*=======================================================================
 * Function   : extractFinaleArchives
 * Description: Try to extract archives using the image provided
 * Synopsis   : int extractFinaleArchives(ArchiveTree* finalSupplies)
 * Input      : ArchiveTree* finalSupplies = support/colls we provide
 * Output     : TRUE on success
 =======================================================================*/
int 
extractFinaleArchives(RecordTree* recordTree, Connexion* connexion)
{
  int rc = FALSE;
  Record* iso = NULL;
  ExtractData data;
  (void) connexion;

  logEmit(LOG_DEBUG, "%s", "remote extraction");
  if (!(data.toKeeps = createRing())) goto error;

  // get the archiveTree's collection
  if ((data.coll = recordTree->collection) == NULL) {
    logEmit(LOG_ERR, "%s", "unknown collection for archiveTree");
    goto error;
  }
  
  // check we get final supplies
  if (isEmptyRing(recordTree->records)) {
    logEmit(LOG_ERR, "%s", "please provide records for have query");
    goto error;
  }

  if (!loadCollection(data.coll, SERV | EXTR | CACH)) goto error;
  if (!lockCacheRead(data.coll)) goto error2;
  if (!addFinalSupplies(recordTree, &iso)) goto error3;

  // up to down recursive match for wanted records on iso archive
  if (!haveArchive(&data, iso->archive)) goto error4;

  // if we still have room, dump the iso if it is not perenne
  if (iso->archive->fromContainers->nbItems == 0
      && iso->archive->state < AVAILABLE) {
    if (!computeExtractScore(data.coll)) goto error4;
    if (iso->archive->extractScore 
	<= 10 /*data.coll->serverTree->scoreParam.badScore*/) {
      data.target = iso->archive;
      if (!extractArchive(&data, iso->archive)) {
	logEmit(LOG_WARNING, "%s", "need more place ?");
      }
    }
  }

  rc = TRUE;
 error4:
  if (!delFinalSupplies(recordTree)) rc = FALSE;
 error3:
  if (!unLockCache(data.coll)) rc = FALSE;
 error2:
  if (!releaseCollection(data.coll, SERV | EXTR | CACH)) rc = FALSE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "remote extraction fails");
  }
  destroyOnlyRing(data.toKeeps);
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
  mdtxUsage(programName);
  fprintf(stderr, " [ -d repository ]");

  mdtxOptions();
  fprintf(stderr, "  ---\n"
	  "  -d, --input-rep\trepository with logo files\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2012/05/01
 * Description: Unit test for cache module.
 * Synopsis   : ./utcache
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char inputRep[256] = ".";
  char* inputRep2 = "/tmp"; // slash needed
  Collection* coll = NULL;
  RecordTree* tree = NULL;
  char path[1024];
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"input-rep", required_argument, NULL, 'd'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, NULL)) 
	!= EOF) {
    switch(cOption) {
      
    case 'd':
      if(optarg == NULL || *optarg == (char)0) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input repository\n",
		programName);
	rc = EINVAL;
	break;
      }
      strncpy(inputRep, optarg, strlen(optarg)+1);
      break; 
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(coll = mdtxGetCollection("coll3"))) goto error;

  utLog("%s", "Clean the cache and add part1:", NULL);
  if (!utCleanCaches()) goto error;

  if (!quickScan(coll)) goto error;
  utLog("%s", "add a demand for logo.png", NULL);
  if (!utDemand(coll, "022a34b2f9b893fba5774237e1aa80ea", 24075, 
		"test@test.com")) goto error;
  utLog("%s", "Now we have :", coll);

  utLog("%s", "provide part 1", NULL);
  sprintf(path, "%s/logoP1.cat", inputRep2);
  if (!(tree = providePart1(coll, path))) goto error;
  if (!extractFinaleArchives(tree, NULL)) goto error;
  utLog("%s", "Now we have :", coll);
  tree = destroyRecordTree(tree);

  utLog("%s", "provide part2", NULL);
  sprintf(path, "%s/logoP2.cat", inputRep2);
  if (!(tree = providePart2(coll, path))) goto error;
  if (!extractFinaleArchives(tree, NULL)) goto error;
  utLog("%s", "Now we have :", coll);
  tree = destroyRecordTree(tree);
 
  utLog("%s", "Clean the cache:", NULL);
  if (!utCleanCaches()) goto error;
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

