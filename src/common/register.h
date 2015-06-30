/*=======================================================================
 * Version: $Id: register.h,v 1.4 2015/06/30 17:37:27 nroche Exp $
 * Project: MediaTeX
 * Module : register
 
 * Manage signals from client to server using registers

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

#ifndef MDTX_COMMON_REGISTER_H
#define MDTX_COMMON_REGISTER_H 1

#include "mediatex-types.h"

#define MDTX_SAVEMD5 0 // force daemon to write md5sum.txt file
#define MDTX_EXTRACT 1 // run extraction procedure
#define MDTX_NOTIFY  2 // run notify procedure
#define MDTX_DELIVER 3 // run extraction procedure

#define MDTX_DONE  '0'
#define MDTX_QUERY '1'
#define MDTX_ERROR '2'

#define MDTX_SHM_BUFF_SIZE 5

typedef struct ShmParam {
  int flag;
  char buf[MDTX_SHM_BUFF_SIZE+1];
} ShmParam;

void mdtxShmCopy(void *buffer, int shmSize, void* arg);
void mdtxShmRead(void *buffer, int shmSize, void* arg);
void mdtxShmDisable(void *buffer, int shmSize, void* arg);
void mdtxShmError(void *buffer, int shmSize, void* arg);
int mdtxShmInitialize();
int mdtxShmFree();

int mdtxAsyncSignal(int signal);
int mdtxSyncSignal(int flag);

#endif /* MDTX_COMMON_REGISTER_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
