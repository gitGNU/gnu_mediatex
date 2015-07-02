/*=======================================================================
 * Version: $Id: utprogbar.c,v 1.2 2015/07/02 17:22:06 nroche Exp $
 * Project: MediaTex
 * Module : unit tests
 *
 * test for getcgivars

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

#include "mediatex.h"
GLOBAL_STRUCT_DEF;

void 
e2fsck_progress(struct ProgBar* progbar, const char *label, 
		     unsigned long max)
{
  unsigned long cur = 0;
  float percent = 0;

  progbar->progress_last_percent = -1;
  progbar->progress_last_time = 0;
  progbar->progress_pos = 0;

  for (cur=0; cur<max; ++cur) {
    percent = calc_percent(cur, max);
    e2fsck_simple_progress(progbar, label, percent);
    usleep(1000);
  }
  e2fsck_clear_progbar(progbar);
}

void 
e2fsck_progress2(struct ProgBar* progbar, const char *label, 
		     unsigned long max)
{
  unsigned long cur = 0;
  float percent = 0;
  int moduloDone = 0;
  int integer = 0;

  progbar->progress_last_percent = -1;
  progbar->progress_last_time = 0;
  progbar->progress_pos = 0;

  for (cur=0; cur<max; ++cur) {
    percent = calc_percent(cur, max);

    // Put a messages disturbing the progbar
    integer = ((int) percent);
    if ((integer % 20) == 0 && integer > moduloDone) {
      logEmit(LOG_INFO, "Message to disturb output");
      moduloDone = integer;
    }

    e2fsck_simple_progress(progbar, label, percent);
    usleep(1000);
  }
  e2fsck_clear_progbar(progbar);
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
  mdtxUsage(programName);
  mdtxOptions();
  return;
}

/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for md5sum module
 * Synopsis   : ./utmd5sum -i file
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  struct ProgBar progbar;
  char test[] = "sapin_de_noel";
  char buff[20];
  unsigned int i;
  int p;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MDTX_SHORT_OPTIONS"i:";
  struct option longOptions[] = {
    MDTX_LONG_OPTIONS,
    {"input-file", required_argument, 0, 'i'},
    {0, 0, 0, 0}
  };

  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MDTX_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  // algo init
  initProgBar(&progbar);

  // test 1
  memset(buff, 0, sizeof(buff));

  for (i=0; i<sizeof(test)-1; ++i) {
    usleep(200000);
    buff[i] = test[i];
    p = i*100/(sizeof(test)-2);
    //printf("%s %i\n", buff, p);
    e2fsck_simple_progress(&progbar, buff, p);
  }

  // test 2
  e2fsck_progress(&progbar, ":)", 1234);
  e2fsck_progress(&progbar, "coucou", 3234);
  e2fsck_progress(&progbar, "thank you very much Theodore Ts'o", 2345);
  e2fsck_progress2(&progbar, ":)", 1234);
  /************************************************************************/

  rc = TRUE;
  //error:
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
