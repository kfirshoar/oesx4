/*
 * CachingFileSystem.cpp
 *
 *  Author: Netanel Zakay, HUJI, 67808  (Operating Systems 2015-2016).
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "BlockData.h"
#include "BufferManager.h"

using namespace std;

struct fuse_operations caching_oper;

static void changeRootPath(char[], const char *);

static BufferManager *bm;
const std::string SPACE = " ";
const std::string GETATTR = "getattr";
const std::string FGETATTR = "fgetattr";
const std::string ACCESS = "access";
const std::string OPEN = "open";
const std::string READ = "read";
const std::string FLUSH = "flush";
const std::string RELEASE = "release";
const std::string OPENDIR = "opendir";
const std::string READDIR = "readdir";
const std::string RELEASEDIR = "releasedir";
const std::string RENAME = "rename";
const std::string INIT = "init";
const std::string DESTROY = "destroy";
const std::string IOCTL = "ioctl";

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int caching_getattr(const char *path, struct stat *statbuf)
{
    logF(GETATTR);
    if(isLogFile(path)) { return -ENOENT;}
    int ret;
    char realPath[PATH_MAX];
    changeRootPath(realPath, path);
    return lstat(realPath, statbuf);
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int caching_fgetattr(const char *path, struct stat *statbuf,
                     struct fuse_file_info *fi)
{
    logF(GETFATTR);
    if(isLogFile(path)) { return -ENOENT;}
    if (!strcmp(path, "/"))
    {
        return caching_getattr(path, statbuf);
    }
    return fstat(fi->fh, statbuf);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int caching_access(const char *path, int mask)
{
    logF(ACCESS);
    if(isLogFile(path)) { return -ENOENT;}
    int ret;
    char rootPath[PATH_MAX];
    changeRootPath(rootPath, path);
    ret = access(rootPath, mask);
    // check for error
    return ret;
}


/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * initialize an arbitrary filehandle (fh) in the fuse_file_info 
 * structure, which will be passed to all file operations.

 * pay attention that the max allowed path is PATH_MAX (in limits.h).
 * if the path is longer, return error.

 * Changed in version 2.2
 */
int caching_open(const char *path, struct fuse_file_info *fi)
{
    logF(OPEN);
    if(isLogFile(path)) { return -ENOENT;}
    int ret;
    int fileDes;
    char rootPath[PATH_MAX];
    changeRootPath(rootPath, path);
    fileDes = open(rootPath, fi->flags)
    if (fileDes < 0)
    {
        // error
    }
    fi->fh = fileDes;
    return ret;
}


/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error. For example, if you receive size=100, offest=0,
 * but the size of the file is 10, you will init only the first 
   ten bytes in the buff and return the number 10.
   
   In order to read a file from the disk, 
   we strongly advise you to use "pread" rather than "read".
   Pay attention, in pread the offset is valid as long it is 
   a multipication of the block size.
   More specifically, pread returns 0 for negative offset 
   and an offset after the end of the file
   (as long as the the rest of the requirements are fulfiiled).
   You are suppose to preserve this behavior also in your implementation.

 * Changed in version 2.2
 */
int caching_read(const char *path, char *buf, size_t size,
                 off_t offset, struct fuse_file_info *fi)
{
    logF(READ);
    if(isLogFile(path)) { return -ENOENT;}
    int readCount = 0;
    unsigned int iCur = offset / bm->blockSize;
    while (size > 0)
    {
        BlockData *bCur = bm->getBlock(path, iCur);
        if (bCur == nullptr)
        {
            int readSize = std::min(bm->blockSize, size);
            char *tmpBuf = new char[readSize];
            int realRead = pread(fi->fh, tmpBuf, readSize, offset);
            bCur = new BlockData(iCur, realRead, std::string(path), tmpBuf);
            bm->insertBlock(bCur);
        }
        int readSize = std::min(bCur->bSize, size);
        strncpy(buf, bCur->data, readSize);
        readCount += readSize;
        buf += readSize;
        size -= readSize;
        if (size == 0 || bCur->bSize < bm->blockSize)
        {
            return readCount;
        }
        iCur++;
    }
    /*
    unsigned int iStart = offset/bm->blockSize;
    BlockData *start = bm->getBlock(path, iStart);
    if(start != nullptr)
    {
        int startRead = std::min(start->bSize - offset, size);
        strncpy(buf, start->data + (offset % bm->blockSize), startRead);
        readCount += startRead;
        buf += startRead;
        size -= startRead;
    }
    else
    {
        pread(fi->fh, buf, )
    }
*/
/*
    while(size > 0)
    {
        if(curIndex == offset/bm->blockSize)
        {
            BlockData *curB = bm->getBlock(path, curIndex);
            if(curB != nullptr)
            {
                strncpy(buf, curB->data + (offset % bm->blockSize), curB->bSize - offset);
                size -= curB->bSize;
            }
        }
        BlockData *curB = bm->getBlock(path, curIndex);
        if(curB != nullptr)
        {
            if(firstBlock)
            {

            }
            strncpy(buf, curB->data + (offset % bm->blockSize), curB->bSize);
            size -= curB->bSize;

        }
    }
    */
    return 0;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int caching_flush(const char *path, struct fuse_file_info *fi)
{
    logF(FLUSH);
    if(isLogFile(path)) { return -ENOENT;}
    return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int caching_release(const char *path, struct fuse_file_info *fi)
{
    
    logF(REALESE);
    if(isLogFile(path)) { return -ENOENT;}
    return close(fi->fh);
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *path, struct fuse_file_info *fi)
{
    logF(OPENDIR);
    if(isLogFile(path)) { return -ENOENT;}
    int ret = 0;
    DIR *dir;
    char rootPath[PATH_MAX];
    changeRootPath(rootPath, path);
    dir = opendir(rootPath);
    //if (dp == NULL)
    //retstat = log_error("bb_opendir opendir");
    fi->fh = (intptr_t)dir;
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * Introduced in version 2.3
 */
int caching_readdir(const char *path, void *buf,
                    fuse_fill_dir_t filler,
                    off_t offset, struct fuse_file_info *fi)
{
    logF(READDIR);
    if(isLogFile(path)) { return -ENOENT;}
    int ret = 0;
    DIR *dir;
    struct dirent *d;
    dir = (DIR*)(uintptr_t)fi->fh;
    d = readdir(dir);
    if (d == 0)
    {
        //ret = log_error;
        return ret;
    }
    do
    {
        if (filler(buf, dir->d_name, NULL, 0) != 0)
        {
            return -ENOMEM;
        }
    } while ((d = readdir(dir)) != NULL);
    return ret;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi)
{
    logF(RELEASEDIR);
    if(isLogFile(path)) { return -ENOENT;}
    closedir((DIR*)(uintptr_t)fi->fh);
    return 0;
}

/** Rename a file */
int caching_rename(const char *path, const char *newpath)
{
    logF(REANAME);
    if(isLogFile(path)) { return -ENOENT;}
    char rootPath[PATH_MAX];
    char newRootPath[PATH_MAX];
    changeRootPath(rootPath, path);
    changeRootPath(newRootPath, newpath);
    bM.renameFile(rootPath, newRootPath);
    return rename(rootPath, newRootPath);
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 
If a failure occurs in this function, do nothing (absorb the failure 
and don't report it). 
For your task, the function needs to return NULL always 
(if you do something else, be sure to use the fuse_context correctly).
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *caching_init(struct fuse_conn_info *conn)
{
    logF(INIT);
    // new obj 
    return NULL;
}


/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
  
If a failure occurs in this function, do nothing 
(absorb the failure and don't report it). 
 
 * Introduced in version 2.3
 */
void caching_destroy(void *userdata)
{
    logF(Destroy)
}


/**
 * Ioctl from the FUSE sepc:
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * However, in our case, this function only needs to print 
 cache table to the log file .
 * 
 * Introduced in version 2.8
 */
int caching_ioctl(const char *, int cmd, void *arg,
                  struct fuse_file_info *, unsigned int flags, void *data)
{
    logF(IOCTL);
    //for (auto it : bM.newData)
    return 0;
}


// Initialise the operations. 
// You are not supposed to change this function.
void init_caching_oper()
{

    caching_oper.getattr = caching_getattr;
    caching_oper.access = caching_access;
    caching_oper.open = caching_open;
    caching_oper.read = caching_read;
    caching_oper.flush = caching_flush;
    caching_oper.release = caching_release;
    caching_oper.opendir = caching_opendir;
    caching_oper.readdir = caching_readdir;
    caching_oper.releasedir = caching_releasedir;
    caching_oper.rename = caching_rename;
    caching_oper.init = caching_init;
    caching_oper.destroy = caching_destroy;
    caching_oper.ioctl = caching_ioctl;
    caching_oper.fgetattr = caching_fgetattr;


    caching_oper.readlink = NULL;
    caching_oper.getdir = NULL;
    caching_oper.mknod = NULL;
    caching_oper.mkdir = NULL;
    caching_oper.unlink = NULL;
    caching_oper.rmdir = NULL;
    caching_oper.symlink = NULL;
    caching_oper.link = NULL;
    caching_oper.chmod = NULL;
    caching_oper.chown = NULL;
    caching_oper.truncate = NULL;
    caching_oper.utime = NULL;
    caching_oper.write = NULL;
    caching_oper.statfs = NULL;
    caching_oper.fsync = NULL;
    caching_oper.setxattr = NULL;
    caching_oper.getxattr = NULL;
    caching_oper.listxattr = NULL;
    caching_oper.removexattr = NULL;
    caching_oper.fsyncdir = NULL;
    caching_oper.create = NULL;
    caching_oper.ftruncate = NULL;
}

//basic main. You need to complete it.
int main(int argc, char *argv[])
{
    
    init_caching_oper();
    argv[1] = argv[2];
    for (int i = 2; i < (argc - 1); i++)
    {
        argv[i] = NULL;
    }
    argv[2] = (char *) "-s";
    argc = 3;

    int fuse_stat = fuse_main(argc, argv, &caching_oper, NULL);
    return fuse_stat;
}

inline void logF(std::string s)
{
    logFile << time() << SPACE << s << std::endl;
}

inline bool isLogFile(std::string path)
{
    if (path.find(LOG_FILE_STRING) == path.length() - LOG_FILE_STRING)
    {
    	return true;
    }
	return false;
}
