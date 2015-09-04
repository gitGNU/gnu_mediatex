/*=======================================================================
 * Version: $Id: uthtml.c,v 1.4 2015/09/04 15:30:20 nroche Exp $
 * Project: MediaTeX
 * Module : html
 *
 * HTML generator

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

/*=======================================================================
 * Function   : htmlLeftPage
 * Description: 
 * Synopsis   : 
 * Input      : FILE* fd
 * Output     : TRUE on success
 =======================================================================*/
int
htmlLeftPageMiddle(FILE* fd)
{
  int rc = FALSE;

  logMain(LOG_DEBUG, "htmlLeftPageMiddle");
  
  htmlLink(fd, "tex2html2", "titles.html", "tous les documents");
  if (!fprintf(fd, "%s","\n")) goto error;
  htmlBr(fd);
  htmlBr(fd);
  htmlLink(fd, "tex2html3", "category_002.html", "média");
  if (!fprintf(fd, "%s","\n")) goto error;
  htmlBr(fd);
  if (!fprintf(fd, "%s","\n")) goto error;
  htmlUlOpen(fd);
  htmlLiOpen(fd);
  htmlLink(fd, "tex2html4", "category_001.html", "images");
  if (!fprintf(fd, "%s","\n")) goto error;
  htmlBr(fd);
  if (!fprintf(fd, "%s","\n")) goto error;
  htmlUlOpen(fd);
  htmlLiOpen(fd);
  htmlLink(fd, "tex2html5", "category_000.html", "animaux");
  if (!fprintf(fd, "%s", "\n")) goto error;
  htmlBr(fd);
  htmlLiClose(fd);
  htmlUlClose(fd);
  htmlLiClose(fd);
  htmlUlClose(fd);
  htmlBr(fd);
  if (!fprintf(fd, "%s", "\nCollaborateurs: ")) goto error;
  htmlBr(fd);
  if (!fprintf(fd, "%s", "\n")) goto error;
  htmlUlOpen(fd);
  htmlLiOpen(fd);
  htmlLink(fd, "tex2html6", "role_000.html", "dessinateur");
  if (!fprintf(fd, "%s", "\n")) goto error;
  htmlBr(fd);
  htmlLiClose(fd);
  htmlUlClose(fd);

  rc = TRUE;
 error:
  if(!rc) {
    logMain(LOG_ERR, "htmlLeftPage fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : htmlRightTail
 * Description: 
 * Synopsis   : 
 * Input      : FILE* fd
 * Output     : TRUE on success
 =======================================================================*/
int
htmlRightTail(FILE* fd)
{
  int rc = FALSE;

  logMain(LOG_DEBUG, "htmlRightTail");
  
  if (!fprintf(fd, "%s", "<P><P>")) goto error;
  htmlBr(fd);
  
  htmlBold(fd, "Document");
  if (!fprintf(fd, "%s", " : ")) goto error;
  if (!htmlCaps(fd, "coccinelle")) goto error;
  
  if (!fprintf(fd, "%s", "\n\n<P>\n")) goto error;
  if (!fprintf(fd, "%s", "(")) goto error;
  htmlLink(fd, "tex2html11", "category_000.html", "animaux");
  if (!fprintf(fd, "%s", ", ")) goto error;
  htmlLink(fd, "tex2html12", "category_001.html", "images");
  if (!fprintf(fd, "%s", 
	       ")\n"
	       "\n")) goto error;
  htmlUlOpen(fd);
  htmlLiOpen(fd);
  if (!fprintf(fd, "%s", 
	       "licence = pompé sur le net (non-free)\n")) goto error;
  htmlLiClose(fd);
  htmlUlClose(fd);
  if (!fprintf(fd, "%s",
	       "\n"
	       "<P>\n"
	       "Collaborateurs :\n"
	       "\n")) goto error;
  htmlUlOpen(fd);
  htmlLiOpen(fd);
  htmlLink(fd, "tex2html13", "role_000.html", "dessinateur");
  if (!fprintf(fd, "%s", "\n: ")) goto error;
  htmlLink(fd, "tex2html14", "human_000.html", "Gotlib");
  if (!fprintf(fd, "%s", "\n")) goto error;
  htmlLiClose(fd);
  htmlUlClose(fd);
  if (!fprintf(fd, "%s",
	       "\n"
	       "<P>\n"
	       "Enregistrements :\n"
	       "\n")) goto error;
  htmlUlOpen(fd);
  htmlLiOpen(fd);
  htmlLink(fd, "tex2html15", 
	   "https://localhost/~mdtx-hello/cgi/get.cgi?hash=365acad0657b0113990fe669972a65de&amp;size=15106", "url");

  if (!fprintf(fd, "%s", "\n")) goto error;
  htmlLink(fd, "tex2html16", 
	   "../extract/365acad0657b0113990fe669972a65de_15106.html", 
	   "(0.00)");

  if (!fprintf(fd, "%s", "\n")) goto error;
  htmlUlOpen(fd);
  htmlLiOpen(fd);
  if (!fprintf(fd, "%s","description = surf\n")) goto error;
  htmlLiClose(fd);
  htmlLiOpen(fd);
  if (!fprintf(fd, "%s", 
	       "format = PNG\n"
	       "\n")) goto error;
  htmlLiClose(fd);
  htmlUlClose(fd);
  htmlLiClose(fd);
  htmlUlClose(fd);

  rc = TRUE;
 error:
  if(!rc) {
    logMain(LOG_ERR, "htmlRightTail fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static void usage(char* programName)
{
  miscUsage(programName);
  miscOptions();
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for md5sum module
 * Synopsis   : ./utcommand -i scriptPath
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  FILE* fd = stdout;
  char* htmlPath = "misc/html.html";
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {0, 0, 0, 0}
  };
  
  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while ((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (!(fd = fopen(htmlPath, "w"))) {
    logMain(LOG_ERR, "fopen fails: %s", strerror(errno));
    goto error;
  }

  if (!htmlMainHead(fd, "Main")) goto error;
  if (!htmlLeftPageHead(fd, "index")) 
    goto error;
  if (!htmlLeftPageMiddle(fd)) goto error;
  if (!htmlLeftPageTail(fd)) goto error;
  if (!htmlRightHead(fd, "https://localhost/~mdtx-hello/")) 
    goto error;
  if (!htmlRightTail(fd)) goto error;
  if (!htmlMainTail(fd, "2013-12-17")) goto error;

  if (fclose(fd)) {
    logMain(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error;
  }
  /************************************************************************/

  rc = TRUE;
 error:
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


