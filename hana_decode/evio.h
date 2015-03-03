/*-----------------------------------------------------------------------------
 * Copyright (c) 2013 Jefferson Science Associates,
 *                    Thomas Jefferson National Accelerator Faciltiy,
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * JLAB Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: timmmer@jlab.org  Tel: (757) 269-5130
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *  Event I/O test program
 *  
 * Author:  Elliott Wolin, June 2001
 *          Carl Timmer 2013, JLAB Data Acquisition Group
 *
 */


#ifndef __EVIO_h__
#define __EVIO_h__

#define EV_VERSION 4

/** Size of block header in 32 bit words.
 *  Must never be smaller than 8, but can be set larger.*/
#define EV_HDSIZ 8


#ifndef S_SUCCESS
#define S_SUCCESS 0
#define S_FAILURE -1
#endif

/* see "Programming with POSIX threads' by Butenhof */
#define evio_err_abort(code,text) do { \
    fprintf (stderr, "%s at \"%s\":%d: %s\n", \
        text, __FILE__, __LINE__, strerror (code)); \
    exit (-1); \
} while (0)


#define S_EVFILE    		0x00730000	/**< evfile.msg Event File I/O */
#define S_EVFILE_TRUNC		0x40730001	/**< Event truncated on read/write */
#define S_EVFILE_BADBLOCK	0x40730002	/**< Bad block number encountered */
#define S_EVFILE_BADHANDLE	0x80730001	/**< Bad handle (file/stream not open) */
#define S_EVFILE_ALLOCFAIL	0x80730002	/**< Failed to allocate event I/O structure */
#define S_EVFILE_BADFILE	0x80730003	/**< File format error */
#define S_EVFILE_UNKOPTION	0x80730004	/**< Unknown option specified */
#define S_EVFILE_UNXPTDEOF	0x80730005	/**< Unexpected end of file while reading event */
#define S_EVFILE_BADSIZEREQ 0x80730006  /**< Invalid buffer size request to evIoct */
#define S_EVFILE_BADARG     0x80730007  /**< Invalid function argument */
#define S_EVFILE_BADMODE    0x80730008  /**< Wrong mode used in evOpen for this operation */

/* macros for swapping ints of various sizes */
#ifdef VXWORKS

#define UINT64_MAX 0xffffffffffffffffULL
#define EVIO_SWAP64(x) ( (((x) >> 56) & 0x00000000000000FFULL) | \
                         (((x) >> 40) & 0x000000000000FF00ULL) | \
                         (((x) >> 24) & 0x0000000000FF0000ULL) | \
                         (((x) >> 8)  & 0x00000000FF000000ULL) | \
                         (((x) << 8)  & 0x000000FF00000000ULL) | \
                         (((x) << 24) & 0x0000FF0000000000ULL) | \
                         (((x) << 40) & 0x00FF000000000000ULL) | \
                         (((x) << 56) & 0xFF00000000000000ULL) )
#else

#define EVIO_SWAP64(x) ( (((x) >> 56) & 0x00000000000000FFL) | \
                         (((x) >> 40) & 0x000000000000FF00L) | \
                         (((x) >> 24) & 0x0000000000FF0000L) | \
                         (((x) >> 8)  & 0x00000000FF000000L) | \
                         (((x) << 8)  & 0x000000FF00000000L) | \
                         (((x) << 24) & 0x0000FF0000000000L) | \
                         (((x) << 40) & 0x00FF000000000000L) | \
                         (((x) << 56) & 0xFF00000000000000L) )
#endif

#define EVIO_SWAP32(x) ( (((x) >> 24) & 0x000000FF) | \
                         (((x) >> 8)  & 0x0000FF00) | \
                         (((x) << 8)  & 0x00FF0000) | \
                         (((x) << 24) & 0xFF000000) )

#define EVIO_SWAP16(x) ( (((x) >> 8) & 0x00FF) | \
                         (((x) << 8) & 0xFF00) )

#include <stdio.h>
#include <pthread.h>

#ifdef sun
    #include <sys/param.h>
#else
    #include <stddef.h>

    #ifdef _MSC_VER
        typedef __int64 int64_t;	// Define it from MSVC's internal type
        #include "msinttypes.h"
        #define strcasecmp _stricmp
        #define strncasecmp strnicmp
    #elif !defined VXWORKS
        #include <stdint.h>		  // Use the C99 official header
    #endif   
#endif

        
/**
 * This structure contains information about file
 * opened for either reading or writing.
 */
typedef struct evfilestruct {

  FILE    *file;         /**< pointer to file. */
  int      handle;       /**< handle used to access this structure. */
  int      rw;           /**< are we reading, writing, piping? */
  int      magic;        /**< magic number. */
  int      byte_swapped; /**< bytes do NOT need swapping = 0 else 1 */
  int      version;      /**< evio version number. */
  int      append;       /**< open buffer or file for writing in append mode = 1, else 0.
                              If append = 2, then an event was already been appended. */
  uint32_t eventCount;   /**< current number of events in (or written to) file/buffer
                          * NOT including dictionary(ies). If the file being written to is split,
                          * this value refers to all split files taken together. */

  /* block stuff */
  uint32_t *buf;           /**< For files, sockets, and reading buffers = pointer to
                            *   buffer of block-being-read / blocks-being-written.
                            *   When writing to file/socket/pipe, this buffer may
                            *   contain multiple blocks.
                            *   When writing to buffer, this points to block header
                            *   in block currently being written to (no separate
                            *   block buffer exists). */
  uint32_t  *next;         /**< pointer to next word in block to be read/written. */
  uint32_t  left;          /**< # of valid 32 bit unread/unwritten words in block. */
  uint32_t  blksiz;        /**< size of block in 32 bit words - v3 or
                            *   size of actual data in block (including header) - v4. */
  uint32_t  blknum;        /**< block number of block being read/written (block #s start at 1). */
  int       blkNumDiff;    /**< When reading, the difference between blknum read in and
                            *   the expected (sequential) value. Used in debug message. */
  uint32_t  blkSizeTarget; /**< target size of block in 32 bit words (including block header). */
  uint32_t  blkEvCount;    /**< number of events written to block so far. */
  uint32_t  bufSize;       /**< When reading, size of block buffer (buf) in 32 bit words.
                            *   When writing file/sock/pipe, size of buffer being written to
                            *   that is actually being used (must be <= bufRealSize). */
  uint32_t  bufRealSize;   /**< When writing file/sock/pipe, total size of buffer being written to.
                            *   Amount of memory actually allocated in 32 bit words (not all may
                            *   be used). */
  uint32_t  blkEvMax;      /**< max number of events per block. */
  int       isLastBlock;   /**< 1 if buf contains last block of file/sock/buf, else 0. */


  /* file stuff: splitting, auto naming, internal buffer */
  char     *baseFileName;   /**< base name of file to be written to. */
  char     *fileName;       /**< actual name of file to be written to. */
  char     *runType;        /**< run type used in auto naming of split files. */
  int       specifierCount; /**< number of C printing int format specifiers in file name (0, 1, 2). */
  int       splitting;      /**< 0 if not splitting file, else 1. */
  uint32_t *currentHeader;  /**< When writing to file/socket/pipe, this points to
                             *   current block header of block being written. */
  uint32_t  bytesToBuf;     /**< # bytes written to internal buffer including ending empty block & dict. */
  uint32_t  eventsToBuf;    /**< # events written to internal buffer including dictionary. */
  uint32_t  eventsToFile;   /**< # of events written to file including dictionary.
                             * If the file is being split, this value refers to the file
                             * currently being written to. */
  uint64_t  bytesToFile;    /**< # bytes flushed to the current file (including ending
                             *   empty block & dictionary), not the total in all split files. */
  uint32_t  runNumber;      /**< run # used in auto naming of split files. */
  uint32_t  splitNumber;    /**< number of next split file (used in auto naming). */
  uint64_t  split;          /**< # of bytes at which to split file when writing
                             *  (defaults to EV_SPLIT_SIZE, 1GB). */
  uint64_t  fileSize;       /**< size of file being written to, in bytes. */


  /* buffer stuff */
  char     *rwBuf;         /**< pointer to buffer if reading/writing from/to buffer. */
  uint32_t  rwBufSize;     /**< size of rwBuf buffer in bytes. */
  uint32_t  rwBytesOut;    /**< number of bytes written to rwBuf with evWrite. */
  uint32_t  rwBytesIn;     /**< number of bytes read from rwBuf so far
                            *   (i.e. # bytes in buffer already used).*/
  int       rwFirstWrite;  /**< 1 if this evWrite is the first for this rwBuf, else 0.
                            *   Needed for calculating accurate value for rwBytesOut. */

  /* socket stuff */
  int   sockFd;            /**< socket file descriptor if reading/writing from/to socket. */

  /* randomAcess stuff */
  int        randomAccess; /**< if true, use random access file/buffer reading. */
  size_t     mmapFileSize; /**< size of mapped file in bytes. */
  uint32_t  *mmapFile;     /**< pointer to memory mapped file. */
  uint32_t  **pTable;      /**< array of pointers to events in memory mapped file or buffer. */

  /* dictionary */
  int   wroteDictionary;   /**< dictionary already written out to a single (split fragment) file? */
  uint32_t dictLength;     /**< length of dictionary bank in bytes. */
  uint32_t *dictBuf;       /**< buffer containing dictionary bank. */
  char *dictionary;        /**< xml format dictionary to either read or write. */

  /* synchronization */
  pthread_mutex_t lock;   /**< lock for multithreaded reads & writes. */

} EVFILE;


/** Structure for Sergei Boiarinov's use. */
typedef struct evioBlockHeaderV4_t {

	uint32_t length;       /**< total length of block in 32-bit words including this complete header. */
	uint32_t blockNumber;  /**< block id # starting at 1. */
	uint32_t headerLength; /**< length of this block header in 32-bit words (always 8). */
	uint32_t eventCount;   /**< # of events in this block (not counting dictionary). */
	uint32_t reserved1;    /**< reserved for future use. */
	uint32_t bitInfo;      /**< Contains version # in lowest 8 bits.
                                If dictionary is included as the first event, bit #9 is set.
                                If is a last block, bit #10 is set. */
	uint32_t reserved2;    /**< reserved for future use. */
	uint32_t magicNumber;  /**< written as 0xc0da0100 and used to check endianness. */

} evioBlockHeaderV4;

/** Offset in bytes from beginning of block header to block length. */
#define EVIO_BH_LEN_OFFSET 0
/** Offset in bytes from beginning of block header to block number. */
#define EVIO_BH_BLKNUM_OFFSET 32
/** Offset in bytes from beginning of block header to header length. */
#define EVIO_BH_HDRLEN_OFFSET 64
/** Offset in bytes from beginning of block header to event count. */
#define EVIO_BH_EVCOUNT_OFFSET 96
/** Offset in bytes from beginning of block header to bitInfo. */
#define EVIO_BH_BITINFO_OFFSET 160
/** Offset in bytes from beginning of block header to magic number. */
#define EVIO_BH_MAGNUM_OFFSET 224

/* prototypes */
#ifdef __cplusplus
extern "C" {
#endif

void set_user_frag_select_func( int32_t (*f) (int32_t tag) );
void evioswap(uint32_t *buffer, int tolocal, uint32_t *dest);

int evOpen(char *filename, char *flags, int *handle);
int evOpenBuffer(char *buffer, uint32_t bufLen, char *flags, int *handle);
int evOpenSocket(int sockFd, char *flags, int *handle);

int evRead(int handle, uint32_t *buffer, uint32_t size);
int evReadAlloc(int handle, uint32_t **buffer, uint32_t *buflen);
int evReadNoCopy(int handle, const uint32_t **buffer, uint32_t *buflen);
int evReadRandom(int handle, const uint32_t **pEvent, uint32_t *buflen, uint32_t eventNumber);
int evGetRandomAccessTable(int handle, const uint32_t ***table, uint32_t *len);

int evWrite(int handle, const uint32_t *buffer);
int evIoctl(int handle, char *request, void *argp);
int evClose(int handle);
int evGetBufferLength(int handle, uint32_t *length);

int evGetDictionary(int handle, char **dictionary, uint32_t *len);
int evWriteDictionary(int handle, char *xmlDictionary);

int evIsContainer(int type);
const char *evGetTypename(int type);
char *evPerror(int error);

void  evPrintBuffer(uint32_t *p, uint32_t len, int swap);

char *evStrReplace(char *orig, const char *replace, const char *with);
char *evStrReplaceEnvVar(const char *orig);
char *evStrFindSpecifiers(const char *orig, int *specifierCount);
char *evStrRemoveSpecifiers(const char *orig);
int   evGenerateBaseFileName(char *origName, char **baseName, int *count);
char *evGenerateFileName(EVFILE *a, int specifierCount, int runNumber,
                         int split, int splitNumber, char *runType);

#ifdef __cplusplus
}

#endif

#endif
