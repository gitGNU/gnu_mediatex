/*=======================================================================
 * Project: MediaTeX
 * Module : checksums
 *
 * Progression bar
 * Code copy/paste from fsck (e2fsprogs-1.41.12/e2fsck/unix.c)

 unix.c - The unix-specific code for e2fsck
 Copyright (C) 1993, 1994, 1995, 1996, 1997 Theodore Ts'o.
 This file may be redistributed under the terms of the GNU Public
 License.
 
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

#ifndef MDTX_MISC_PROGBAR_H
#define MDTX_MISC_PROGBAR_H 1

#include "mediatex-types.h" // off_t

typedef struct ProgBar {
  char bar[MISC_CHECKSUMS_PROGBARSIZE];
  char spaces[MISC_CHECKSUMS_PROGBARSIZE];

  unsigned int progress_last_percent;
  unsigned int progress_last_time;
  unsigned int progress_pos;
} ProgBar;
  
float calc_percent(off_t curr, off_t max);
void initProgBar(struct ProgBar* progbar);
void e2fsck_clear_progbar(struct ProgBar* progbar);
void e2fsck_simple_progress(struct ProgBar* progbar, const char *label, 
			    float percent);

#endif /* MDTX_MISC_PROGBAR_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
