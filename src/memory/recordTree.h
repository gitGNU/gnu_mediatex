/*=======================================================================
 * Project: MediaTeX
 * Module : record tree
 *
 * Record producer interface

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

#ifndef MDTX_MEMORY_RECORD_H
#define MDTX_MEMORY_RECORD_H 1

#include "mediatex-types.h"
//#include <netinet/in.h>

// message type
typedef enum {UNKNOWN, DISK, CGI, HAVE, NOTIFY, UPLOAD} MessageType;

// type write into Record struct
typedef enum {
  UNDEF_ = 0,
  SUPPLY = 1, 
  DEMAND = 2,
  REMOVE = 4 // used to not delete from ring now but later 
  // as server threads will concurently loop on same rings
} Type;

// atomic cache information
struct Record
{
  // no identifiers (using struct address)
  Archive* archive;
  Server*  server;
  Type     type;
  time_t   date;  // if obsolete or not (so we may delete it)
  char*    extra; // path for supply and mail in fact for demand
};

// used to save or send cache contents
struct RecordTree
{
  Collection* collection;
  MessageType messageType;
  char        fingerPrint[MAX_SIZE_MD5+1]; /* server id */
  AESData     aes;
  int         doCypher;   // do AES cypher the body when serializing
  AVLTree*    records;
};

int cmpRecord(const void *p1, const void *p2);
int cmpRecordAvl(const void *p1, const void *p2);
int cmpRecordPath(const void *p1, const void *p2);
int cmpRecordPathAvl(const void *p1, const void *p2);

Record* createRecord(void);
Record* destroyRecord(Record* self);
Record* copyRecord(Record* destination, Record* source);
int serializeRecordTree(RecordTree* self, char* path, char* fingerPrint);
void logRecordTree(int logModule, int logPriority,
		   RecordTree* self, char* fingerPrint);

RecordTree* createRecordTree(void);
RecordTree* destroyRecordTree(RecordTree* self);
int serializeRecord(RecordTree* tree, Record* self);
int logRecord(int logModule, int logPriority, Record* self);

/* API */
char* strMessageType(MessageType self);
RecordType getRecordType(Record* self);
char* strRecordType(Record* self);
char* strRecordType2(RecordType type);

Record* newRecord(Server* server, Archive* archive, Type type, char* extra);
Record* addRecord(Collection* coll, Server* server, Archive* archive,
		  Type type, char* extra);
int delRecord(Collection* coll, Record* self);

int diseaseRecordTree(RecordTree* self);

#endif /* MDTX_MEMORY_RECORD_H */

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* End: */
