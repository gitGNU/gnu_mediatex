/*=======================================================================
 * Version: $Id: device.c,v 1.3 2015/06/03 14:03:44 nroche Exp $
 * Project: MediaTeX
 * Module : checksums
 *
 * Device informations

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

#include "../mediatex.h"
#include "alloc.h"
#include "device.h"

#include <unistd.h>
#include <mntent.h>

/*=======================================================================
 * Function   : absolutePath
 * Description: return the absolute path (and resolv symlinks into dirname)
 * Synopsis   : char* absolutePath(char* path)
 * Input      : char* path: potential relative path
 * Output     : char*: absolute path (0 on error)

 * should work for all possible input paths:
 *   file
 *   path/
 *   path/file
 =======================================================================*/
char* 
absolutePath(char* path)
{
  char* rc = 0;
  char* pwd1 = 0;
  char *pwd2 = 0;
  char *base = 0;
  int len2 = 0, i=0;
  char car = (char)0;

  // remind the current directory for cd back into
  if ((pwd1 = getcwd(0, 0)) == 0) {
    logEmit(LOG_ERR, "getcwd: %s", strerror(errno));
    goto error;
  }
  remind(pwd1);

  // remove leading / if there
  if (path[strlen(path)-1] == '/') 
    path[strlen(path)-1] = (char)0;

  // check if path contains directories
  for (i=strlen(path); i>0 && path[i] != '/'; --i);
  if (i == 0) { // here, no directory
    pwd2 = pwd1;
    pwd1 = 0;
    base = path; // basename if relative path
  }
  else {
    car = path[i+1];
    path[i+1] = (char)0; // now path is the dirname

    // change directory to dirname
    if (chdir(path) != 0) {
      logEmit(LOG_ERR, "chdir %s: %s", path, strerror(errno));
      goto error;
    }

    // get the absolute dirname
    if ((pwd2 = getcwd(0, 0)) == 0) {
      logEmit(LOG_ERR, "getcwd: %s", strerror(errno));
      goto error;
    }
    remind(pwd2);

    // cd back to the previous current directory 
    if (chdir(pwd1) != 0) {
      logEmit(LOG_ERR, "chdir %s: %s", path, strerror(errno));
      goto error;
    }
   
    path[i+1] = car; // path rewind as it was provided
    base = path+i+1; // the basename 
    free(pwd1);
    pwd1 = 0;
  }

  // concatenate absolute dirname and the basename
  len2 = strlen(pwd2);
  if ((rc = malloc(len2 + 1 + strlen(base) + 1)) == 0) {
    logEmit(LOG_ERR, "cannot malloc absolute path: %s", strerror(errno));
    goto error;
  }

  strncpy(rc, pwd2, len2+1);
  rc[len2] = '/';
  strncpy(rc + 1 + len2, base, strlen(base)+1);

  if (!env.noRegression) {
    logEmit(LOG_INFO, "absolute path is %s", rc);
  }
  
 error:
  if (!rc) {
    if (car != (char)0) path[i+1] = car; // path rewind as it was provided
    logEmit(LOG_ERR, "fails to get absolute path for: %s", path);
  }
  free(pwd1);
  free(pwd2);
  return rc;
}

/*=======================================================================
 * Function   : symlinkTarget
 * Description: return the link target file
 * Synopsis   : symlinkTarget(char* path)
 * Input      : char* path: path to test
 * Output     : char*: path to target file
 *                     0 if not a symlink
 =======================================================================*/
char* symlinkTarget(char* path) {
  char* rc = 0;
  struct stat statBuffer;

  if (path == 0 || *path == (char)0) {
    logEmit(LOG_ERR, "%s", "please provide a path for the path");
    goto error;
  }
 
  // get file attributes
  if (lstat(path, &statBuffer)) {
    logEmit(LOG_ERR, "status error on %s: %s", path, strerror(errno));
    goto error;
  }
  
  if (!S_ISLNK(statBuffer.st_mode)) {
    if (!env.noRegression) {
      logEmit(LOG_INFO, "not a symlink: %s", path);
    }
    goto end;
  }

  /* The size of a symbolic  link  is the length of the pathname 
     it contains, without a terminating null byte. */
  if ((rc = malloc(statBuffer.st_size+1)) == 0) {
    logEmit(LOG_ERR, "cannot malloc symlink path: %s", strerror(errno));
    goto error;
  }
    
  if (readlink(path, rc, statBuffer.st_size) == -1) {
    logEmit(LOG_ERR, "cannot copy symlink path: %s", strerror(errno));
    goto error;
  }
  
  rc[statBuffer.st_size] = (char)0;
  if (!env.noRegression) {
    logEmit(LOG_INFO, "find a symlink: %s -> %s", path, rc);
  }
 end:
 error:
  return rc;
}

/*=======================================================================
 * Function   : mount2device
 * Description: find a matching device in ftab
 * Synopsis   : char* mount2device(char* absPath)
 * Input      : char* absPath: path to match 
 *              (must be absolute path to works)
 * Output     : char* : matching device path found in ftab
 * Note       : getmntent_r(): _BSD_SOURCE || _SVID_SOURCE
 =======================================================================*/
char* mount2device(char* absPath)
{
  char* rc = 0;
  FILE* mtab = 0;
  struct mntent mntEntry;
  const int buflen = 255;
  char buf[buflen];

  // loop on mtab
  if ((mtab = setmntent(MISC_CHECKSUMS_MTAB, "r")) == 0) {
    logEmit(LOG_ERR, "setmntent error on %s: %s", 
	    MISC_CHECKSUMS_MTAB, strerror(errno));
    goto error;
  }
  
  errno = 0;
  while(1) {
    if (getmntent_r(mtab, &mntEntry, buf, buflen) == 0)  {
      if (errno == 0) break;
      logEmit(LOG_ERR, "getmntent_r error: %s", strerror(errno));
      goto error;
    }

    // looking for matchin dirname
    if (!strncmp(mntEntry.mnt_dir, absPath, strlen(absPath))) {
      if ((rc = malloc(strlen(mntEntry.mnt_fsname)+1)) == 0) {
	logEmit(LOG_ERR, "cannot malloc device path: %s", strerror(errno));
	goto error;
      }

      strcpy(rc, mntEntry.mnt_fsname);
      logEmit(LOG_INFO, "found mounted device %s", rc);
      break;
    }
  }
  
  if (fclose(mtab) != 0) {
    logEmit(LOG_ERR, "fclose fails: %s", strerror(errno));
    goto error;
  }

  return rc;
 error:
  if(!env.noRegression) {
    logEmit(LOG_INFO, "cannot match a device path: %s", absPath);
  }
  return rc;
}


/*=======================================================================
 * Function   : normalizePath
 * Description: absolute path without symlink
 * Synopsis   : int normalizePath(char* inPath, char* outPath)
 * Input      : char* inPath: input path
 * Output     : char* outPath: output path
 * Output     : int : TRUE on success
 =======================================================================*/
int normalizePath(char* inPath, char** outPath)
{
  int rc = FALSE;
  char* path = 0;
  char* absPath = 0;
  char* basename = 0;
  int i = 0;
  int nbLink = 0;

  path = inPath;
  if (path == 0 || *path == (char)0) {
    logEmit(LOG_ERR, "%s", "please provide a path for the device");
    goto error;
  }

  logEmit(LOG_DEBUG, "%s", "normalizePath");
  
  do {
    if (basename) free(basename);
    
    // get absolute path
    if ((absPath = absolutePath(path)) == 0) {
      goto error;
    }
    if (path && path != inPath) free(path);

    // extract dirname
    for (i=strlen(absPath); i>0 && absPath[i] != '/'; --i);

    // look for symlink
    if ((basename = symlinkTarget(absPath)) != 0) {
      ++nbLink;

      // if an absolute symlink delete the previous path
      if (*basename == '/') *absPath = (char)0;
	
      // concatenate symlink
      absPath[i+1] = (char)0; // now absPath is the dirname
      
      if ((path = malloc(strlen(absPath) + strlen(basename) + 1))
  	  == 0) {
  	logEmit(LOG_ERR, "cannot malloc path: %s", strerror(errno));
  	goto error;
      }

      // copy dirname (if not deleted)
      strncpy(path, absPath, strlen(absPath)+1);

      // copy basename
      strncpy(path+strlen(absPath), basename, strlen(basename)+1);
      free(absPath);
    }
    else {
      path = absPath;
      absPath = 0;
    }

  } while (nbLink < MISC_CHECKSUMS_MAX_NBLINK && basename != 0);
  if (nbLink >= MISC_CHECKSUMS_MAX_NBLINK) {
    logEmit(LOG_ERR, "reach %i symlinks (is there a loop ?)",
  	    MISC_CHECKSUMS_MAX_NBLINK);
    goto error;
  }
  
  *outPath = path;
  path = 0;
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "normalizePath fails");
  }
  free(absPath);   
  free(basename);
  if (path != inPath) free(path);
  return rc;
}

/*=======================================================================
 * Function   : getDevice
 * Description: try to match path with an absolute device path
 * Synopsis   : char* getDevice(char** path, char** outPath)
 * Input      : char* inPath: input path
 * Output     : char** outPath: output path
 * Output     : int : TRUE on success
 =======================================================================*/
int getDevice(char* inPath, char** outPath)
{
  int rc = FALSE;
  char *oldPath = 0;
  char *newPath = 0;

  logEmit(LOG_DEBUG, "%s", "getDevice");
  
  oldPath = inPath;
  if (!normalizePath(oldPath, &newPath)) goto error;
  /*
  // find if one device matchs in mtab
  oldPath = newPath;
  if ((newPath = mount2device(oldPath)) != 0) {
    oldPath = newPath;
    if (!normalizePath(oldPath, &newPath)) goto error;
  } else {
    newPath = oldPath;
  }
  */

  *outPath = newPath;
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "getDevice fails");
  }
  return rc;
}

/*=======================================================================
 * Function   : isDevice
 * Description: Check if path is a block device
 * Synopsis   : int isDevice(char* path)
 * Input      : char* path: path to test
 * Output     : TRUE for block or caracter device
 =======================================================================*/
int isBlockDevice(char* path, int* isB) {
  int rc = FALSE;
  struct stat statBuffer;

  logEmit(LOG_DEBUG, "%s", "isBlockDevice");
  *isB = FALSE;

  if (path == 0 || *path == (char)0) {
    logEmit(LOG_ERR, "%s", "please provide a path for the device");
    goto error;
  }

  // get file attributes
  if (stat(path, &statBuffer)) {
    logEmit(LOG_ERR, "status error on %s: %s", path, strerror(errno));
    goto error;
  }

  rc = TRUE;
#ifndef utMAIN
  if (env.noRegression) goto end;
#endif

  if (S_ISCHR(statBuffer.st_mode)) {
    logEmit(LOG_INFO, "find character device: %s", path);
    goto end;
  }
    
  if (S_ISBLK(statBuffer.st_mode)) {
    logEmit(LOG_INFO, "find bloc device: %s", path);
    *isB = TRUE;
    goto end;
  }
  
  logEmit(LOG_INFO, "do not match a block device: %s", path);
 end:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "isBlockDevice fails");
  }
 error:
  return rc;
}

/*=======================================================================
 * Function   : getIsoSize
 * Description: read the iso size from its primary volume descriptor
 * Synopsis   : int getIsoSize(int fd, off_t *size, 
 *                    unsigned long int *count, unsigned short int *bs) 
 * Input      : int fd: file descriptor on the iso or cd device
 * Output     : off_t *size = bs * count
 *	        unsigned long int *count = Volume Space Size 
 *              unsigned short int *bs = Logical Block Size 
 *              TRUE on success
 * Note       : see http://wiki.osdev.org/ISO_9660 for the offset values
                we need this because of 0 padded at the end of the disk
 =======================================================================*/
int getIsoSize(int fd, off_t *size, 
	       unsigned long int *count, unsigned short int *bs) 
{
  int rc = 0;
  off_t offset = 0;
  char invariant[8] = ".CD001.";
  char buffer[8]    = "       "; 

  logEmit(LOG_DEBUG, "%s", "getIsoSize");
  invariant[0] = 1;
  invariant[6] = 1;
  *size = 0;
  *count = 0;
  *bs = 0;

  // Identifier: invariant on iso
  offset = (8<<12);
  if ((lseek(fd, offset, SEEK_SET)) == -1) {
    logEmit(LOG_ERR, "lseek: %s", strerror(errno));
    goto error;
  }	
  if (read(fd, buffer, 8) < 0) {
    logEmit(LOG_ERR, "read: %s", strerror(errno));
    goto error;
  }
  if (strncmp(buffer, invariant, 7)) {
    logEmit(LOG_INFO, "%s", "is not an iso");
    goto end;
  }

  // Number of Logical Blocks in which the volume is recorded.
  offset = (8<<12)+80;
#if __BYTE_ORDER == __BIG_ENDIAN
  offset +=4; // int32_LSB-MSB
#endif

  if ((lseek(fd, offset, SEEK_SET)) == -1) {
    logEmit(LOG_ERR, "lseek: %s", strerror(errno));
    goto error;
  }	
  if (read(fd, count, 4) < 0) {
    logEmit(LOG_ERR, "read: %s", strerror(errno));
    goto error;
  }
 
  // The size in bytes of a logical block.
  offset = (8<<12)+128;
#if __BYTE_ORDER == __BIG_ENDIAN
  offset +=2; // int16_LSB-MSB
#endif

  if ((lseek(fd, offset, SEEK_SET)) == -1) {
    logEmit(LOG_ERR, "lseek: %s", strerror(errno));
    goto error;
  }	
  if (read(fd, bs, 2) < 0) {
    logEmit(LOG_ERR, "read: %s", strerror(errno));
    goto error;
  }

  *size = (*count)*(*bs);
  logEmit(LOG_INFO, "iso size: %lli", *size);
 end:
  rc = TRUE;
 error:
  if (!rc) {
    logEmit(LOG_ERR, "%s", "getIsoSize fails");
  }
  return rc;
}

/************************************************************************/

#ifdef utMAIN
#include "command.h"
GLOBAL_STRUCT_DEF;

/*=======================================================================
 * Function   : usage
 * Description: Print the usage.
 * Synopsis   : static void usage(char* programName)
 * Input      : programName = the name of the program; usually
 *                                  argv[0].
 * Output     : N/A
 =======================================================================*/
static void usage(char* programName)
{
  miscUsage(programName);
  fprintf(stderr, "\n\t\t[ -i device ]");

  miscOptions();
  fprintf(stderr, "  ---\n" 
	  "  -i, --input-file\tinput device file to test\n");
  return;
}


/*=======================================================================
 * Function   : main 
 * Author     : Nicolas ROCHE
 * modif      : 
 * Description: Unit test for md5sum module
 * Synopsis   : ./utcommand -i scriptPath
 * Input      : N/A
 * Output     : N/A
 =======================================================================*/
int 
main(int argc, char** argv)
{
  char* inputPath = 0;
  char* devicePath = 0;
  int isBlockDev = FALSE;
  int fd = 0;
  off_t size = 0;
  unsigned short int bs = 0;
  unsigned long int count = 0;
  // ---
  int rc = 0;
  int cOption = EOF;
  char* programName = *argv;
  char* options = MISC_SHORT_OPTIONS"i:";
  struct option longOptions[] = {
    MISC_LONG_OPTIONS,
    {"input-file", required_argument, 0, 'i'},
    {0, 0, 0, 0}
  };
  
  // import mdtx environment
  getEnv(&env);

  // parse the command line
  while((cOption = getopt_long(argc, argv, options, longOptions, 0)) 
	!= EOF) {
    switch(cOption) {
      
    case 'i':
      if(optarg == 0 || *optarg == (char)0) {
	fprintf(stderr, "%s: nil or empty argument for the input device\n",
		programName);
	rc = EINVAL;
	break;
      }
      if ((inputPath = malloc(strlen(optarg) + 1)) == 0) {
	fprintf(stderr, "cannot malloc the input device path: %s", 
		strerror(errno));
	rc = ENOMEM;
	break;
      }
      
      strncpy(inputPath, optarg, strlen(optarg)+1);
      break;
      
      GET_MISC_OPTIONS; // generic options
    }
    if (rc) goto optError;
  }

  // export mdtx environment
  if (!setEnv(programName, &env)) goto optError;

  /************************************************************************/
  if (inputPath == 0) {
    usage(programName);
    logEmit(LOG_ERR, "%s", "Please provide an input device path");
    goto error;
  }

  if (!getDevice(inputPath, &devicePath)) goto error;
  if (!isBlockDevice(devicePath, &isBlockDev)) goto error;
  logEmit(LOG_NOTICE, "%s -> %s (is %sa block device)",
	  inputPath, devicePath, isBlockDev?"":"not ");

  if ((fd = open(devicePath, O_RDONLY)) == -1) {
    logEmit(LOG_ERR, "open: %s", strerror(errno));
    goto error;
  }
  
  // compute iso size if block device
  if (!getIsoSize(fd, &size, &count, &bs)) goto error;
  logEmit(LOG_NOTICE, "%s is %san iso",
	  devicePath, (size > 0)?"":"not ");
  if (size > 0) {
    logEmit(LOG_NOTICE, "volume size = %lu", count);
    logEmit(LOG_NOTICE, "block size = %hu", bs);
    logEmit(LOG_NOTICE, "size = %lli", (long long int)size);
  }
  /************************************************************************/

  rc = TRUE;
 error:
  free(devicePath);
  free(inputPath);
  ENDINGS;
  rc=!rc;
 optError:
  exit(rc);
}

#endif // utMAIN

/* Local Variables: */
/* mode: c */
/* mode: font-lock */
/* mode: auto-fill */
/* End: */


