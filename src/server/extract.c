/*=======================================================================
 * Project: MediaTeX
 * Module : mdtx-extract
 *
 * Manage remote extraction

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

#include "mediatex-config.h"
#include "server/mediatex-server.h"

int extractArchive(ExtractData* data, Archive* archive, int doCp);

/*=======================================================================
 * Function   : mdtxCall
 * Description: call mdtx client
 * Synopsis   : int mdtxCall()
 * Input      : N/A
 * Output     : TRUE on success
 =======================================================================*/
int 
mdtxCall(int nbArgs, ...)
{ 
  int rc = FALSE;
  va_list args;
  char *argv[] = {
    CONF_BINDIR CONF_MEDIATEXDIR,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int i = 0;

  logMain(LOG_DEBUG, "mdtxCall");
  if (nbArgs > 9) {
    logMain(LOG_ERR, "mdtxCall can only pass 9 parameters to client");
    goto error;
  }

  va_start(args, nbArgs);
  while (--nbArgs >= 0) {
    argv[++i] = va_arg(args, char *);
  }
  va_end(args);

  if (!env.noRegression && !env.dryRun) {
    if (!execScript(argv, 0, 0, TRUE)) goto error;
  }

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "mdtx client call fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getAbsoluteExtractPath
 * Description: get an absolute path to the temp extraction directory
 * Synopsis   : char* getAbsoluteExtractPath(Collection* coll, char* path) 
 * Input      : Collection* coll = the related collection
 *              char* path = the relative path
 * Output     : absolute path, 0 on failure
 =======================================================================*/
char*
getAbsoluteExtractPath(Collection* coll, char* path) 
{
  char* rc = 0;

  checkCollection(coll);
  checkLabel(path);
  logMain(LOG_DEBUG, "getAbsoluteExtractPath");

  if (!(rc = createString(coll->extractDir))) goto error;
  if (!(rc = catString(rc, "/"))) goto error;
  if (!(rc = catString(rc, path))) goto error;
  
 error:
  if (!rc) {
    logMain(LOG_ERR, "absExtractPath fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : getAbsoluteArchivePath
 * Description: Local asolute path where we can access the archive
 * Synopsis   : char* getAbsoluteArchivePath(Collection* coll, Archive* archive) 
 * Input      : Collection* coll = the related collection
 *              Archive* archive = the releted archive
 * Output     : absolute path, NULL on failure
 =======================================================================*/
char*
getAbsoluteArchivePath(Collection* coll, Archive* archive) 
{
  char* rc = 0;
  Record* record = 0;

  checkCollection(coll);
  checkArchive(archive);
  logMain(LOG_DEBUG, "getAbsoluteArchivePath");

  if (archive->state >= AVAILABLE) {
    // local supply
    if (!(record = archive->localSupply)) goto error;
    if (isEmptyString(record->extra)) goto error;
  }
  else {
    // final supply
    if (!(record = rgHead(archive->finalSupplies)) ||
	isEmptyString(record->extra)) goto error;      
  }

  rc = getAbsoluteRecordPath(coll, record);
 error:
  if (!rc) {
    logMain(LOG_ERR, "getAbsoluteArchivePath fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : extractDd
 * Description: call dd
 * Synopsis   : int extractDd(Collection* coll, char* device, 
 *                            char* target, off_t size)
 * Input      : Collection* coll
 *              char* device: source device
 *              char* target: destination path
 *              off_t size
 * Output     : TRUE on success
 =======================================================================*/
int 
extractDd(Collection* coll, char* device, char* target, off_t size)
{
  int rc = FALSE;
  char* argv[] = {"/bin/dd", 0, 0, "bs=2048", 0, 0};
  char count[MAX_SIZE_SIZE+7];

  checkCollection(coll);
  checkLabel(device);
  checkLabel(target);
  logMain(LOG_NOTICE, "do dd");
  
  if (!(argv[1] = createString("if="))
      || !(argv[1] = catString(argv[1], device)))
    goto error;

  if (!(argv[2] = createString("of="))
      || !(argv[2] = catString(argv[2], target)))
    goto error;
  
  // divide size by bs=2048 (count needed, else we get an IO error)
  sprintf(count, "count=%lli", (long long int)size>>11);
  argv[4] = count;

  if (!env.dryRun && !execScript(argv, 0, 0, FALSE)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractDd fails");
  }
  argv[1] = destroyString(argv[1]);
  argv[2] = destroyString(argv[2]);
  return rc;
}

/*=======================================================================
 * Function   : extractCp
 * Description: call cp
 * Synopsis   : int extractCp(char* source, char* target)
 * Input      : char* source: source path
 *              char* target: destination path
 * Output     : TRUE on success
 =======================================================================*/
int 
extractCp(char* source, char* target)
{
  int rc = FALSE;
  char* argv[] = {"/bin/cp", "-f", 0, 0, 0};

  checkLabel(source);
  checkLabel(target);
  logMain(LOG_NOTICE, "do cp");

  argv[2] = source;
  argv[3] = target;

  if (!env.dryRun && !execScript(argv, 0, 0, FALSE)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractCp fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : extractScp
 * Description: call scp
 * Synopsis   : int extractScp(Collection* coll, Record* record, 
 *                             char* target)
 * Input      : Collection* coll
 *              Record* record: remote source
 *              char* target: local destination path
 * Output     : TRUE on success
 =======================================================================*/
int 
extractScp(Collection* coll, Record* record, char* target)
{
  int rc = FALSE;

  checkCollection(coll);
  checkRecord(record);
  logMain(LOG_NOTICE, "do scp", record->extra);
  
  if (!env.dryRun && 
      !mdtxCall(9, "adm", "get", record->extra, 
		"as", coll->label, 
		"on", record->server->fingerPrint, 
		"as", target))
    goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractScp fails");
  } 
  return rc;
}

/*=======================================================================
 * Function   : extractIso
 * Description: call mdtx (that call mount)
 * Synopsis   : iso extractIso(Collection* coll, FromAsso* asso, 
 *                             char* targetPath)
 * Input      : Collection* coll
 *              FromAsso* asso
 *              char* targetPath = target path
 * Output     : TRUE on success
 =======================================================================*/
int 
extractIso(Collection* coll, FromAsso* asso, char* targetPath)
{
  int rc = FALSE;
  Archive* archive = (Archive*)0;
  char* iso = 0;
  char tmp[] = "mnt";
  char* tmpDir = 0;
  char* tmpFile = 0;

  logMain(LOG_NOTICE, "do iso extract");

  // should have one and only one parent
  if (!(archive = (Archive*)asso->container->parents->head->it)) 
    goto error;

  // build a mount point into temporary extraction directory
  if (!buildTargetDir(coll, &tmpDir, coll->extractDir, tmp)) goto error;
  if (!(tmpFile = createString(tmpDir))) goto error2;
  if (!(tmpFile = catString(tmpFile, asso->path))) goto error2;

  // mount the iso
  if (!(iso = getAbsoluteArchivePath(coll, archive))) goto error2;
  if (!mdtxCall(5, "adm", "mount", iso, "on", tmpDir)) 
    goto error2;

  // copy the asso file
  if (!extractCp(tmpFile, targetPath)) goto error3;
  
  rc = TRUE;
 error3:
  // umount the iso
  if (!mdtxCall(3, "adm", "umount", tmpDir)) rc = FALSE;
 error2:
  if (!removeDir(coll->extractDir, tmpFile?tmpFile:tmpDir)) rc = FALSE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "isoExtract fails");
  }
  destroyString(tmpDir);
  destroyString(tmpFile);
  destroyString(iso);
  return(rc);
}

/*=======================================================================
 * Function   : extractCat
 * Description: call cat
 * Synopsis   : int extractCat(Collection* coll, FromAsso* asso, 
 *                             char* targetPath)
 * Input      : Collection* coll
 *              FromAsso* asso
 *              char* targetPath: destination path
 * Output     : TRUE on success
 * Note       : we need to load bash in order to do the '>' redirection
 =======================================================================*/
int 
extractCat(Collection* coll, FromAsso* asso, char* targetPath)
{
  int rc = FALSE;
  char *cmd = 0;
  Archive* archive = 0;
  RGIT* curr = 0;
  char* argv[] = {"/bin/bash", "-c", 0, 0};
  char* path2 = 0;

  logMain(LOG_NOTICE, "do cat");
  if (!(cmd = createString("/bin/cat "))) goto error;
 
  // all parts
  while ((archive = rgNext_r(asso->container->parents, &curr))) {
    if (!(path2 = getAbsoluteArchivePath(coll, archive))) goto error;
    if (!(cmd = catString(cmd, path2))) goto error;
    if (!(cmd = catString(cmd, " "))) goto error;
    path2 = destroyString(path2);
  }
  
  // concatenation resulting file
  if (!(cmd = catString(cmd, ">"))) goto error;
  if (!(cmd = catString(cmd, targetPath))) goto error;
  argv[2] = cmd;

  if (!env.dryRun && !execScript(argv, 0, 0, FALSE)) goto error;

  rc = TRUE;
 error:  
  if (!rc) {
    logMain(LOG_ERR, "fails to do cat");
  }
  path2 = destroyString(path2);
  cmd = destroyString(cmd);
  return(rc);
}

/*=======================================================================
 * Function   : extractTar
 * Description: call tar -x
 * Synopsis   : int extractTar(Collection* coll, FromAsso* asso,
 *                             char* options, char* targetPath)
 * Input      : Collection* coll
 *              FromAsso* asso
 *              char* options: -zxf, -jxf or -xf
 *              char* targetPath
 * Output     : TRUE on success
 =======================================================================*/
int 
extractTar(Collection* coll, FromAsso* asso, char* options, 
	   char* targetPath)
{
  int rc = FALSE;
  char *argv[] = {"/bin/tar", "-C", 0, 0, 0, 0, 0};
  Archive* archive = 0;
  char tmp[] = "tar";
  char* tmpDir = 0;
  char* tmpFile = 0;

  logMain(LOG_NOTICE, "do tar %s", options);

  // build a temporary extraction directory
  if (!buildTargetDir(coll, &tmpDir, coll->extractDir, tmp)) goto error;
  if (!(tmpFile = createString(tmpDir))) goto error2;
  if (!(tmpFile = catString(tmpFile, asso->path))) goto error2;

  if (!env.dryRun) {
    argv[2] = tmpDir; 
    argv[3] = options;
    if (!(archive = rgHead(asso->container->parents))) goto error2;
    if (!(argv[4] = getAbsoluteArchivePath(coll, archive))) goto error2;
    argv[5] = asso->path;

    if (!execScript(argv, 0, 0, FALSE)) goto error2;

    if (rename(tmpFile, targetPath)) {
      logMain(LOG_ERR, "rename fails: %s", strerror(errno));
      goto error2;
    }
  }

  rc = TRUE;
 error2:
  if (!removeDir(coll->extractDir, tmpFile?tmpFile:tmpDir)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to do tar");
  }
  destroyString(tmpDir);
  destroyString(tmpFile);
  destroyString(argv[4]);
  return(rc);
}

/*=======================================================================
 * Function   : extractXzip
 * Description: call Xunzip
 * Synopsis   : int extractXzip(Collection* coll, FromAsso* asso,
 *                              char* bin, char* targetPath)
 * Input      : Collection* coll
 *              FromAsso* asso
 * Output     : TRUE on success
 * Note       : we need to load bash in order to do the '>'redirection
 *              use redirection so as to not delete original file and
 *              to decompress into another directory
 =======================================================================*/
int 
extractXzip(Collection* coll, FromAsso* asso, char* bin, char* targetPath)
{
  int rc = FALSE;
  char* argv[] = {"/bin/bash", "-c", 0, 0};
  char *cmd = 0;
  char* path = 0;
  Archive* archive = 0;

  logMain(LOG_NOTICE, "do %s", bin);

  if (asso->container->parents->nbItems <= 0) goto error;
  if (!(archive = (Archive*)asso->container->parents->head->it)) 
    goto error;
  if (!(path = getAbsoluteArchivePath(coll, archive))) goto error;

  if (!(cmd = createString(bin))) goto error;
  if (!(cmd = catString(cmd, path))) goto error;
  if (!(cmd = catString(cmd, " >"))) goto error;
  if (!(cmd = catString(cmd, targetPath))) goto error;
  argv[2] = cmd;

  if (!env.dryRun && !execScript(argv, 0, 0, FALSE)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to do Xzip");
  }
  path = destroyString(path);
  cmd = destroyString(cmd);
  return(rc);
}

/*=======================================================================
 * Function   : extractAfio
 * Description: call afio -i
 * Synopsis   : int extractAfio(Collection* coll, FromAsso* asso
 *                              char* targetPath)
 * Input      : Collection* coll
 *              FromAsso* asso
 *              char* targetPath
 * Output     : TRUE on success
 =======================================================================*/
int 
extractAfio(Collection* coll, FromAsso* asso, char* targetPath)
{
  int rc = FALSE;
  char *argv[] = {"/bin/afio", "-i", "-y", 0, 0, 0};
  Archive* archive = 0;
  char tmp[] = "afio";
  char* tmpDir = 0;
  char* tmpFile = 0;

  logMain(LOG_NOTICE, "do afio -i");

  // build a temporary extraction directory
  if (!buildTargetDir(coll, &tmpDir, coll->extractDir, tmp)) goto error;
  if (!(tmpFile = createString(tmpDir))) goto error2;
  if (!(tmpFile = catString(tmpFile, asso->path))) goto error2;

  if (!env.dryRun) {
    argv[3] = asso->path;
    if (!(archive = rgHead(asso->container->parents))) goto error2;
    if (!(argv[4] = getAbsoluteArchivePath(coll, archive))) goto error2;

    if (!execScript(argv, 0, tmpDir, FALSE)) goto error2;

    if (rename(tmpFile, targetPath)) {
      logMain(LOG_ERR, "rename fails: %s", strerror(errno));
      goto error2;
    }
  }

  rc = TRUE;
 error2:
  if (!removeDir(coll->extractDir, tmpFile?tmpFile:tmpDir)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to do afio");
  }
  destroyString(tmpDir);
  destroyString(tmpFile);
  argv[4] = destroyString(argv[4]);
  return(rc);
}

/*=======================================================================
 * Function   : extractCpio
 * Description: call cpio -i
 * Synopsis   : int extractCpio(Collection* coll, FromAsso* asso
 *                              char* targetPath)
 * Input      : Collection* coll
 *              FromAsso* asso
 *              char* targetPath
 * Output     : TRUE on success
 =======================================================================*/
int 
extractCpio(Collection* coll, FromAsso* asso, char* targetPath)
{
  int rc = FALSE;
  char *argv[] = {"/bin/cpio", "-i", "--file", 0, 
		  "--make-directories", 0, 0};
  Archive* archive = 0;
  char tmp[] = "cpio";
  char* tmpDir = 0;
  char* tmpFile = 0;

  logMain(LOG_NOTICE, "do cpio -i");

  // build a temporary extraction directory
  if (!buildTargetDir(coll, &tmpDir, coll->extractDir, tmp)) goto error;
  if (!(tmpFile = createString(tmpDir))) goto error2;
  if (!(tmpFile = catString(tmpFile, asso->path))) goto error2;

  if (!env.dryRun) {
    if (!(archive = rgHead(asso->container->parents))) goto error2;
    if (!(argv[3] = getAbsoluteArchivePath(coll, archive))) goto error2;
    argv[5] = asso->path;

    if (!execScript(argv, 0, tmpDir, TRUE)) goto error2;

    if (rename(tmpFile, targetPath)) {
      logMain(LOG_ERR, "rename fails: %s", strerror(errno));
      goto error2;
    }
  }

  rc = TRUE;
 error2:
  if (!removeDir(coll->extractDir, tmpFile?tmpFile:tmpDir)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to do cpio");
  }
  destroyString(tmpDir);
  destroyString(tmpFile);
  argv[3] = destroyString(argv[3]);
  return(rc);
}

/*=======================================================================
 * Function   : extractZip
 * Description: call unzip
 * Synopsis   : int extractZip(Collection* coll, FromAsso* asso
 *                             char* targetPath)
 * Input      : Collection* coll
 *              FromAsso* asso
 *              char* targetPath
 * Output     : TRUE on success
 =======================================================================*/
int 
extractZip(Collection* coll, FromAsso* asso, char* targetPath)
{
  int rc = FALSE;
  char *argv[] = {"/usr/bin/unzip", "-d", 0, 0, 0, 0};
  Archive* archive = 0;
  char tmp[] = "zip";
  char* tmpDir = 0;
  char* tmpFile = 0;

  logMain(LOG_NOTICE, "do unzip");

  // build a temporary extraction directory
  if (!buildTargetDir(coll, &tmpDir, coll->extractDir, tmp)) goto error;
  if (!(tmpFile = createString(tmpDir))) goto error2;
  if (!(tmpFile = catString(tmpFile, asso->path))) goto error2;

  if (!env.dryRun) {
    argv[2] = tmpDir; 
    if (!(archive = rgHead(asso->container->parents))) goto error2;
    if (!(argv[3] = getAbsoluteArchivePath(coll, archive))) goto error2;
    argv[4] = asso->path; 

    if (!execScript(argv, 0, 0, FALSE)) goto error2;

    if (rename(tmpFile, targetPath)) {
      logMain(LOG_ERR, "rename fails: %s", strerror(errno));
      goto error2;
    }
  }

  rc = TRUE;
 error2:
  if (!removeDir(coll->extractDir, tmpFile?tmpFile:tmpDir)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to do unzip");
  }
  destroyString(tmpDir);
  destroyString(tmpFile);
  destroyString(argv[3]);
  return(rc);
}

/*=======================================================================
 * Function   : extractRar
 * Description: call rar x
 * Synopsis   : int extractRar(Collection* coll, FromAsso* asso
 *                             char* targetPath, char* targetPath)
 * Input      : Collection* coll
 *              FromAsso* asso
 *              char* targetPath
 * Output     : TRUE on success
 =======================================================================*/
int 
extractRar(Collection* coll, FromAsso* asso, char* targetPath)
{
  int rc = FALSE;
  char *argv[] = {"/usr/bin/rar", "x", 0, 0, 0, 0};
  Archive* archive = 0;
  char tmp[] = "rar";
  char* tmpDir = 0;
  char* tmpFile = 0;

  logMain(LOG_NOTICE, "do rar x");

  // build a temporary extraction directory
  if (!buildTargetDir(coll, &tmpDir, coll->extractDir, tmp)) goto error;
  if (!(tmpFile = createString(tmpDir))) goto error2;
  if (!(tmpFile = catString(tmpFile, asso->path))) goto error2;

  if (!env.dryRun) {
    if (!(archive = rgHead(asso->container->parents))) goto error2;
    if (!(argv[2] = getAbsoluteArchivePath(coll, archive))) goto error2;
    argv[3] = asso->path; 
    argv[4] = tmpDir; 

    if (!execScript(argv, 0, 0, FALSE)) goto error2;

    if (rename(tmpFile, targetPath)) {
      logMain(LOG_ERR, "rename fails: %s", strerror(errno));
      goto error2;
    }
  }

  rc = TRUE;
 error2:
  if (!removeDir(coll->extractDir, tmpFile?tmpFile:tmpDir)) rc = FALSE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "fails to do rar e");
  }
  destroyString(argv[2]);
  destroyString(tmpDir);
  destroyString(tmpFile);
  return(rc);
}

/*=======================================================================
 * Function   : extractAddToKeep
 * Description: lock just extracted archive as it may be used for 
 *              depper extraction too
 * Synopsis   : int extractAddToKeep(ExtractData* data, Archive* archive)
 * Input      : ExtractData* data
 *              Archive* archive
 * Output     : TRUE on success
 =======================================================================*/
int
extractAddToKeep(ExtractData* data, Archive* archive)
{
  int rc = FALSE;
  Collection* coll = 0;

  coll = data->coll;
  checkCollection(coll);
  checkArchive(archive);
  logMain(LOG_DEBUG, "add to-keep on %s:%lli",   
	  archive->hash, archive->size); 

  if (!keepArchive(coll, archive)) goto error;
  if (!rgInsert(data->toKeeps, archive)) goto error;

  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractAddToKeep fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : extractDelToKeeps
 * Description: unlock archive (no more extraction using it)
 * Synopsis   : int extractDelToKeeps(Collection* coll, RG* toKeeps)
 * Input      : Collection* coll
 *              RG* toKeeps
 * Output     : TRUE on success
 =======================================================================*/
int
extractDelToKeeps(Collection* coll, RG* toKeeps)
{
  int rc = FALSE;
  Archive* archive = 0;
  RGIT* curr = 0;

  checkCollection(coll);
  logMain(LOG_DEBUG, "del toKeeps"); 

  while ((archive = rgNext_r(toKeeps, &curr))) {
    logMain(LOG_INFO, "del to-keep on %s:%lli", 
	    archive->hash, archive->size);     

    if (!unKeepArchive(coll, archive)) goto error;
  }
  
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractDelToKeeps fails");
  }
  rgDelete(toKeeps);
  return rc;
}

/*=======================================================================
 * Function   : cacheSet
 * Description: move file extracted in temporary directory into 
 *               the cache cache directory
 * Synopsis   : char* moveIntoCache(Collection* coll, char* path) 
 * Input      : Collection* coll = the related collection
 *              absoluteExtractPath = source path
 *              relativeCanonicalPath = use to compute destination
 *              char* path = the relative path from temporary 
 *               extraction directory
 * Output     : TRUE on success
 * Note       : as tar cannot rename a file, and in order not to have
 *              collisions, we extract files in a temporary place 
 *              and next rename them
 =======================================================================*/
int
cacheSet(ExtractData* data, Record* record,
	char* absoluteExtractPath, char* relativeCanonicalPath)
{
  int rc = FALSE;
  Collection* coll = 0;
  char* absoluteCachePath = 0;
  char* relativeCachePath = 0;

  coll = data->coll;
  checkCollection(coll);
  checkRecord(record);
  checkLabel(absoluteExtractPath);
  checkLabel(relativeCanonicalPath);
  logMain(LOG_DEBUG, "cacheSet");  

  if (!buildTargetFile(data->coll, &absoluteCachePath,
		       coll->cacheDir, relativeCanonicalPath)) 
    goto error;

  // move from extract dir into the cache dir
  // (rename do not honor default target acl)
  logMain(LOG_INFO, "move %s to %s", 
	  absoluteExtractPath, absoluteCachePath);
  /*
  if (rename(absoluteExtractPath, absoluteCachePath)) {
    logMain(LOG_ERR, "rename fails: %s", strerror(errno));
    goto error;
  }
  */
  if (!extractCp(absoluteExtractPath, absoluteCachePath)) goto error;
  if (unlink(absoluteExtractPath)) {
    logMain(LOG_ERR, "unlink fails: %s", strerror(errno));
    goto error;
  }

  // toggle !malloc record to local-supply...
  relativeCachePath = absoluteCachePath + strlen(coll->cacheDir) + 1;
  record->extra = destroyString(record->extra);
  if (!(record->extra = createString(relativeCachePath))) goto error;

  // ...and add a toKepp on the new extracted archive
  if (!extractAddToKeep(data, record->archive)) goto error;

  logMain(LOG_NOTICE, "%s:%lli extracted", 
	  record->archive->hash, record->archive->size);
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "cacheSet fails");
  }
  destroyString(absoluteCachePath);
  return rc;
}

/*=======================================================================
 * Function   : extractRecord
 * Description: extract a record 
 *               - cp from final supplies (ie supports)
 *               - scp from remote cache's contents
 * Synopsis   : int extractRecord(Collection* coll, Record* sourceRecord)
 * Input      : Collection* coll
 *              Record* sourceRecord
 * Output     : TRUE on success
 * Note       : extract REMOTE and FINAL supplies
 =======================================================================*/
int 
extractRecord(ExtractData* data, Record* sourceRecord)
{
  int rc = FALSE;
  Collection* coll = 0;
  char* finalSupplySource = 0;
  char* finalSupplyTarget = 0;
  char* relativeCanonicalPath = 0;
  char* absoluteExtractPath = 0;
  FromAsso* asso = 0;
  Record* targetRecord = 0;
  char* device = 0;
  int isBlockDev = FALSE;

  coll = data->coll;
  checkCollection(coll);
  checkRecord(sourceRecord);
  logMain(LOG_DEBUG, "extractRecord %s:%lli", 
	  sourceRecord->archive->hash, sourceRecord->archive->size);

  // split extra if we get a final supply
  if (getRecordType(sourceRecord) == FINAL_SUPPLY) {
    finalSupplySource = getFinalSupplyInPath(sourceRecord->extra);
    finalSupplyTarget = getFinalSupplyOutPath(sourceRecord->extra);

    if (!finalSupplySource || !finalSupplyTarget) {
      logMain(LOG_ERR, "fails to parse support paths: %s",
	      sourceRecord->extra);
      goto error;
    }
  }

  // allocate place on cache
  if (!cacheAlloc(&targetRecord, coll, sourceRecord->archive)) goto error;

  // Retieve canonical path we will try to use for extraction:
  // - final supply
  if (finalSupplyTarget) {
    relativeCanonicalPath = finalSupplyTarget;
    goto next;
  }
  // - first canonical target name (may be severals)
  if (sourceRecord->archive &&
      (asso = rgHead(sourceRecord->archive->fromContainers))) {
    relativeCanonicalPath = asso->path;
    goto next;
  }
  // - source path for top containers
  relativeCanonicalPath = sourceRecord->extra;
  
 next:
  // build destination path into temporary extraction directory
  if (!buildTargetFile(data->coll, &absoluteExtractPath,
		       coll->extractDir, relativeCanonicalPath)) 
    goto error;

  // extract record into temporary extraction directory
  switch (getRecordType(sourceRecord)) {
  case FINAL_SUPPLY:
    // may be a block device to dump
    if (!getDevice(finalSupplySource, &device)) goto error;
    if (!isBlockDevice(device, &isBlockDev)) goto error;
    if (isBlockDev) {
      if (!extractDd(coll, device, absoluteExtractPath, 
		     sourceRecord->archive->size)) goto error;
    }
    else {
      if (!extractCp(finalSupplySource, absoluteExtractPath)) goto error;
    }
    break;
  case REMOTE_SUPPLY:
    if (!extractScp(coll, sourceRecord, 
		    absoluteExtractPath)) goto error;
    break;
  default:
    logMain(LOG_ERR, "cannot extract %s record", 
	    strRecordType(sourceRecord));
    goto error;
  }
  
  // toggle !malloc record to local supply
  if (!cacheSet(data, targetRecord, 
		absoluteExtractPath, relativeCanonicalPath)) goto error;
  if (!removeDir(coll->extractDir, absoluteExtractPath)) goto error;

  targetRecord = 0;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractRecord fails");
  }
  if (targetRecord) delCacheEntry(coll, targetRecord);
  destroyString(device);
  destroyString(absoluteExtractPath);
  destroyString(finalSupplySource);
  destroyString(finalSupplyTarget);
  return rc;
}

/*=======================================================================
 * Function   : extractFromAsso
 * Description: extract a FromAss (iso, cat, tgz...)
 *              (extract from container available into the local cache)
 * Synopsis   : int extractFromAsso(Collection* coll, Record* record)
 * Input      : Collection* coll
 *              Record* record
 * Output     : TRUE on success
 * Note       : extract LOCAL supply
 =======================================================================*/
int 
extractFromAsso(ExtractData* data, FromAsso* asso)
{
  int rc = FALSE;
  Collection* coll = 0;
  Record* targetRecord = 0;
  char* absoluteExtractPath = 0;
 
  coll = data->coll;
  checkCollection(coll);
  logMain(LOG_DEBUG, "extractFromAsso %s:%lli", 
	  asso->archive->hash, asso->archive->size);

  // allocate place on cache
  if (!cacheAlloc(&targetRecord, coll, asso->archive)) goto error;

  // build destination directory into temporary extraction directory
  if (!buildTargetFile(data->coll, &absoluteExtractPath,
		       coll->extractDir, asso->path)) 
    goto error;

  // extract record into temporary extraction directory
  switch (asso->container->type) {
  case ISO:
    if (!extractIso(coll, asso, absoluteExtractPath)) goto error;
    break;
    
  case CAT:
    if (!extractCat(coll, asso, absoluteExtractPath)) goto error;
    break;
    
  case TGZ:
    if (!extractTar(coll, asso, "-zxf", absoluteExtractPath)) goto error;
    break;

  case TBZ:
    if (!extractTar(coll, asso, "-jxf", absoluteExtractPath)) goto error;
    break;

  case AFIO:
    if (!extractAfio(coll, asso, absoluteExtractPath)) goto error;
    break;

  case TAR:
    if (!extractTar(coll, asso, "-xf", absoluteExtractPath)) goto error;
    break;

  case CPIO:
    if (!extractCpio(coll, asso, absoluteExtractPath)) goto error;
    break;

  case GZIP:
    if (!extractXzip(coll, asso, "/bin/zcat ", absoluteExtractPath)) 
      goto error;
    break;

  case BZIP:
    if (!extractXzip(coll, asso, "/bin/bzcat ", absoluteExtractPath)) 
      goto error;
    break;

  case ZIP:
    if (!extractZip(coll, asso, absoluteExtractPath)) goto error;
    break;

  case RAR:
    if (!extractRar(coll, asso, absoluteExtractPath)) goto error;
    break;
    
  default:
    logMain(LOG_ERR, "container %s not manage", 
	    strEType(asso->container->type));
    goto error;
  }
  
  // toggle !malloc record to local supply
  if (!cacheSet(data, targetRecord, absoluteExtractPath, asso->path)) 
    goto error;
  if (!removeDir(coll->extractDir, absoluteExtractPath)) goto error;

  targetRecord = 0;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractFromAsso fails");
  }
  if (targetRecord) delCacheEntry(coll, targetRecord);
  destroyString(absoluteExtractPath);
  return rc;
}

/*=======================================================================
 * Function   : extractContainer
 * Description: Single container extraction
 * Synopsis   : int extractContainer
 * Input      : Collection* coll = context
 *              Container* container = what to extract
 * Output     : int data->found = TRUE if all parts are extracted
 *              FALSE on error
 * Note       : Recursive calls to extract container(s)
 =======================================================================*/
int
extractContainer(ExtractData* data, Container* container)
{
  int rc = FALSE;
  Archive* archive = 0;
  RGIT* curr = 0;
  int found = FALSE;

  checkCollection(data->coll);
  if (container->type == INC) {
    logMain(LOG_ERR, "internal error: INC container not expected");
    goto error;
  }

  logMain(LOG_DEBUG, "extract container %s/%s:%lli", 
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size);

  // we extract all parents (as only one for TGZ, several for CAT...)
  curr = 0;
  found = TRUE;
  while ((archive = rgNext_r(container->parents, &curr))) {
    if (!extractArchive(data, archive, FALSE)) goto error;
    found = found && data->found;
  }
  
  logMain(LOG_INFO, "%sfound container %s/%s:%lli", data->found?"":"not ", 
	  strEType(container->type), container->parent->hash,
	  (long long int)container->parent->size);

  // postfix: copy container parts from final supplies
  if (!found) {
    curr = 0;
    while ((archive = rgNext_r(container->parents, &curr))) {
      if (archive->state >= AVAILABLE) continue;
      if (archive->finalSupplies->nbItems == 0) continue;
      
      logMain(LOG_INFO, "extract part from support");
      if (!extractArchive(data, archive, TRUE)) goto error;
    }
  }

  data->found = found;
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "container extraction error");
    data->found = FALSE;
  }
  return rc;
} 

/*=======================================================================
 * Function   : extractArchive
 * Description: Single archive extraction
 * Synopsis   : int extractArchive(ExtractData* data, Archive* archive,
 *                                 int doCpIntoCache)
 * Input      : Collection* coll: context
 *              Archive* archive: what to extract
 *              int doCp: use by extractContainer to provides container 
 *                        parts, when not all available.
 * Output     : int data->found: TRUE if extracted
 *              int rc: FALSE on error
 * Note       : Recursive call
 =======================================================================*/
int
extractArchive(ExtractData* data, Archive* archive, int doCp)
{
  int rc = FALSE;
  Collection* coll = 0;
  int isThere = FALSE;
  FromAsso* asso = 0;
  Record* record = 0;
  char* path = 0;
  RGIT* curr = 0;
  RG* toKeepsBck = 0;
  RG* toKeepsTmp = 0;
  int toDeliver = FALSE;
  static int nbRemoteCopying = 0;

  logMain(LOG_DEBUG, "extract an archive: %s:%lli", 
	  archive->hash, archive->size);
  coll = data->coll;
  data->found = FALSE;
  checkCollection(coll);

  // deliver this archive if extraction success,
  // if it match any CGI or local user demands
  curr = 0;
  while ((record = rgNext_r(archive->demands, &curr))) {
    if (getRecordType(record) & (LOCAL_DEMAND | FINAL_DEMAND)) {
      toDeliver = TRUE;
      break;
    }
  }
  
  // local supply
  if (archive->state >= AVAILABLE) {
    
    // test if the file is really there 
    if (!(path = getAbsoluteRecordPath(coll, archive->localSupply))) 
      goto error;
    if (!callAccess(path, &isThere)) goto error;
    if (!isThere) {
      logMain(LOG_DEBUG, "no input file: %s", path);
      goto error;
    }
    if (!extractAddToKeep(data, archive)) goto error;
    data->found = TRUE;
    goto end;
  }

  // final supply
  if (archive->finalSupplies->nbItems > 0) {

    // copy archive into the cache if it helps
    if (archive->state == WANTED // localy (toDeliver) or remotely 
	|| doCp // to help next remote extractions (see extractContainer)
	|| (data->cpContext == X_DO_LOCAL_COPY // provides bad images
	    && archive->state < WANTED
	    && isBadTopContainer(coll, archive)))
      {
	curr = 0;
	while (!data->found &&
	       (record = rgNext_r(archive->finalSupplies, &curr))) {	  
	  data->found = extractRecord(data, record);
	}
      }
    data->found = TRUE; // from cache or final-supply
    goto end;
  }

  // remote supply (infixed call to scp the smallest file)
  if (data->scpContext != X_NO_REMOTE_COPY) {
    if (nbRemoteCopying >= MAX_REMOTE_COPIES) {
      logMain(LOG_WARNING,
	      "skip remote extraction as %i are already running",
	      nbRemoteCopying);
    }
    else {
      curr = 0;
      while (!data->found &&
	     (record = rgNext_r(archive->remoteSupplies, &curr))) {
	if (record->type & REMOVE) continue;

	// nat server scp from nat clients on remote demand
	if (data->scpContext == X_GW_REMOTE_COPY &&
	    !rgShareItems(coll->localhost->gateways,
			  record->server->networks))
	  continue;

	// remote copy
	++nbRemoteCopying;
	if (!extractRecord(data, record)) {
	  --nbRemoteCopying;
	  goto error;
	}
	--nbRemoteCopying;
	data->found = TRUE;
	goto end; 
      }
    }
  }

  // Recursive call: we stop on the first extraction

  // backup toKeeps
  toKeepsBck = data->toKeeps;
  if (!(data->toKeeps = createRing())) goto error;

  curr = 0;
  while (!data->found && 
	 (asso = rgNext_r(archive->fromContainers, &curr))) {
    if (!extractContainer(data, asso->container)) goto error;
  }

  // backup temporary toKeeps and restore previous context
  toKeepsTmp = data->toKeeps;
  data->toKeeps = toKeepsBck;
  
  if (data->found) {
    if (!extractFromAsso(data, asso)) goto error;
  }

  // release temporary toKeeps (recursive extraction is now done)
  if (!extractDelToKeeps(coll, toKeepsTmp)) goto error;
  
 end:
  logMain(LOG_NOTICE, "%s:%lli %sfound", 
	  archive->hash, archive->size, data->found?"":"not ");
  if (data->found && toDeliver) {
    if (!deliverArchive(coll, archive)) goto error;
  }
  rc = TRUE;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extractArchive fails");
  }
  path = destroyString(path);
  toKeepsTmp = destroyOnlyRing(toKeepsTmp);
  return rc;
} 

/*=======================================================================
 * Function   : extractArchives
 * Description: Try to extract all the archives one by one
 * Synopsis   : int extractRemoteArchives(Collection* coll)
 * Input      : Collection* coll
 * Output     : TRUE on success
 =======================================================================*/
int
extractArchives(Collection* coll)
{
  int rc = FALSE;
  int rc2 = FALSE;
  Archive* archive = 0;
  Record* record = 0;
  AVLNode* node = 0;
  RGIT* curr = 0;
  ExtractData data;

  logMain(LOG_DEBUG, "extractArchives %s collection", coll->label);
  checkCollection(coll);
  memset(&data, 0, sizeof(ExtractData));
  if (!(data.toKeeps = createRing())) goto error;
  data.cpContext = X_DO_LOCAL_COPY;
  data.coll = coll;

  if (!loadCollection(coll, SERV|EXTR|CACH)) goto error;
  if (!lockCacheRead(coll)) goto error2;

  // try to extract/deliver all demands
  for (node = coll->cacheTree->archives->head; node; node = node->next) {
    archive = node->item;
    if (isEmptyRing(archive->demands) &&
	  // candidates for (local or remote) final supplies
	!(archive->state < WANTED && isBadTopContainer(coll, archive)))
      continue;
    logMain(LOG_INFO, "looking for %s%lli",
	    archive->hash, (long long int)archive->size);
    
    // define default scp extraction context:
    // only scp to provides nat client content (not recheable by others)
    data.scpContext = X_GW_REMOTE_COPY;

    // but also do scp when archive is locally wanted...
    curr = 0;
    while ((record = rgNext_r(archive->demands, &curr))) {
      if (getRecordType(record) == FINAL_DEMAND) {
	data.scpContext = X_DO_REMOTE_COPY;
	break;
      }
    }
    
    // ...or if it is a top container having bad score,
    // not already burned locally
    if (isBadTopContainer(coll, archive) &&
	!haveRecords(archive->finalSupplies))
      data.scpContext = X_DO_REMOTE_COPY;
    
    rc2 = extractArchive(&data, archive, isBadTopContainer(coll, archive));
    if (!extractDelToKeeps(coll, data.toKeeps)) goto error3;
    
    if (!rc2) {
      logMain(LOG_WARNING, "however continue...");
      continue;
    }
  }

  rc = TRUE;
 error3:
  if (!unLockCache(coll)) rc = FALSE;
 error2:
  if (!releaseCollection(coll, SERV|EXTR|CACH)) goto error;
 error:
  if (!rc) {
    logMain(LOG_ERR, "extraction fails (internal error)");
  }
  destroyOnlyRing(data.toKeeps);
  return rc;
} 

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */
