/*=======================================================================
 * Version: $Id: mediatex.c,v 1.5 2015/09/22 11:42:40 nroche Exp $
 * Project: MediaTeX
 * Module : wrapper client software
 *
 * Client software's main function

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
#include "client/mediatex-client.h"
#include <locale.h>

GLOBAL_STRUCT_DEF_BIN;

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
  mdtxUsage(programName);
  fprintf(stderr, "\n\t\t[ -w ] query");
  mdtxOptions();
  fprintf(stderr, "  -a, --alone\tdo not make cvs remote queries\n");
  fprintf(stderr, "  ---\n\n"
#warning "to complete"
	  "Admin queries:\n\n"
	  "  adm (init|remove|purge)\n\n"
	  "  adm (add|del) user USER\n\n"
	  "  adm add coll COLL[@HOST[:PORT]]\n\n"
	  "  adm del coll COLL\n\n"

	  "Debugging queries:\n\n"
	  "  adm (update|commit|make) [coll COLL]\n\n"
	  "  adm (bind|unbind)\n\n"
	  "  adm mount ISO on PATH\n\n"
	  "  adm umount PATH\n\n"
	  "  adm get PATH as COLL on HASH\n\n"

	  "Queries to daemon:\n\n"
	  "  srv (save|extract|notify|deliver)\n\n"

	  "Data management:\n\n"
	  "  add supp SUPP to (all|coll COLL)\n\n"
	  "  del supp SUPP from (all|coll COLL)\n\n"
	  "  add supp SUPP on PATH\n\n"
	  "  del supp SUPP\n\n"
	  "  note supp SUPP as TEXT\n\n"
	  "  check supp SUPP on PATH\n\n"
	  "  upload PATH to coll COLL\n\n"
	  
	  "Meta-data management:\n\n"
	  "  add key PATH to coll COLL\n\n"
	  "  del key HASH from coll COLL\n\n"
	  "  list (supp|coll)\n\n"	  
	  "  motd\n\n"
	  "  (upgrade|make) [coll COLL]\n\n"
	  "  su [coll COLL]\n\n"
	  );
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
  char* options = MDTX_SHORT_OPTIONS "w";
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
  /*
  env.logFile = 0;
  env.logFacility = "file";
  env.logSeverity = "info";
  env.confFile = CONF_ETCDIR "/mediatex.conf";
  env.dryRun = FALSE;
  env.debugLexer = FALSE;
  env.debugParser = FALSE;
  env.noRegression = FALSE;
  */
  env.noCollCvs = FALSE; // enable cvs
  env.allocLimit *= 2;
  env.allocDiseaseCallBack = clientDiseaseAll;
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'a':
      env.noCollCvs = 1;
      break;
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;
 
  /************************************************************************/
  logMain(LOG_INFO, "** mdtx-wrapper **");
  if (!undo_seteuid()) goto error;

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
