/*=======================================================================
 * Version: $Id: threads.h,v 1.2 2014/11/13 16:37:10 nroche Exp $
 * Project: MediaTeX
 * Module : server/threads
 *
 * Handle sockets and signals

 MediaTex is an Electronic Records Management System
 Copyright (C) 2014  Nicolas Roche
 
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

#ifndef _MDTX_SERVER_THREADS_H
#define _MDTX_SERVER_THREADS_H 1

// parameter for socketJob 
typedef struct Connexion {
  int sock;
  uint32_t ipv4;
  // no use to record port here as incoming port is random
  char* host;
  Server* server;
} Connexion;

// callback functions requiered
extern int hupManager();
extern int termManager();
extern void* signalJob(void* arg);
extern void* socketJob(void* arg);

// to be called by Job callbacks when finishing
void signalJobEnds();
void socketJobEnds();

// main thread
int mainLoop();

#endif /* _MDTX_SERVER_THREADS_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
