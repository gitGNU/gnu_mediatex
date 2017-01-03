/*=======================================================================
 * Project: MediaTeX
 * Module : register
 
 * Manage signals from client to server using registers

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

#ifndef MDTX_COMMON_REGISTER_H
#define MDTX_COMMON_REGISTER_H 1

#include "mediatex-types.h"

// registers defined into share memory
#define REG_SAVEMD5   0 // force daemon to write md5sum.txt file
#define REG_EXTRACT   1 // run extraction
#define REG_NOTIFY    2 // run notify
#define REG_QUICKSCAN 3 // run (quick) scan
#define REG_SCAN      4 // run scan
#define REG_TRIM      5 // only let containers
#define REG_CLEAN     6 // remove what if safe locally
#define REG_PURGE     7 // remove what if safe
#define REG_STATUS    8 // log the memory status
#define REG_SHM_BUFF_SIZE 9

// possible values into registers
#define REG_DONE    '0'
#define REG_QUERY   '1'
#define REG_PENDING '2'
#define REG_ERROR   '3'

typedef struct ShmParam {
  int flag;
  char buf[REG_SHM_BUFF_SIZE+1];
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
