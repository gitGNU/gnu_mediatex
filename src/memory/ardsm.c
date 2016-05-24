/*=======================================================================
 * Project: MediaTeX
 * Module : ardsm
 *
 * Abstract Ring Data Structure Management implementation.
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

#include "mediatex-config.h"


/* Item related : */

/*=======================================================================
 * Function   : rgCreate 
 * Description: Allocates memory for an item of the ring.
 * Synopsis   : RGIT* item = rgCreate(void)
 * Input      : N/A
 * Output     : RGIT *item = address of the allocated memory; nil
 *              if the allocation faled.
 =======================================================================*/
RGIT* 
rgCreate(void)
{
  RGIT *item = 0;
  
  if((item = (RGIT*)malloc(sizeof(RGIT))) == 0) {
    logMemory(LOG_ERR, "malloc: cannot allocate a ring item");
    goto error;
  }

  item->next = item->prev = (RGIT *)0;
  item->it = (void *)0;
 error:
  return(item);
}

/*=======================================================================
 * Function   : rgDestroy
 * Description: Frees the memory allocated for an item of the ring.
 * Synopsis   : void rgDestroy(RGIT* item)
 * Input      : RGIT *item = address returned by previous call to
 *              rgCreate()
 * Output     : N/A
 =======================================================================*/
void 
rgDestroy(RGIT* item)
{
  if(item) {
    free(item);
    item = 0;
  }
  
  return;
}


/* rg */

/*=======================================================================
 * Function   : rgInit
 * Description: Initialise the internals of a ring.
 * Synopsis   : void rgInit(RG* ring)
 * Input      : RG *ring = address of the ring to initialise.
 * Output     : N/A
 =======================================================================*/
void 
rgInit(RG* ring)
{
  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  /*	allocate ring, initialise :	*/
  ring->head = ring->curr = ring->tail = (RGIT *)0;
  ring->nbItems = 0;
 error:
  return;
}

/*=======================================================================
 * Function   : rgDelete
 * Description: Delete the items of the ring. Caution ! It's an
 *              effective destruction of the memory space; before you
 *              must free all the internal memory --- especially
 *              that attached to the "it" member.
 * Synopsis   : void rgDelete(RG* ring)
 * Input      : RG *ring = address of a ring;
 * Output     : N/A
 =======================================================================*/
void 
rgDelete(RG* ring)
{
  RGIT *temp = (RGIT *)0;

  if(ring == (RG *)0) goto error;

  while (ring->head != (RGIT *)0) {
    temp = ring->head->next;
    rgDestroy(ring->head);
    ring->head = temp;
  }
	
  ring->curr = ring->tail = (RGIT *)0;
  ring->nbItems = 0;
 error:
  return;
}

/*=======================================================================
 * Function   : rgInsert
 * Description: Insert a new item at the tail of the ring.
 * Synopsis   : int rgInsert(RG* ring, void* it)
 * Input      : RG *ring = address of the ring in wich to insert an item
 *              void *it = address of the item content to be inserted
 * Output     : TRUE if the insertion succeded; FALSE otherwise.
 =======================================================================*/
int 
rgInsert(RG* ring, void* it)
{
  int rc = FALSE;
  RGIT *item = 0;

  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  if ((item = rgCreate()) == 0) 
    goto error;

  item->it = it;	/*	:	content	*/
  if(ring->head == (RGIT *)0) {
    /*	first insertion :	*/
    ring->head = ring->tail = item;
  }
  else {	/*	nth insertion :	*/
    ring->tail->next = item;
    item->prev = ring->tail;
    ring->tail = item;
  }
  ring->tail->next = (RGIT *)0;
  ++(ring->nbItems);
  rc = TRUE;
 error:
  return(rc);
}

/*=======================================================================
 * Function   : rgInsert_r
 * Description: Insert a new item at the tail of the ring. (re-entrant)
 * Synopsis   : int rgInsert(RG* ring, void* it, RG** curr)
 * Input      : RG *ring = address of the ring in wich to insert an item
 *              void *it = address of the item content to be inserted
 *              RGIT** curr = the inserted item 
 * Output     : TRUE if the insertion succeded; FALSE otherwise.
 =======================================================================*/
int 
rgInsert_r(RG* ring, void* it, RGIT** curr)
{
  int rc = FALSE;

  *curr = 0;
  rc = rgInsert(ring, it);
  if (rc) *curr = ring->tail;
  return(rc);
}

/*=======================================================================
 * Function   : rgHeadInsert (ardsm) 
 * Description: Insert a new item at the head of the ring.
 * Synopsis   : int rgHeadInsert(RG* ring, void* it)
 * Input      : RG *ring = address of the ring in wich to insert an item
 *              void *it = address of the item content to be inserted
 * Output     : TRUE if the insertion succeded; FALSE otherwise.
 =======================================================================*/
int 
rgHeadInsert(RG* ring, void* it)
{
  int rc = FALSE;
  RGIT *item = 0;

  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  if ((item = rgCreate()) == 0) 
    goto error;

  item->it = it;	/*	:	content	*/
  if(ring->head == (RGIT *)0) {	
    /*	first insertion :	*/
    ring->head = ring->tail = item;
    ring->tail->next = (RGIT *)0;
  }
  else {	/*	nth insertion :	*/
    ring->head->prev = item;
    item->next = ring->head;
    ring->head = item;
  }
	
  ++ring->nbItems;
  rc = TRUE;
 error:
  return(rc);
}

/*=======================================================================
 * Function   : rgRemove
 * Description: Remove the current item from the ring.
 * Synopsis   : void rgRemove(RG* ring)
 * Input      : RG *ring = ring on wich to remove the current item.
 * Output     : N/A
 =======================================================================*/
void 
rgRemove(RG* ring)
{
  RGIT *temp = (RGIT *)0;

  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  if(ring->curr != (RGIT *)0) {
    /*	not an empty ring or not after tail :	*/
    if(ring->curr == ring->head) {	
      /*	head of ring :	*/
      if(ring->head == ring->tail) {
	/*	last element in ring :	*/
	rgDestroy(ring->curr);
	ring->head = ring->tail = ring->curr = (RGIT *)0;
      }
      else {
	temp = ring->head->next;
	rgDestroy(ring->head);
	ring->head = temp;
	ring->head->prev = (RGIT *)0;
	ring->curr = ring->head;
      }
    }
    else {
      if(ring->curr == ring->tail) {	
	/*	tail of ring :	*/
	temp = ring->tail->prev;
	rgDestroy(ring->tail);
	ring->tail = temp;
	ring->tail->next = (RGIT *)0;
	ring->curr = ring->tail;
      }
      else {
	/*	ordinary :	*/
	ring->curr->prev->next = ring->curr->next;
	ring->curr->next->prev = ring->curr->prev;
	temp = ring->curr->next;
	rgDestroy(ring->curr);
	ring->curr = temp;
      }
    }
    --(ring->nbItems);
  }
 error:	
  return;
}

/*=======================================================================
 * Function   : rgRemove_r
 * Description: Remove item from the ring (re-entrent function).
 * Synopsis   : void rgRemove_r(RG* ring, RGIT** curr)
 * Input      : RG *ring    = ring on wich to remove the current item.
 *              RGIT** curr = item to remove 
 * Output     : N/A
 =======================================================================*/
void rgRemove_r(RG* ring, RGIT** curr)
{
  RGIT *temp = (RGIT *)0;

  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  if(*curr != (RGIT *)0) {
    // not an empty ring or not after tail:
    if(*curr == ring->head) {
      // head of ring:
      if(ring->head == ring->tail) {
	// last element in ring:
	rgDestroy(*curr);
	ring->head = ring->tail = *curr = (RGIT *)0;
      }
      else {
	temp = ring->head->next;
	rgDestroy(ring->head);
	ring->head = temp;
	ring->head->prev = (RGIT *)0;
	*curr = ring->head;
      }
    }
    else {
      if(*curr == ring->tail) {
	// tail of ring:
	temp = ring->tail->prev;
	rgDestroy(ring->tail);
	ring->tail = temp;
	ring->tail->next = (RGIT *)0;
	*curr = ring->tail;
      }
      else {
	// ordinary:
	(*curr)->prev->next = (*curr)->next;
	(*curr)->next->prev = (*curr)->prev;
	temp = (*curr)->next;
	rgDestroy(*curr);
	(*curr) = temp;
      }
    }
    --(ring->nbItems);
  }
 error:
  return;
}

/*=======================================================================
 * Function   : rgCurrent
 * Description: Get the content of the current item from the ring.
 * Synopsis   : void* rgCurrent(RG* ring)
 * Input      : RG *ring   = object ring.
 * Output     : void *item = address of the next item's content in
 *                           the ring; if nil, we are at the end of
 *                           the ring
 =======================================================================*/
void* 
rgCurrent(RG* ring)
{
  void *rc = (void *)0;

  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  if(ring->curr != (RGIT *)0) {
    rc = ring->curr->it;
  } 
 error:
  return(rc);
}

/*=======================================================================
 * Function   : rgPrevious
 * Description: Get the previous item's content in the ring.
 * Synopsis   : void* rgPrevious(RG* ring)
 * Input      : RG *ring   = object ring.
 * Output     : void *item = address of the next item's content in the
 *			     ring; if nil, then we reached the end
 *			     of the ring.
 =======================================================================*/
void* 
rgPrevious(RG* ring)
{
  void *rc = (void *)0;

  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  if(ring->head != (RGIT *)0) {
    /*	not an empty ring :	*/
    if(ring->curr == (RGIT *)0) {
      /*	end of ring :	*/
      ring->curr = ring->tail;
    }
    else {
      /*	nth element	:	*/
      ring->curr = ring->curr->prev;
    }
  }
	
  if(ring->curr != (RGIT *)0) {
    rc = ring->curr->it;
  }
 error:
  return(rc);
}

/*=======================================================================
 * Function   : rgNext
 * Description: Get the next item's content in the ring.
 * Synopsis   : void* rgNext(RG* ring)
 * Input      : RG *ring   = object ring.
 * Output     : void *item = address of the next item's content in the
 *		         ring; nil, then we reached the end of the ring.
 =======================================================================*/
void* 
rgNext(RG* ring)
{
  void *rc = (void *)0;

  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  if(ring->head != (RGIT *)0) {	
    /*	not an empty ring :	*/
    if(ring->curr == (RGIT *)0)	{
      /*	begin of ring :	*/
      ring->curr = ring->head;
    }
    else {
      /*	nth element :	*/
      ring->curr = ring->curr->next;
    }
  }
	
  if(ring->curr != (RGIT *)0) {
    rc = ring->curr->it;
  }
 error:
  return(rc);
}

/* /\*======================================================================= */
/*  * Function   : rgNext_r */
/*  * Description: Same as above but reentrant */
/*  * Synopsis   : */
/*  * Input      : RG *ring   = object ring. */
/*  *              RGIT* curr = current item */
/*  * Output     : void *item */
/*  =======================================================================*\/ */
/* void* */
/* rgNext_r(RG* ring, RGIT** curr) */
/* { */
/*   void *rc = (void *)0; */
  
/*   if(ring == (RG *)0) { */
/*     logMemory(LOG_ERR, "please do not provide an empty ring"); */
/*     goto error; */
/*   } */

/*   if(ring->head == (RGIT *)0) goto error; */
/*   /\*	not an empty ring :	*\/ */

/*   if(*curr == (RGIT *)0) { */
/*     /\*	begin of ring :	*\/ */
/*     *curr = ring->head; */
/*   } */
/*   else { */
/*     /\*	nth element :	*\/ */
/*     *curr = (*curr)->next; */
/*   } */
  
/*   if(*curr != (RGIT *)0) { */
/*     rc = (*curr)->it; */
/*   } */
/*  error: */
/*   return(rc); */
/* } */

/* void* rgNext_r(RG* ring, RGIT** curr) */
/* { */
/*   if(!ring->head) return 0; */
/*   *curr = (*curr)? (*curr)->next : ring->head; */
/*   return (void*)((*curr)? (*curr)->it : 0); */
/* } */

/*=======================================================================
 * Function   : rgHead
 * Description: Return the first item (usefull for deletion)
 * Synopsis   : void* rgHead(RG* ring)
 * Input      : RG *ring   = object ring.
 * Output     : void *item
 =======================================================================*/
void*
rgHead(RG* ring)
{
  void *rc = (void *)0;
  
  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  if(ring->head != (RGIT *)0) {
    /*	not an empty ring :	*/
    ring->curr = ring->head;
    rc = ring->curr->it;
  }
 error:
  return(rc);
}

/*=======================================================================
 * Function   : rgRewind
 * Description: Rewind the ring (the next item on the ring will be the
 *		first (head of) in the ring.
 * Synopsis   : void rgRewind(RG* ring)
 * Input      : RG *ring = object ring.
 * Output     : N/A
 =======================================================================*/
void 
rgRewind(RG* ring)
{
  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  ring->curr = (RGIT *)0;
 error:	
  return;
}

/*=======================================================================
 * Function   : rgSort
 * Description: Sort the ring. Cost is 3.O(N)+O(qsort) 
 * Synopsis   : void rgSort(RG* ring, 
 *                          int(*compar)(const void *, const void *))
 * Input      : RG *ring = object ring.
 *              int(*compar)(const void *, const void *) comparaison
 *              function betwen two items
 * Output     : FALSE on allocation error
 =======================================================================*/
int 
rgSort(RG* ring, int(*compar)(const void *, const void *))
{
  int rc = FALSE;
  int nbItem = 0;	
  void** pItemArray;
  void*  pItem;
  RGIT* curr = 0;

  if(ring == (RG *)0) {	
    logMemory(LOG_ERR, "please do not provide an empty ring");
    goto error;
  }

  if(ring->nbItems > 1) {
    if ((pItemArray = (void**)malloc(ring->nbItems * sizeof(void*))) 
	== (void**)0) {
      logMemory(LOG_ERR, 
	      "malloc: cannot allocate an array to sort the ring");
      goto error;
    }

    nbItem = 0;
    curr = 0;
    while ((pItem = (void*)rgNext_r(ring, &curr))) {	
      pItemArray[nbItem++] = pItem;
    }
	  
    qsort(pItemArray, nbItem, sizeof(void*), compar);
	  
    nbItem = 0;
    curr = 0;
    while ((pItem = (void*)rgNext_r(ring, &curr))) {	
      curr->it = pItemArray[nbItem++];
    }
	    
    free(pItemArray);
  }

  ring->curr = (RGIT *)0; // rewind
  rc = TRUE;
 error:
  if (!rc) {
    logMemory(LOG_ERR, "rgSort fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : rgMatchItem
 * Description: look if an item match on the ring
 * Synopsis   : int rgMatchItem(RG* ring, void* pItem, 
 *                             int(*compar)(const void *, const void *))
 * Input      : RG* ring = ring to look into
 *              void* pItem = item to look for
 *              int(*compar)(const void *, const void *) comparaison
 *              function betwen two items
 * Output     : a pointer on RGIT or 0 if not found
 =======================================================================*/
RGIT*
rgMatchItem(RG* ring, void* pItem, 
	    int(*compar)(const void *, const void *))
{
  void* pIt = 0;
  RGIT* rc = 0;

  while ((pIt = rgNext_r(ring, &rc))) {
    if (!compar(&pIt, &pItem)) break;
  }

  return rc;
}

/*=======================================================================
 * Function   : rgHaveItem
 * Description: look if an item is on the ring
 * Synopsis   : int rgHaveItem(RG* ring, void* pItem)
 * Input      : RG* ring = ring to look into
 *              void* pItem = item to look for
 * Output     : a pointer on RGIT or 0 if not found
 =======================================================================*/
RGIT*
rgHaveItem(RG* ring, void* pItem)
{
  RGIT* rc = 0;
  void* pIt = 0;

  while ((pIt = rgNext_r(ring, &rc))) {
    if (pIt == pItem) break;
  }

  return rc;
}

/*=======================================================================
 * Function   : rgDelItem
 * Description: del an item is found on the ring 
 * Synopsis   : int rgDelItem(RG* ring, void* pItem)
 * Input      : RG* ring = ring to look into
 *              void* pItem = item to look for
 * Output     : number of items deleted: 0 or 1
 =======================================================================*/
int
rgDelItem(RG* ring, void* pItem)
{
  int rc = 0;
  RGIT* curr = 0;
  void* pIt = 0;

  while ((pIt = rgNext_r(ring, &curr))) {
    if (pIt == pItem) break;
  }

  if (pIt) {
    rgRemove_r(ring, &curr);
    rc = 1;
  }

  return rc;
}

/*=======================================================================
 * Function   : rgShareItems
 * Description: look if 2 rings have one ore more common items
 * Synopsis   : int rgShaveItems(RG* ring1, RG* ring2)
 * Input      : RG* ring1 = ring to look into
 *              RG* ring2 = ring to look into
 * Output     : Boolean = intersection of the 2 rings is not empty
 =======================================================================*/
int
rgShareItems(RG* ring1, RG* ring2)
{
  int rc = FALSE;
  RGIT* curr1 = 0;
  RGIT* curr2 = 0;
  void* pIt1 = 0;
  void* pIt2 = 0;

  if (isEmptyRing(ring1)) goto end;
  if (isEmptyRing(ring2)) goto end;
  
  // O(n2) but rings should usaly only have 1 or 2 items
  while ((pIt1 = rgNext_r(ring1, &curr1))) {
    pIt2 = 0;
    while ((pIt2 = rgNext_r(ring2, &curr2))) {
      if (pIt1 == pIt2) {
	rc = TRUE;
	goto end;
      }
    }
  }
 end:
  return rc;
}

/*=======================================================================
 * Function   : cmpPtr
 * Author     : Nicolas ROCHE
 * modif      :
 * Description: compare 2 pointers
 * Synopsis   : int cmpPtr(const void *p1, const void *p2)
 * Input      : const void *p1, const void *p2 : pointers on objects
 * Output     : like strcmp
 =======================================================================*/
int 
cmpPtr(const void *p1, const void *p2)
{
  /* p1 and p2 are pointers on &items */ 
  return (*(void**)p1 - *(void**)p2); // growing addresses
}

/*=======================================================================
 * Function   : rgInter
 * Description: 
 * Synopsis   : RG* rgInter(RG* ring1, RG* ring2)
 * Input      : RG* ring1
 *              RG* ring2
 * Output     : 0 on error
 * Note       : the items on the resulting link are not dupplicated
 =======================================================================*/
RG*
rgInter(RG* ring1, RG* ring2)
{
  RG* rc = 0;
  RGIT* curr1 = 0;
  RGIT* curr2 = 0;
  void* pIt1 = 0;
  void* pIt2 = 0;

  if (!rgSort(ring1, cmpPtr)) goto error;
  if (!rgSort(ring2, cmpPtr)) goto error;
  if (!(rc = createRing())) goto error;

  pIt1 = rgNext_r(ring1, &curr1);
  pIt2 = rgNext_r(ring2, &curr2);

  while (pIt1 && pIt2) {
    if (pIt1 == pIt2) {
      if (!rgInsert(rc, pIt1)) goto error;
      pIt1 = rgNext_r(ring1, &curr1);
      pIt2 = rgNext_r(ring2, &curr2);
      continue;
    }
    if (pIt1 < pIt2) {
      pIt1 = rgNext_r(ring1, &curr1);
      continue;
    }
    // pIt2 > pIt1
    pIt2 = rgNext_r(ring2, &curr2);
  }

  return rc;
 error:
  rc = destroyOnlyRing(rc);
  return rc;
}

/*=======================================================================
 * Function   : rgUnion
 * Description: 
 * Synopsis   : RG* rgUnion(RG* ring1, RG* ring2)
 * Input      : RG* ring1
 *              RG* ring2
 * Output     : 0 on error
 * Note       : the items on the resulting link are not dupplicated
 =======================================================================*/
RG*
rgUnion(RG* ring1, RG* ring2)
{
  RG* rc = 0;
  RGIT* curr1 = 0;
  RGIT* curr2 = 0;
  void* pIt1 = 0;
  void* pIt2 = 0;

  if (!rgSort(ring1, cmpPtr)) goto error;
  if (!rgSort(ring2, cmpPtr)) goto error;
  if (!(rc = createRing())) goto error;

  pIt1 = rgNext_r(ring1, &curr1);
  pIt2 = rgNext_r(ring2, &curr2);

  while (pIt1 && pIt2) {
    if (pIt1 == pIt2) {
      if (!rgInsert(rc, pIt1)) goto error;
      pIt1 = rgNext_r(ring1, &curr1);
      pIt2 = rgNext_r(ring2, &curr2);
      continue;
    }
    if (pIt1 < pIt2) {
      if (!rgInsert(rc, pIt1)) goto error;
      pIt1 = rgNext_r(ring1, &curr1);
      continue;
    }
    // pIt2 > pIt1
    if (!rgInsert(rc, pIt2)) goto error;
    pIt2 = rgNext_r(ring2, &curr2);
  }

  while (pIt1) {
    if (!rgInsert(rc, pIt1)) goto error;
    pIt1 = rgNext_r(ring1, &curr1);
  }
  while (pIt2) {
    if (!rgInsert(rc, pIt2)) goto error;
    pIt2 = rgNext_r(ring2, &curr2);
  }

  return rc;
 error:
  rc = destroyOnlyRing(rc);
  return rc;
}

/*=======================================================================
 * Function   : rgMinus
 * Description: 
 * Synopsis   : RG* rgMinus(RG* ring1, RG* ring2)
 * Input      : RG* ring1
 *              RG* ring2
 * Output     : 0 on error
 * Note       : the items on the resulting link are not dupplicated
 =======================================================================*/
RG*
rgMinus(RG* ring1, RG* ring2)
{
  RG* rc = 0;
  RGIT* curr1 = 0;
  RGIT* curr2 = 0;
  void* pIt1 = 0;
  void* pIt2 = 0;

  if (!rgSort(ring1, cmpPtr)) goto error;
  if (!rgSort(ring2, cmpPtr)) goto error;
  if (!(rc = createRing())) goto error;

  pIt1 = rgNext_r(ring1, &curr1);
  pIt2 = rgNext_r(ring2, &curr2);

  while (pIt1 && pIt2) {
    if (pIt1 == pIt2) {
      pIt1 = rgNext_r(ring1, &curr1);
      pIt2 = rgNext_r(ring2, &curr2);
      continue;
    }
    if (pIt1 < pIt2) {
      if (!rgInsert(rc, pIt1)) goto error;
      pIt1 = rgNext_r(ring1, &curr1);
      continue;
    }
    // pIt2 > pIt1
    pIt2 = rgNext_r(ring2, &curr2);
  }

  while (pIt1) {
    if (!rgInsert(rc, pIt1)) goto error;
    pIt1 = rgNext_r(ring1, &curr1);
  }

  return rc;
 error:
  rc = destroyOnlyRing(rc);
  return rc;
}

/*=======================================================================
 * Function   : createRing (ardsm) 
 * Description: allocate un new ring struct and initialise it.
 * Synopsis   : RG* createRing(void)
 * Input      : N/A
 * Output     : the new empty ring address
 =======================================================================*/
RG* 
createRing()
{
  RG* rc = 0;

  if ((rc = (RG*)malloc(sizeof(RG))) == 0) {
    logMemory(LOG_ERR, "malloc: cannot allocate a ring");
    goto error;
  }
							  
  memset(rc, 0, sizeof(RG));
  rgInit(rc);
 error:	
  return(rc);
}

/*=======================================================================
 * Function   : destroyOnlyRing (admsgp) 
 * Description: Destroy a ring not what it contains.
 * Synopsis   : RG* destroyOnlyRing(RG* self)
 * Input      : RG* ring to free
 * Output     : Nil address of ring
 =======================================================================*/
RG* 
destroyOnlyRing(RG* self)
{
  if (self) {
    rgDelete(self);
    free(self);
  } 
  return (RG*)0;
}

/*=======================================================================
 * Function   : destroyRing (admsgp) 
 * Description: Destroy a ring.
 * Synopsis   : RG* destroyRing(RG* self, void* (*destroyItem)(void *))
 * Input      : RG* destination
 *              RG* source
 *              void* (*destroyItem)(void *) : generic item destroyer
 * Output     : Nil address of ring
 =======================================================================*/
RG* 
destroyRing(RG* self, void* (*destroyItem)(void *))
{
  void* item = 0;
  RGIT* curr = 0;
	
  if(self) {
    //rgRewind(self);
    while ((item = (RG*)rgNext_r(self, &curr))) {
      if (destroyItem) {
	item = destroyItem(item);
      }
    }

    self = destroyOnlyRing(self);
  }
	
  return(self);
}

/*=======================================================================
 * Function   : copyRing
 * Description: Copy a source ring into a destination ring
 * Synopsis   : RG* copyRing(RG* destination, RG* source,
 *                           void* (*destroyItem)(void *),
 *                           void* (*copyItem)(void *, const void *))
 * Input      : RG* destination
 *              RG* source
 *              void* (*destroyItem)(void *) : generic item destroyer
 *              void* (*copyItem)(void *, const void *)) : generic
 *                           item copy constructor
 * Output     : RG* destination
 =======================================================================*/
RG* 
copyRing(RG* destination, RG* source, 
	 void* (*destroyItem)(void *),
	 void* (*copyItem)(void *, const void *))
{
  void* sourceItem = 0;
  void* destinationItem = 0;
  RGIT* curr = 0;

  if(source == 0) {
    logMemory(LOG_ERR, "please do not provide an empty source ring");
    goto error;
  }

  destination = destroyRing(destination, destroyItem);
  if ((destination = createRing()) == 0) {
    goto error;
  }
  
  if(source) {
    //rgRewind(source);
    while ((sourceItem = (void*)rgNext_r(source, &curr))) {
      if ((destinationItem = copyItem(destinationItem, sourceItem)) == 0) {
	goto error;
      }
      
      if (rgInsert(destination, destinationItem) == FALSE) {
	destinationItem = destroyItem(destroyItem);
	goto error;
      }
      
      destinationItem = 0;
    }
    
    destination->nbItems = source->nbItems;
  }
  
  return destination;
 error:
  logMemory(LOG_ERR, "fails to copy a ring");
  destination = destroyRing(destination, destroyItem);
  return destination;
}

/*=======================================================================
 * Function   : isEmptyString
 * Author     : Nicolas ROCHE
 * modif      :
 * Description: test if a ring is empty
 * Synopsis   : int isEmptyRing(RG* self);
 * Input      : self: ring to test
 * Output     : TRUE if empty
 =======================================================================*/
inline int isEmptyRing(RG* self)
{
  return (self == 0 || self->head == 0)?TRUE:FALSE;
}

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
