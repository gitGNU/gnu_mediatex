/*=======================================================================
 * Version: $Id: ardsm.h,v 1.1 2014/10/13 19:39:07 nroche Exp $
 * Project: SCOREDIT
 * Module : ardsm
 *
 * Abstract Ring Data Structure Management interface.
 * This file was originally written by Peter Felecan under GNU GPL

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Felecan Peter
 
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

#ifndef MEMORY_ARDSM_H
#define MEMORY_ARDSM_H 1

typedef struct RGIT
{   /*  RinG ITem : */
    struct RGIT* next;
    struct RGIT* prev;

    void* it;   /*  : ring's item content   */
} RGIT;

typedef struct RG
{   /*  RinG :  */
  RGIT* head;
  RGIT* curr;
  RGIT* tail;
  int nbItems;
} RG;

/* void* rgNext_r(RG* ring, RGIT** curr) is provided as a macro */
#define rgNext_r(ring, curr)				\
(ring->head?						\
 ((*curr = (*curr) ? (*curr)->next : ring->head)?	\
  (*curr)->it : NULL) : NULL)

RGIT* rgCreate(void);
void rgDestroy(RGIT* item);
void rgInit(RG* ring);
void rgDelete(RG* ring);
int rgInsert(RG* ring, void *item);
int rgInsert_r(RG* ring, void *item, RGIT** curr);
int rgHeadInsert(RG* ring, void* it);
void rgRemove(RG* ring);
void rgRemove_r(RG* ring, RGIT** curr);
void* rgCurrent(RG* ring);
void* rgPrevious(RG* ring);
void* rgNext(RG* ring);
void* rgHead(RG* ring);
void rgRewind(RG* ring);
int rgSort(RG* ring, int(*compar)(const void *, const void *));
RGIT* rgMatchItem(RG* ring, void* pItem, 
		  int(*compar)(const void *, const void *));
RGIT* rgHaveItem(RG* ring, void* pItem);
int rgDelItem(RG* ring, void* pItem);
int rgShareItems(RG* ring1, RG* ring2);
RG* rgInter(RG* ring1, RG* ring2);
RG* rgUnion(RG* ring1, RG* ring2);
RG* rgMinus(RG* ring1, RG* ring2);

RG* createRing(void);
RG* destroyOnlyRing(RG* self);
RG* destroyRing(RG* self, void*(*destroyItem)(void*));
RG* copyRing(RG* destination, RG* source, 
			 void* (*destroyItem)(void*),
			 void* (*copyItem)(void*, const void*));
inline int isEmptyRing(RG* self);

#endif /* MEMORY_ARDSM_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
