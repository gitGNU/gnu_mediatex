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
 -----------------------------------------------------------------------
 On Sun, May 25, 2014 at 09:28:18AM +0200, Nicolas Roche wrote:
 > Thanks for your e2fsk tools.
 > As you can see (in attachment), I grab your progression bar code from
 > e2fsk/unix.c.
 > I wonder if I can reuse your code into my personal project.
 > I cannot stat precisely under which license is it.
 > May I re-use it under a permissive license, the LGPLv2 for instance ?

 The e2fsck code is specifically licensed under the GPLv2.

 In the specific case of the progress bar, it appears that I am the
 sole author of the code, from looking at the git logs, so sure, I am
 willing to relicense e2fsck's progress bar code under the LGPLv2.

 This only applies to that code, since other parts of the e2fsprogs
 code base has contributions from other people, and I can't give
 permission for code which they may have contributed.

 Regards,
					- Ted
=======================================================================*/

#include "progbar.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h> // usleep

#define MISC_CHECKSUMS_PROGBAR_DEBUG FALSE

float 
calc_percent(off_t curr, off_t max)
{
  float	percent;
  
  if (max == 0) return 100.0;
  percent = ((float) curr) / ((float) max);
  return (percent * 100);
}

void 
initProgBar(struct ProgBar* progbar)
{
  memset(progbar->bar, '=', MISC_CHECKSUMS_PROGBARSIZE);
  memset(progbar->spaces, ' ', MISC_CHECKSUMS_PROGBARSIZE);
  progbar->bar[MISC_CHECKSUMS_PROGBARSIZE-1] = (char)0;
  progbar->spaces[MISC_CHECKSUMS_PROGBARSIZE-1] = (char)0;

  progbar->progress_last_percent = -1;
  progbar->progress_last_time = 0;
  progbar->progress_pos = 0;
}

void 
e2fsck_clear_progbar(struct ProgBar* progbar)
{
#if !MISC_CHECKSUMS_PROGBAR_DEBUG
  fprintf(stderr, "%s\r", progbar->spaces + (sizeof(progbar->spaces) - 80));
#else
  fprintf(stderr, "(clear)\n");
#endif
}

void 
e2fsck_simple_progress(struct ProgBar* progbar, const char *label, 
			    float percent)
{
  static const char spinner[] = "\\|/-";
  
  int i;
  unsigned int tick;
  struct timeval tv;
  int dpywidth;
  unsigned int fixed_percent;
  
  /*
   * Calculate the new progress position.  If the
   * percentage hasn't changed, then we skip out right
   * away.
   */
  fixed_percent = (int) ((10 * percent) + 0.5);
  if (progbar->progress_last_percent == fixed_percent) return;
  progbar->progress_last_percent = fixed_percent;
  
  /*
   * If we've already updated the spinner once within
   * the last 1/8th of a second, no point doing it
   * again.
   */
  gettimeofday(&tv, 0);
  tick = (tv.tv_sec << 3) + (tv.tv_usec / (1000000 / 8));
  if ((tick == progbar->progress_last_time) &&
      (fixed_percent) && (fixed_percent != 1000))
    return;
  progbar->progress_last_time = tick;
  
  /*
   * Advance the spinner, and note that the progress bar
   * will be on the screen
   */
  progbar->progress_pos = (progbar->progress_pos+1) & 3;

  dpywidth = 66 - strlen(label);
  dpywidth = 8 * (dpywidth / 8);
  
  i = ((percent * dpywidth) + 50) / 100;
  fprintf(stderr, "%s: |%s%s", label,
	 progbar->bar + (sizeof(progbar->bar) - (i+1)),
	 progbar->spaces + (sizeof(progbar->spaces) - (dpywidth - i + 1)));
  if (fixed_percent == 1000)
    fputc('|', stderr);
  else
    fputc(spinner[progbar->progress_pos & 3], stderr);
  fprintf(stderr, " %4.1f%%  ", percent);
#if !MISC_CHECKSUMS_PROGBAR_DEBUG
  fputs(" \r", stderr);
#else
  fputs(" \n", stderr);
#endif
  if (fixed_percent == 1000)
    e2fsck_clear_progbar(progbar);
  fflush(stderr);
  
  return;
}
