/*=======================================================================
 * Version: $Id: utserverTree.c,v 1.7 2015/10/20 19:41:44 nroche Exp $
 * Project: MediaTeX
 * Module : serverTree

* Server producer interface

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

#include "mediatex.h"
#include "memory/utFunc.h"

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
  RG* reach = 0;
  void* it = 0;
  RG* tmp = 0;
  //RG* tmp2 = 0;
  RGIT* curr = 0;
  Server* server = 0;
  int nbReachable = 0;

  logMemory(LOG_DEBUG, "isReachable from %s to %s",
	  from->fingerPrint, to->fingerPrint);

  // networks we can reach
  if (!(reach = createRing())) goto error;
  while ((it = rgNext_r(from->networks, &curr))) {
    if (!rgInsert(reach, it)) goto error;
  }
  nbReachable = reach->nbItems;

  // loop until "to" is located under a reachable network
  /* printf("\n"); */
  while (loop) {

    /* printf("looping...\n"); */
    /* printf("we now we can reach:"); */
    /* while ((it = rgNext(reach))) printf(" %s", (char*)it); */
    /* printf ("\n"); */

    /* printf("we want to reach on of them:"); */
    /* while ((it = rgNext(to->networks))) printf(" %s", (char*)it); */
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
    curr = 0; 
    while (!loop && (server = rgNext_r(coll->serverTree->servers, &curr))) {
      if (!(tmp = rgInter(reach, server->gateways))) goto error;
      if (!isEmptyRing(tmp)) {

	/* printf("new reachable networks:"); */
	/* while ((it = rgNext(server->networks))) printf(" %s", (char*)it); */
	/* printf ("\n");      */

	// we find one so we now we can reach its networks
	tmp = destroyOnlyRing(tmp);
	if (!(tmp = rgUnion(reach, server->networks))) goto error;
	reach = destroyOnlyRing(reach);
	reach = tmp;
	tmp = 0;

	// but is there new networks ?
	loop = (reach->nbItems > nbReachable);
      }
      tmp = destroyOnlyRing(tmp);
    }
  }

  /* printf("... not reachable\n"); */
 end:
  logMemory(LOG_INFO, "%s may %sreach %s",
	  from->fingerPrint, rc?"":"not ", to->fingerPrint);
  reach = destroyOnlyRing(reach);
  return rc;
 error:
  logMemory(LOG_DEBUG, "isReachable fails");
  reach = destroyOnlyRing(reach);
  return FALSE;
}

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
  fprintf(stderr, " [ -d repository ]");

  memoryOptions();
  fprintf(stderr, "  ---\n"
	  "  -d, --input-rep\tsrcdir directory for make distcheck\n");
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
  char inputRep[256] = ".";
  Collection* coll = 0;
  Server* server1 = 0;
  Server* server2 = 0;
  Server* server3 = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MEMORY_SHORT_OPTIONS"d:";
  struct option longOptions[] = {
    MEMORY_LONG_OPTIONS,
    {"input-rep", required_argument, 0, 'd'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  env = envUnitTest;
  env.dryRun = FALSE;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'd':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, 
		"%s: nil or empty argument for the input repository\n",
		programName);
	rc = EINVAL;
	break;
      }
      strncpy(inputRep, optarg, strlen(optarg)+1);
      break; 

      GET_MEMORY_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!createExempleConfiguration(inputRep)) goto error;
  if (!(coll = getCollection("coll1"))) goto error;
  if (!createExempleServerTree(coll)) goto error;
  
  // test serializing
  if (!serializeServerTree(coll)) {
    logMemory(LOG_ERR, "sorry, cannot serialize the exemple");
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

  // test disease and log parameters
  if (!diseaseServer(coll, server1)) goto error;
  if (!diseaseServer(coll, server2)) goto error;
  if (!diseaseServer(coll, server3)) goto error;
  env.dryRun = TRUE;
  coll->serverTree->log = APACHE | AUDIT;
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

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
