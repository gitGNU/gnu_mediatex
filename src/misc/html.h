/*=======================================================================
 * Project: MediaTeX
 * Module : html
 *
 * HTML generator

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

#ifndef MDTX_MISC_HTML_H
#define MDTX_MISC_HTML_H 1

#define htmlBr(fd)				\
  if (!fprintf(fd, "%s", "\n<BR>")) goto error
#define htmlUlOpen(fd)				\
  if (!fprintf(fd, "%s", "<UL>\n")) goto error
#define htmlUlClose(fd)				\
  if (!fprintf(fd, "%s", "</UL>\n")) goto error
#define htmlLiOpen(fd)				\
  if (!fprintf(fd, "%s", "<LI>")) goto error
#define htmlLiClose(fd)				\
  if (!fprintf(fd, "%s", "</LI>\n")) goto error
#define htmlPOpen(fd)				\
  if (!fprintf(fd, "%s", "\n<P>\n")) goto error 
#define htmlPClose(fd)					\
  if (!fprintf(fd, "%s", "\n</P>\n")) goto error
#define htmlVerb(fd, text)			\
  if (!fprintf(fd, "<code>%s</code>", text)) goto error
#define htmlItalic(fd, text)			\
  if (!fprintf(fd, "\n<SPAN  CLASS=\"textit\">%s</SPAN>", text)) \
    goto error
#define htmlBold(fd, text) \
  if (!fprintf(fd, "\n<SPAN  CLASS=\"textbf\">%s</SPAN>", text)) \
    goto error

int htmlCaps(FILE* fd, char* text);

#define htmlLink(fd, name, url, text);					\
  if (name == 0) {							\
    if (!fprintf(fd, "%s", "<A ")) goto error;				\
  } else {								\
    if (!fprintf(fd, "<A NAME=\"%s\"\n", name?(char*)name:"?"))		\
      goto error;							\
  }									\
  if (!fprintf(fd, "HREF=\"%s\">%s</A>", url, text)) goto error;

#define htmlImage(fd, imgPath, imgFile, linkUrl)			\
  if (!fprintf(fd, "<A HREF=\"%s\">\n", linkUrl)) goto error;		\
  if (!fprintf(fd, "<IMG BORDER=\"0\" SRC=\"%s/%s\" ALT=\"download\"/>", \
	       imgPath, imgFile));					\
  if (!fprintf(fd, "%s", "\n</A>")) goto error;				\

int htmlMainHeadBasic(FILE* fd, char* title, char* url);
int htmlLeftPageHeadBasic(FILE* fd, char* rep, char* url);
int htmlLeftPageTail(FILE* fd);
int htmlRightHeadBasic(FILE* fd, char* masterUrl, char* url);
int htmlMainTail(FILE* fd, char* date);

#define SSI_HOME_TEMPLATE "<!--#echo var='HOME' -->"
#define htmlMainHead(fd, title) \
  htmlMainHeadBasic(fd, title, SSI_HOME_TEMPLATE)
#define htmlLeftPageHead(fd, rep) \
  htmlLeftPageHeadBasic(fd, rep, SSI_HOME_TEMPLATE)
#define htmlRightHead(fd, masterUrl) \
  htmlRightHeadBasic(fd, masterUrl, SSI_HOME_TEMPLATE)

#define htmlSSIHeader(fd, path, rep)					\
  if (!fprintf(fd, "<!--#set var='HOME' value='%s' -->\n"		\
	       "<!--#include virtual='%s/%sHeader.shtml' -->\n",	\
	       path, path, rep)) goto error;

// list include an header one level upper that includes the main header
#define htmlSSIHeader2(fd, path, rep)					\
  if (!fprintf(fd, "<!--#set var='HOME' value='../%s' -->\n"		\
	       "<!--#include virtual='%s/%sHeader.shtml' -->\n",	\
	       path, path, rep)) goto error;

#define htmlSSIFooter(fd, path) \
  if (!fprintf(fd, "\n<!--#include virtual='%s/footer.html' -->\n",	\
	       path)) goto error;

#endif /* MDTX_MISC_HTML_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
