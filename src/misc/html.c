/*=======================================================================
 * Version: $Id: html.c,v 1.4 2015/06/30 17:37:32 nroche Exp $
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

#include "mediatex-config.h"

/*=======================================================================
 * Function   : htmlCaps
 * Description: 
 * Synopsis   : 
 * Input      : FILE* fd
 * Output     : TRUE on success
 =======================================================================*/
int
htmlCaps(FILE* fd, char* text)
{
  int rc = FALSE;
  char* string = 0;
  int l = 0;

  if (text == 0) goto error;
  logEmit(LOG_DEBUG, "%s", "htmlCaps");

  l = strlen(text);
  if (!(string = malloc(l+1))) {
    logEmit(LOG_ERR, "%s", "malloc fails");
    goto error;
  }
  strncpy(string, text, l);
  string[l] = (char)0;
  
  while (--l >= 0) {
    string[l] = toupper(string[l]);
  }

  if (!fprintf(fd, "<SMALL>%s</SMALL>", string)) 
    goto error;

  rc = TRUE;
 error:
  if(!rc) {
    logEmit(LOG_ERR, "%s", "htmlCaps fails");
  }
  free(string);
  return rc;
}


/*=======================================================================
 * Function   : htmlMainHead
 * Description: 
 * Synopsis   : 
 * Input      : FILE* fd
 * Output     : TRUE on success
 =======================================================================*/
int
htmlMainHeadBasic(FILE* fd, char* title, char* url)
{
  int rc = FALSE;

  logEmit(LOG_DEBUG, "%s", "htmlMainHead");
  
  if (!fprintf(fd, 
	       "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.1//EN'>\n"
	       "\n"
	       "<!--Inspired from LaTeX2HTML 2008 (1.71)\n"
	       " Generated by MediaTeX software\n"
	       " Nicolas Roche -->\n"
	       "<HTML>\n"
	       "<HEAD>\n"
	       "<TITLE>%s</TITLE>\n"
	       "<META NAME='description' CONTENT='MediaTeX catalog'>\n"
	       "<META NAME='keywords' CONTENT='%s'>\n"
	       "<META NAME='resource-type' CONTENT='data files'>\n"
	       "<META NAME='distribution' CONTENT='private content'>\n"
	       "\n"
	       "<META HTTP-EQUIV='Content-Type' "
	       "CONTENT='text/html; charset=utf-8'>\n"
	       "<META NAME='Generator' CONTENT='MediaTeX v2013'>\n"
	       "<META HTTP-EQUIV='Content-Style-Type' "
	       "CONTENT='text/css'>\n"
	       "\n"
	       "<LINK REL='STYLESHEET' "
	       "HREF='%s/mediatex.css'>\n"
	       "\n"
	       "</HEAD>\n"
	       "\n"
	       "<BODY >\n"
	       "<TABLE CELLPADDING=3>\n",
	       title, title, url)) goto error;

  rc = TRUE;
 error:
  if(!rc) {
    logEmit(LOG_ERR, "%s", "htmlMainHead fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : htmlLeftPageTop
 * Description: 
 * Synopsis   : 
 * Input      : FILE* fd
 *              char* rep = relative path from HOME
 * Output     : TRUE on success
 =======================================================================*/
int
htmlLeftPageHeadBasic(FILE* fd, char* rep, char* url)
{
  int rc = FALSE;

  logEmit(LOG_DEBUG, "%s", "htmlLeftPageHead");
  
  if (!fprintf(fd,
	       "<TR><TD ALIGN='LEFT' VALIGN='TOP' WIDTH=192>"
	       "<TABLE  WIDTH='100%%'>\n"
	       "<TR><TD>\n"
	       "<A NAME='tex2html1'\n"
	       "HREF='%s/%s'><IMG\n"
	       "ALIGN='BOTTOM' BORDER='0' "
	       "SRC='%s/logo.png'\n"
	       "ALT='top'></A>\n"
	       "<P>\n",
	       url, rep, url)) goto error;

  rc = TRUE;
 error:
  if(!rc) {
    logEmit(LOG_ERR, "%s", "htmlLeftPageHead fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : htmlLeftPageTail
 * Description: 
 * Synopsis   : 
 * Input      : FILE* fd
 * Output     : TRUE on success
 =======================================================================*/
int
htmlLeftPageTail(FILE* fd)
{
  int rc = FALSE;

  logEmit(LOG_DEBUG, "%s", "htmlLeftPageTail");
  
  if (!fprintf(fd, "%s",
	       "\n"
	       "<P>\n"
	       "</TD></TR>\n"
	       "</TABLE></TD>\n"
	       )) goto error;

  rc = TRUE;
 error:
  if(!rc) {
    logEmit(LOG_ERR, "%s", "htmlLeftPageTail fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : htmlRightHeadBasic
 * Description: 
 * Synopsis   : 
 * Input      : FILE* fd
 *              char* masterUrl: viewvc only remains on master host
 * Output     : TRUE on success
 =======================================================================*/
int
htmlRightHeadBasic(FILE* fd, char* masterUrl, char* url)
{
  int rc = FALSE;

  logEmit(LOG_DEBUG, "%s", "htmlRightHead");
  
  //"<TD ALIGN='LEFT' VALIGN='TOP' WIDTH=504><TABLE  WIDTH='100%%'>\n"

  if (!fprintf(fd,
	       "<TD ALIGN='LEFT' VALIGN='TOP'><TABLE>\n"
	       "<TR><TD>\n"
	       "<TABLE CELLPADDING=3>\n"
	       "<TR><TD ALIGN='CENTER'><A NAME='tex2html7'\n"
	       "HREF='%s/index'>Index</A></TD>\n"
	       "<TD ALIGN='CENTER'><A NAME='tex2html8'\n"
	       "HREF='%s/cache'>Cache</A></TD>\n"
	       "<TD ALIGN='CENTER'><A NAME='tex2html9'\n"
	       "HREF='%s/score'>Score</A></TD>\n"
	       "<TD ALIGN='CENTER'><A NAME='tex2html10'\n"
	       "HREF='%s/cgi/viewvc.cgi'>Version</A></TD>\n"
	       "</TR>\n"
	       "</TABLE>\n"
	       "<DIV ALIGN='CENTER'>\n"
	       "\n"
	       "</DIV>\n"
	       "\n",
	       url, url, url, masterUrl)) goto error;

  rc = TRUE;
 error:
  if(!rc) {
    logEmit(LOG_ERR, "%s", "htmlRightHeadBasic fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : htmlMainTail
 * Description: 
 * Synopsis   : 
 * Input      : FILE* fd
 * Output     : TRUE on success
 =======================================================================*/
int
htmlMainTail(FILE* fd, char* date)
{
  int rc = FALSE;

  logEmit(LOG_DEBUG, "%s", "htmlMainTail");
  
  if (!fprintf(fd,
	       "</TD></TR>\n"
	       "</TABLE></TD>\n"
	       "</TR>\n"
	       "</TABLE>\n"
	       "<BR><HR>\n"
	       "<ADDRESS>%s</ADDRESS>\n"
	       "</BODY>\n"
	       "</HTML>\n\n",
	       date)) goto error;

  rc = TRUE;
 error:
  if(!rc) {
    logEmit(LOG_ERR, "%s", "htmlMainTail fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : 
 * Description: 
 * Synopsis   : 
 * Input      : FILE* fd
 * Output     : TRUE on success
 =======================================================================*/
int
truc(FILE* fd)
{
  int rc = FALSE;

  logEmit(LOG_DEBUG, "%s", "truc");
  
  goto error;

  rc = TRUE;
 error:
  if(!rc) {
    logEmit(LOG_ERR, "%s", "truc fails");
  }
  return rc;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */


