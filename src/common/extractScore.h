/*=======================================================================
 * Project: MediaTeX
 * Module : extractScore
 *
 * Compute scores based on extraction rules

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014 2015 2016 2017 Nicolas Roche
 
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

#ifndef MDTX_COMMON_EXTRACT_SCORE_H
#define MDTX_COMMON_EXTRACT_SCORE_H 1

#include "mediatex-types.h"

int computeExtractScore(Collection* coll);
char* getExtractStatus(Collection* coll, off_t* badSize, 
		       RG** badArchives);

#endif /* MDTX_COMMON_EXTRACT_SCORE_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
