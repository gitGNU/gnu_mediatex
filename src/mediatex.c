/*=======================================================================
 * Project: MediaTeX
 * Module : wrapper client software
 *
 * Client software's main function

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
#include <locale.h>

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static void 
usage(char* programName)
{
  fprintf(stderr,
	  "`" PACKAGE_NAME "' "
	  "is an Electronic Record Management System (ERMS), "
	  "focusing on the archival storage entity define by the `OAIS' "
	  "draft and on the `NF Z 42-013' requirements.\n");

  mdtxUsage(programName);
  fprintf(stderr, " [ -a ] query");

  mdtxOptions();
  fprintf(stderr, "  -a, --alone\t\tdo not make cvs remote queries\n");
  
  fprintf(stderr, "\nExamples:\n"	  
	  "\nAdmin queries:\n"
	  "  adm (init|remove|purge)  Initialise, remove and purge @mediatex software.\n"
	  "  adm (add|del) user USER  Manage publishers users.\n"
	  "  adm add coll COLL[@HOST[:PORT]]  Create/subscribe a collection.\n"
	  "  adm del coll COLL  Destroy/unsubscribe a collection.\n"

	  "\nData management:\n"
	  "  add supp SUPP on PATH  Add a new external support: a device or file not always accessible.\n"
	  "  add file PATH  Add a new external support's file: a file always accessible locally.\n"
	  "  del supp SUPP  Remove a support (external support or support's file).\n"
	  "  list supp  List supports.\n"
	  "  note supp SUPP as TEXT  Associate a short text (10 character's) with a support, which will be display on the list.\n"
	  "  check supp SUPP on PATH  Provide an external support (that is accessible now).\n"
	  "  add supp SUPP to (all|coll COLL)  Share a support with a collection.\n"
	  "  del supp SUPP from (all|coll COLL)  Withdraw a support from a collection.\n"
	  "  upload+{0,2} [file FILE as TARGET] [catalog FILE] [rules FILE] to coll COLL  Upload an incoming archive into the cache. Appending one or two '+' will run ``uprade'' or ``upgrade+make'' queries too.\n"
	  
	  "\nMeta-data management:\n"
	  "  list [master] coll  List collections.\n"	  
	  "  motd  Display actions @actorPublisherO have to perform.\n"
	  "  add key PATH to coll COLL  Subscribe a remote server to a collection.\n"
	  "  del key FINGERPRINT from coll COLL  Unsubscribe a remote server from a collection.\n"
	  "  upgrade[+] [coll COLL]  Synchronise local server. Appending one '+' will run ``make'' queries too.\n"
	  "  make [coll COLL]  Build the local HTML catalogue\n"
	  "  clean [coll COLL]  Remove the local HTML catalogue.\n"
	  "  su [coll COLL]  Change to mediatex or collection system user.\n"
	  "  audit coll COLL for MAIL  Extract all archives from a collection.\n"

	  "\nQueries to daemon:\n"
	  "  srv (save|extract|notify)  repectively ask server to dump its state into disk, perform extractions, communicate its state to other servers, deliver mails related to extracted archives (no more needed).\n"
	  "  srv [quick] scan  ask server to manage file added manually to the cache. Only check file's sizes but do not compute checksums on quick scan.\n"
	  "  srv (trim|clean|purge)  ask server remove from the cache all files that repectively can be extracted by using containers present into the cache, are safe and can be extracted locally using local supports or that are safe.\n"

	  "\nInternal/debug queries:\n"
	  "  adm (update|commit|make) [coll COLL]  Manage CVS synchronisation (already managed by ``upgrade'' query) or Build the local HTML catalogue without CVS synchronisation.\n"
	  "  adm (bind|unbind)  Manage collection repository binding on the chrooted jail for SSH remote access (already manage by mediatexd).\n"
	  "  adm mount ISO on PATH  Mount an ISO devices.\n"
	  "  adm umount PATH  Un-mount an ISO devices.\n"
	  "  adm get PATH as COLL on HASH as PATH  Retrieve a remote collection's file via SSH.\n"
	  );

  mdtxHelp();
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 2010/12/10
 * Description: Entry point for mdtx wrapper
 * Synopsis   : ./mdtx
 * Input      : stdin
 * Output     : rtfm
 =======================================================================*/
int 
main(int argc, char** argv)
{
  int uid = getuid();
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS "a";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"alone", no_argument, 0, 'a'},
    {0, 0, 0, 0}
  };

  setlocale (LC_ALL, "");
  setlocale(LC_NUMERIC, "C"); // so as printf do not write comma in float
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  // import mdtx environment
  env.noGit = FALSE;         // enable git
  env.noGitPullPush = FALSE; // enable git to reach network
  env.allocLimit *= 4;
  env.allocDiseaseCallBack = clientDiseaseAll;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'a':
      env.noGitPullPush = TRUE;
      break;
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;
 
  /************************************************************************/
  logMain(LOG_INFO, "** mdtx-wrapper **");
  if (!undoSeteuid()) goto error;

  // become mdtx user if we are not root
  if (uid && !becomeUser(env.confLabel, TRUE)) goto error;

  if (!parseShellQuery(argc, argv, optind)) goto error;
  if (!clientSaveAll()) goto error;
  if (uid && !logoutUser(uid)) rc = FALSE;
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
