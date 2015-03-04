/*-----------------------------------------------------------------------------
 * Copyright (c) 1991,1992 Southeastern Universities Research Association,
 *                         Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 * Email: coda@cebaf.gov  Tel: (804) 249-7101  Fax: (804) 249-7363
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *	Event I/O routines
 *	
 * Author:  Chip Watson, CEBAF Data Acquisition Group
 *
 * Modified: Stephen A. Wood, TJNAF Hall C
 *      Works on ALPHA 64 bit machines if _LP64 is defined
 *      Will read input from standard input if filename is "-"
 *      If input filename is "|command" will take data from standard output
 *                        of command.
 *      If input file is compressed and uncompressible with gunzip, it will
 *                        decompress the data on the fly.
 *
 * Modified: Carl Timmer, DAQ group, Jan 2012
 *      Get rid of gzip compression stuff
 *      Get rid of ALPHA stuff
 *      Everything is now _LP64
 *      Updates for evio version 4 include:
 *          1) Enable reading from / writing to buffers and sockets
 *          2) Same format for writing to file as for writing to buffer or over socket
 *          3) Allow seemless storing of dictionary as very first bank
 *          4) Keep information about padding for 8 byte int & short data.
 *          5) Each data block is variable size with an integral number of events
 *          6) Data block size is suggestable
 *          7) Added documentation - How about that!
 *          8) Move 2 routines from evio_util.c to here (evIsContainer, evGetTypename)
 *          9) Add read that allocates memory instead of using supplied buffer
 *         10) Add random access reads using memory mapped files
 *         11) Add routine for getting event pointers in random access mode
 *         12) Add no-copy reads for streams
 *         13) Add routine for finding how many bytes written to buffer
 *         14) Additional options for evIoctl including getting # of events in file/buffer, etc.
 *
 * Modified: Carl Timmer, DAQ group, Sep 2012
 *      Make threadsafe by using read/write lock when getting/closing handle
 *      and using mutex to prevent simultaneous reads/writes per handle.
 * 
 * Modified: Carl Timmer, DAQ group, Aug 2013
 *      Build in file splitting & auto-naming capabilities.
 *      Fix bug in mutex handling.
 *      Allow mutexes to be turned off.
 *      Allow setting buffer size.
 * 
 * Routines:
 * ---------
 * 
 * int  evOpen                 (char *filename, char *flags, int *handle)
 * int  evOpenBuffer           (char *buffer, int bufLen, char *flags, int *handle)
 * int  evOpenSocket           (int sockFd, char *flags, int *handle)
 * int  evRead                 (int handle, uint32_t *buffer, uint32_t buflen)
 * int  evReadAlloc            (int handle, uint32_t **buffer, uint32_t *buflen)
 * int  evReadNoCopy           (int handle, const uint32_t **buffer, uint32_t *buflen)
 * int  evReadRandom           (int handle, const uint32_t **pEvent, uint32_t *buflen, uint32_t eventNumber)
 * int  evWrite                (int handle, const uint32_t *buffer)
 * int  evIoctl                (int handle, char *request, void *argp)
 * int  evClose                (int handle)
 * int  evGetBufferLength      (int handle, uint32_t *length)
 * int  evGetRandomAccessTable (int handle, const uint32_t *** const table, uint32_t *len)
 * int  evGetDictionary        (int handle, char **dictionary, uint32_t *len)
 * int  evWriteDictionary      (int handle, char *xmlDictionary)
 * int  evIsContainer          (int type)
 * char *evPerror              (int error)
 * const char *evGetTypename   (int type)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "evio.h"


/* A few items to make the code more readable */
#define EV_READFILE     0
#define EV_READPIPE     1
#define EV_READSOCK     2
#define EV_READBUF      3

#define EV_WRITEFILE    4
#define EV_WRITEPIPE    5
#define EV_WRITESOCK    6
#define EV_WRITEBUF     7


/** Number used to determine data endian */
#define EV_MAGIC 0xc0da0100

/** Version 3's fixed block size in 32 bit words */
#define EV_BLOCKSIZE_V3 8192

/** Version 4's target block size in 32 bit words (256kB) */
#define EV_BLOCKSIZE_V4 64000

/** Minimum block size allowed if size reset */
#define EV_BLOCKSIZE_MIN 256
/*#define EV_BLOCKSIZE_MIN 16*/

/** Maximum block size allowed if size reset. Need to play nice with jevio.
 * Can be a max of (2^31 - 1 - 2headers) = 2147483583 , but set it to 100MB. */
#define EV_BLOCKSIZE_MAX 25600000

/** In version 4, lowest 8 bits are version, rest is bit info */
#define EV_VERSION_MASK 0xFF

/** In version 4, dictionary presence is 9th bit in version/info word */
#define EV_DICTIONARY_MASK 0x100

/** In version 4, "last block" is 10th bit in version/info word */
#define EV_LASTBLOCK_MASK 0x200

/** In version 4, maximum max number of events per block */
#define EV_EVENTS_MAX 100000

/** In version 4, default max number of events per block */
#define EV_EVENTS_MAX_DEF 10000

/** In version 4, if splitting file, default split size in bytes (2GB) */
#define EV_SPLIT_SIZE 2000000000L

/**
 * @file
 * <pre>
 * Let's take a look at an evio block header also
 * known as a physical record header.
 * 
 * In versions 1, 2 & 3, evio files impose an anachronistic
 * block structure. The complication that arises is that logical records
 * (events) will sometimes cross physical record boundaries.
 *
 * ####################################
 * Evio block header, versions 1,2 & 3:
 * ####################################
 *
 * MSB(31)                          LSB(0)
 * <---  32 bits ------------------------>
 * _______________________________________
 * |             Block Size              |
 * |_____________________________________|
 * |            Block Number             |
 * |_____________________________________|
 * |           Header Size = 8           |
 * |_____________________________________|
 * |               Start                 |
 * |_____________________________________|
 * |                Used                 |
 * |_____________________________________|
 * |              Version                |
 * |_____________________________________|
 * |              Reserved               |
 * |_____________________________________|
 * |              Magic #                |
 * |_____________________________________|
 *
 *
 *      Block Size    = number of 32 bit ints in block (including this one).
 *                      This is fixed for versions 1-3, generally at 8192 (32768 bytes)
 *      Block Number  = id number (starting at 1)
 *      Header Size   = number of 32 bit nts in this header (always 8)
 *      Start         = offset to first event header in block relative to start of block
 *      Used          = # of used/valid words (header + data) in block,
 *                      (normally = block size, but in last block may be smaller)
 *      Version       = evio format version
 *      Reserved      = reserved
 *      Magic #       = magic number (0xc0da0100) used to check endianness
 *
 *
 * 
 * In version 4, only an integral number of complete
 * events will be contained in a single block.
 *
 * ################################
 * Evio block header, version 4+:
 * ################################
 *
 * MSB(31)                          LSB(0)
 * <---  32 bits ------------------------>
 * _______________________________________
 * |             Block Size              |
 * |_____________________________________|
 * |            Block Number             |
 * |_____________________________________|
 * |          Header Length = 8          |
 * |_____________________________________|
 * |             Event Count             |
 * |_____________________________________|
 * |              Reserved               |
 * |_____________________________________|
 * |          Bit info         | Version |
 * |_____________________________________|
 * |              Reserved               |
 * |_____________________________________|
 * |            Magic Number             |
 * |_____________________________________|
 *
 *
 *      Block Size         = number of ints in block (including this one).
 *      Block Number       = id number (starting at 1)
 *      Header Length      = number of ints in this header (EV_HDSIZ which is currently 8)
 *      Event Count        = number of events in this block (always an integral #).
 *                           NOTE: this value should not be used to parse the following
 *                           events since the first block may have a dictionary whose
 *                           presence is not included in this count.
 *      Bit info & Version = Lowest 8 bits are the version number (4).
 *                           Upper 24 bits contain bit info.
 *                           If a dictionary is included as the first event, bit #9 is set (=1)
 *      Magic #            = magic number (0xc0da0100) used to check endianness
 *
 *
 *   Bit info (24 bits) has the following bits defined (starting at 1):
 * 
 *   Bit  9     = true if dictionary is included (relevant for first block only)
 *   Bit  10    = true if this block is the last block in file or network transmission
 *   Bits 11-14 = type of events following (ROC Raw = 0, Physics = 1, PartialPhysics = 2,
 *                DisentangledPhysics = 3, User = 4, Control = 5, Prestart = 6, Go = 7,
 *                Pause = 8, End = 9, Other = 15)
 *
 *
 * ################################
 * COMPOSITE DATA:
 * ################################
 *   This is a new type of data (value = 0xf) which originated with Hall B.
 *   It is a composite type and allows for possible expansion in the future
 *   if there is a demand. Basically it allows the user to specify a custom
 *   format by means of a string - stored in a tagsegment. The data in that
 *   format follows in a bank. The routine to swap this data must be provided
 *   by the definer of the composite type - in this case Hall B. The swapping
 *   function is plugged into this evio library's swapping routine.
 *   Here's what it looks like.
 *
 * MSB(31)                          LSB(0)
 * <---  32 bits ------------------------>
 * _______________________________________
 * |  tag    | type |    length          | --> tagsegment header
 * |_________|______|____________________|
 * |        Data Format String           |
 * |                                     |
 * |_____________________________________|
 * |              length                 | \
 * |_____________________________________|  \  bank header
 * |       tag      |  type   |   num    |  /
 * |________________|_________|__________| /
 * |               Data                  |
 * |                                     |
 * |_____________________________________|
 *
 *   The beginning tagsegment is a normal evio tagsegment containing a string
 *   (type = 0x3). Currently its type and tag are not used - at least not for
 *   data formatting.
 *   The bank is a normal evio bank header with data following.
 *   The format string is used to read/write this data so that takes care of any
 *   padding that may exist. As with the tagsegment, the tags and type are ignored.
 * </pre>
 */

#define EV_HD_BLKSIZ 0	/**< Position of blk hdr word for size of block in 32-bit words. */
#define EV_HD_BLKNUM 1	/**< Position of blk hdr word for block number, starting at 0. */
#define EV_HD_HDSIZ  2	/**< Position of blk hdr word for size of header in 32-bit words (=8). */
#define EV_HD_COUNT  3  /**< Position of blk hdr word for number of events in block (version 4+). */
#define EV_HD_START  3  /**< Position of blk hdr word for first start of event in this block (ver 1-3). */
#define EV_HD_USED   4	/**< Position of blk hdr word for number of words used in block (<= BLKSIZ) (ver 1-3). */
#define EV_HD_RESVD1 4  /**< Position of blk hdr word for reserved (ver 4+). */
#define EV_HD_VER    5	/**< Position of blk hdr word for version of file format (+ bit info in ver 4+). */
#define EV_HD_RESVD2 6	/**< Position of blk hdr word for reserved. */
#define EV_HD_MAGIC  7	/**< Position of blk hdr word for magic number for endianness tracking. */


/** Turn on 9th bit to indicate dictionary included in block */
#define setDictionaryBit(a)     ((a)[EV_HD_VER] |= EV_DICTIONARY_MASK)
/** Turn off 9th bit to indicate dictionary included in block */
#define clearDictionaryBit(a)   ((a)[EV_HD_VER] &= ~EV_DICTIONARY_MASK)
/** Is there a dictionary in this block? */
#define hasDictionary(a)        (((a)[EV_HD_VER] & EV_DICTIONARY_MASK) > 0 ? 1 : 0)
/** Is there a dictionary in this block? */
#define hasDictionaryInt(i)     ((i & EV_DICTIONARY_MASK) > 0 ? 1 : 0)
/** Turn on 10th bit to indicate last block of file/transmission */
#define setLastBlockBit(a)      ((a)[EV_HD_VER] |= EV_LASTBLOCK_MASK)
/** Turn off 10th bit to indicate last block of file/transmission */
#define clearLastBlockBit(a)    ((a)[EV_HD_VER] &= ~EV_LASTBLOCK_MASK)
/** Turn off 10th bit to indicate last block of file/transmission */
#define clearLastBlockBitInt(i) (i &= ~EV_LASTBLOCK_MASK)
/** Is this the last block of file/transmission? */
#define isLastBlock(a)          (((a)[EV_HD_VER] & EV_LASTBLOCK_MASK) > 0 ? 1 : 0)
/** Is this the last block of file/transmission? */
#define isLastBlockInt(i)       ((i & EV_LASTBLOCK_MASK) > 0 ? 1 : 0)

/** Initialize a block header */
#define initBlockHeader(a) { \
    (a)[EV_HD_BLKSIZ] = 0; \
    (a)[EV_HD_BLKNUM] = 1; \
    (a)[EV_HD_HDSIZ]  = EV_HDSIZ; \
    (a)[EV_HD_COUNT]  = 0; \
    (a)[EV_HD_RESVD1] = 0; \
    (a)[EV_HD_VER]    = EV_VERSION; \
    (a)[EV_HD_RESVD2] = 0; \
    (a)[EV_HD_MAGIC]  = EV_MAGIC; \
} \

/** Initialize a last block header */
#define initLastBlockHeader(a,b) { \
    (a)[EV_HD_BLKSIZ] = 8; \
    (a)[EV_HD_BLKNUM] = b; \
    (a)[EV_HD_HDSIZ]  = EV_HDSIZ; \
    (a)[EV_HD_COUNT]  = 0; \
    (a)[EV_HD_RESVD1] = 0; \
    (a)[EV_HD_VER]    = EV_VERSION | EV_LASTBLOCK_MASK; \
    (a)[EV_HD_RESVD2] = 0; \
    (a)[EV_HD_MAGIC]  = EV_MAGIC; \
} \


/* Prototypes for static routines */
static  int      fileExists(char *filename);
static  int      evOpenImpl(char *srcDest, uint32_t bufLen, int sockFd, char *flags, int *handle);
static  int      evGetNewBuffer(EVFILE *a);
static  char *   evTrim(char *s, int skip);
static  int      tcpWrite(int fd, const void *vptr, int n);
static  int      tcpRead(int fd, void *vptr, int n);
static  int      evReadAllocImpl(EVFILE *a, uint32_t **buffer, uint32_t *buflen);
static  void     localClose(EVFILE *a);
static  int      getEventCount(EVFILE *a, uint32_t *count);
static  int      evWriteImpl(int handle, const uint32_t *buffer, int useMutex, int isDictionary);
static  int      evFlush(EVFILE *a);
static  int      splitFile(EVFILE *a);
static  int      evWriteDictImpl(EVFILE *a, const uint32_t *buffer);
static  int      generateSixthWord(int version, int hasDictionary, int isEnd, int eventType);
static  void     resetBuffer(EVFILE *a);
static  int      expandBuffer(EVFILE *a, uint32_t newSize);
static  int      evWriteDictImpl(EVFILE *a, const uint32_t *buffer);
static  void     writeEventToBuffer(EVFILE *a, const uint32_t *buffer,
                                    uint32_t wordsToWrite, int isDictionary);
static  int      writeEmptyLastBlockHeader(EVFILE *a, int blockNumber);
static  int      writeNewHeader(EVFILE *a, uint32_t wordSize,
                          uint32_t eventCount, uint32_t blockNumber,
                          int hasDictionary,   int isLast,
                          int isCurrentHeader, int absoluteMode);

/* Dealing with EVFILE struct */
static void      structInit(EVFILE *a);
static void      structDestroy(EVFILE *a);
static void      mutexLock(EVFILE *a);
static void      mutexUnlock(EVFILE *a);
static void      handleReadLock(int handle);
static void      handleReadUnlock(int handle);
static void      handleWriteLock(int handle);
static void      handleWriteUnlock(int handle);
static void      getHandleLock(void);
static void      getHandleUnlock(void);

/* Append Mode */
static  int      toAppendPosition(EVFILE *a);

/* Random Access Mode */
static  int      memoryMapFile(EVFILE *a, const char *fileName);
static  int      generatePointerTable(EVFILE *a);

/* Array that holds all pointers to structures created with evOpen().
 * Space in the array is allocated as needed, beginning with 100
 * and adding 50% every time more are needed. */
static EVFILE **handleList = NULL;
/* The number of handles available for use. */
static int handleCount = 0;

#ifndef VXWORKS
    /* Pthread mutex for serializing calls to get and free handles. */
    static pthread_mutex_t getHandleMutex = PTHREAD_MUTEX_INITIALIZER;
    /*static pthread_rwlock_t handleLock = PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP;*/
  
    /* Array of pthread read-write lock pointers for preventing simultaneous calls
     * to evClose and read/write routines. Need 1 for each evOpen() call. */
    static pthread_rwlock_t **handleLocks = NULL;
    
#elif defined VXWORKS_5

    /** Implementation of strdup for vxWorks. */
    static char *strdup(const char *s1) {
        char *s;
        if (s1 == NULL) return NULL;
        if ((s = (char *) malloc(strlen(s1)+1)) == NULL) return NULL;
        return strcpy(s, s1);
    }
    
    /** Implementation of strndup for vxWorks. */
    static char *strndup(const char *s1, size_t count) {
        int len;
        char *s;
        if (s1 == NULL) return NULL;
    
        len = strlen(s1) > count ? count : strlen(s1);
        if ((s = (char *) malloc(len+1)) == NULL) return NULL;
        s[len] = '\0';
        return strncpy(s, s1, len);
    }
    
    /** Implementation of strcasecmp for vxWorks. */
    static int strcasecmp(const char *s1, const char *s2) {
        int i, len1, len2;
      
        /* handle NULL's */
        if (s1 == NULL && s2 == NULL) {
            return 0;
        }
        else if (s1 == NULL) {
            return -1;
        }
        else if (s2 == NULL) {
            return 1;
        }
      
        len1 = strlen(s1);
        len2 = strlen(s2);
      
        /* handle different lengths */
        if (len1 < len2) {
            for (i=0; i<len1; i++) {
                if (toupper((int) s1[i]) < toupper((int) s2[i])) {
                    return -1;
                }
                else if (toupper((int) s1[i]) > toupper((int) s2[i])) {
                    return 1;
                }
            }
            return -1;
        }
        else if (len1 > len2) {
            for (i=0; i<len2; i++) {
                if (toupper((int) s1[i]) < toupper((int) s2[i])) {
                    return -1;
                }
                else if (toupper((int) s1[i]) > toupper((int) s2[i])) {
                    return 1;
                }
            }
            return 1;
        }
      
        /* handle same lengths */
        for (i=0; i<len1; i++) {
            if (toupper((int) s1[i]) < toupper((int) s2[i])) {
                return -1;
            }
            else if (toupper((int) s1[i]) > toupper((int) s2[i])) {
                return 1;
            }
        }
      
        return 0;
    }
    
    /** Implementation of strncasecmp for vxWorks. */
    static int strncasecmp(const char *s1, const char *s2, size_t n) {
        int i, len1, len2;
      
        /* handle NULL's */
        if (s1 == NULL && s2 == NULL) {
            return 0;
        }
        else if (s1 == NULL) {
            return -1;
        }
        else if (s2 == NULL) {
            return 1;
        }
      
        len1 = strlen(s1);
        len2 = strlen(s2);
        
        /* handle short lengths */
        if (len1 < n || len2 < n) {
            return strcasecmp(s1, s2);
        }
       
        /* both lengths >= n, but compare only n chars */
        for (i=0; i<n; i++) {
            if (toupper((int) s1[i]) < toupper((int) s2[i])) {
                return -1;
            }
            else if (toupper((int) s1[i]) > toupper((int) s2[i])) {
                return 1;
            }
        }
      
        return 0;
    }

#endif

/*-----------------------------------------------------------------------------*/


/**
 * Routine to print the contents of a buffer.
 * 
 * @param p pointer to buffer
 * @param len number of 32-bit words to print out
 * @param swap swap if true
 */
void evPrintBuffer(uint32_t *p, uint32_t len, int swap) {
    uint32_t i;
    printf("\nBUFFER:\n");
    for (i=0; i < len; i++) {
        if (swap) {
            printf("%u   0x%08x\n", i, EVIO_SWAP32(*(p++)));
        }
        else {
            printf("%u   0x%08x\n", i, *(p++));
        }
    }
    printf("\n");
}


/**
 * Routine to intialize an EVFILE structure for writing.
 * If reading, relevant stuff gets overwritten anyway.
 * 
 * @param a   pointer to structure being inititalized
 */
static void structInit(EVFILE *a)
{
    a->file         = NULL;
    a->handle       = 0;
    a->rw           = 0;
    a->magic        = EV_MAGIC;
    a->byte_swapped = 0;
    a->version      = EV_VERSION;
    a->append       = 0;
    a->eventCount   = 0;

    /* block stuff */
    a->buf           = NULL;
    a->next          = NULL;
    /* Space in number of words, not in header, left for writing */
    a->left          = EV_BLOCKSIZE_V4 - EV_HDSIZ;
    /* Total data written = block header size so far */
    a->blksiz        = EV_HDSIZ;
    a->blknum        = 1;
    a->blkNumDiff    = 0;
    /* Target block size (final size may be larger or smaller) */
    a->blkSizeTarget = EV_BLOCKSIZE_V4;
    /* Start with this size block buffer */
    a->bufSize       = EV_BLOCKSIZE_V4;
    a->bufRealSize   = EV_BLOCKSIZE_V4;
    /* Max # of events/block */
    a->blkEvMax      = EV_EVENTS_MAX_DEF;
    a->blkEvCount    = 0;
    a->isLastBlock   = 0;

    /* file stuff */
    a->baseFileName   = NULL;
    a->fileName       = NULL;
    a->runType        = NULL;
    a->runNumber      = 1;
    a->specifierCount = 0;
    a->splitting      = 0;
    a->splitNumber    = 0;
    a->split          = EV_SPLIT_SIZE;
    a->fileSize       = 0L;
    a->bytesToFile    = 0L;
    a->bytesToBuf     = 4*EV_HDSIZ; /* start off with 1 ending block header */
    a->eventsToBuf    = 0;
    a->eventsToFile   = 0;
    a->currentHeader  = NULL;

    /* buffer stuff */
    a->rwBuf        = NULL;
    a->rwBufSize    = 0;
    /* Total data read/written so far */
    a->rwBytesOut   = 0;
    a->rwBytesIn    = 0;
    a->rwFirstWrite = 1;

    /* socket stuff */
    a->sockFd = 0;

    /* randomAcess stuff */
    a->randomAccess = 0;
    a->mmapFileSize = 0;
    a->mmapFile = NULL;
    a->pTable   = NULL;

    /* dictionary */
    a->wroteDictionary = 0;
    a->dictLength = 0;
    a->dictBuf    = NULL;
    a->dictionary = NULL;

    /* synchronization */
    pthread_mutex_init(&a->lock, NULL);
}


/**
 * Routine to destroy an EVFILE structure.
 * @param a   pointer to structure being inititalized
 */
static void structDestroy(EVFILE *a)
{   /* If it doesn't work, so what? */
    pthread_mutex_destroy(&a->lock);
}


/**
 * Routine to lock the pthread mutex in an EVFILE structure.
 * @param a pointer to EVFILE structure
 */
static void mutexLock(EVFILE *a)
{
    int status = pthread_mutex_lock(&a->lock);
    if (status != 0) {
        evio_err_abort(status, "Failed mutex lock");
    }
}


/**
 * Routine to unlock the pthread mutex in an EVFILE structure.
 * @param a pointer to EVFILE structure
 */
static void mutexUnlock(EVFILE *a)
{
    int status = pthread_mutex_unlock(&a->lock);
    if (status != 0) {
        evio_err_abort(status, "Failed mutex unlock");
    }
}


/**
 * Routine to lock the pthread mutex used for getting and releasing handles.
 * @param a pointer to EVFILE structure
 */
static void getHandleLock(void)
{
#ifndef VXWORKS
    int status;
    status = pthread_mutex_lock(&getHandleMutex);
    if (status != 0) {
        evio_err_abort(status, "Failed get handle lock");
    }
#endif
}


/**
 * Routine to unlock the pthread mutex used for getting and releasing handles.
 * @param a pointer to EVFILE structure
 */
static void getHandleUnlock(void)
{
#ifndef VXWORKS
    int status;
    status = pthread_mutex_unlock(&getHandleMutex);
    if (status != 0) {
        evio_err_abort(status, "Failed get handle unlock");
    }
#endif
}


/** Routine to grab read lock used to prevent simultaneous
 *  calls to evClose and read/write routines. */
static void handleReadLock(int handle) {
#ifndef VXWORKS
    pthread_rwlock_t *handleLock = handleLocks[handle-1];
    int status = pthread_rwlock_rdlock(handleLock);
    if (status != 0) {
        evio_err_abort(status, "Failed handle read lock");
    }
#endif
}


/** Routine to release read lock used to prevent simultaneous
 *  calls to evClose and read/write routines. */
static void handleReadUnlock(int handle) {
#ifndef VXWORKS
    pthread_rwlock_t *handleLock = handleLocks[handle-1];
    int status = pthread_rwlock_unlock(handleLock);
    if (status != 0) {
        evio_err_abort(status, "Failed handle read unlock");
    }
#endif
}


/** Routine to grab write lock used to prevent simultaneous
 *  calls to evClose and read/write routines. */
static void handleWriteLock(int handle) {
#ifndef VXWORKS
    pthread_rwlock_t *handleLock = handleLocks[handle-1];
    int status = pthread_rwlock_wrlock(handleLock);
    if (status != 0) {
        evio_err_abort(status, "Failed handle write lock");
    }
#endif
}


/** Routine to release write lock used to prevent simultaneous
 *  calls to evClose and read/write routines. */
static void handleWriteUnlock(int handle) {
#ifndef VXWORKS
    pthread_rwlock_t *handleLock = handleLocks[handle-1];
    int status = pthread_rwlock_unlock(handleLock);
    if (status != 0) {
        evio_err_abort(status, "Failed handle write unlock");
    }
#endif
}


/**
 * Routine to expand existing storage space for EVFILE structures
 * (one for each evOpen() call).
 * 
 * @return S_SUCCESS if success
 * @return S_EVFILE_ALLOCFAIL if memory cannot be allocated
 */
static int expandHandles()
{
    /* If this is the first initialization, add 100 places for 100 evOpen()'s */
    if (handleCount < 1 || handleList == NULL) {
        int i;
        
        handleCount = 100;

        handleList = (EVFILE **) calloc(handleCount, sizeof(EVFILE *));
        if (handleList == NULL) {
            return S_EVFILE_ALLOCFAIL;
        }

/* Take all the read-write locks out of vxWorks */
#ifndef VXWORKS
        handleLocks = (pthread_rwlock_t **) calloc(handleCount, sizeof(pthread_rwlock_t *));
        if (handleLocks == NULL) {
            return S_EVFILE_ALLOCFAIL;
        }
        /* Initialize new array of mutexes */
        for (i=0; i < handleCount; i++) {
            pthread_rwlock_t *plock = (pthread_rwlock_t *) calloc(1, sizeof(pthread_rwlock_t));
            pthread_rwlock_init(plock, NULL);
            handleLocks[i] = plock;
        }
#endif
    }
    /* We're expanding the exiting arrays */
    else {
        /* Create new, 50% larger arrays */
        int i, newCount = handleCount * 1.5;
        EVFILE **newHandleList;
#ifndef VXWORKS
        pthread_rwlock_t **newHandleLocks;

        newHandleLocks = (pthread_rwlock_t **) calloc(newCount, sizeof(pthread_rwlock_t *));
        if (newHandleLocks == NULL) {
            return S_EVFILE_ALLOCFAIL;
        }
#endif
        
        newHandleList = (EVFILE **) calloc(newCount, sizeof(EVFILE *));
        if (newHandleList == NULL) {
            return S_EVFILE_ALLOCFAIL;
        }

        /* Copy old into new */
        for (i=0; i < handleCount; i++) {
            newHandleList[i]  = handleList[i];
#ifndef VXWORKS
            newHandleLocks[i] = handleLocks[i];
#endif
        }

#ifndef VXWORKS
        /* Initialize the rest */
        for (i=handleCount; i < newCount; i++) {
            pthread_rwlock_t *plock = (pthread_rwlock_t *) calloc(1, sizeof(pthread_rwlock_t));
            pthread_rwlock_init(plock, NULL);
            newHandleLocks[i] = plock;
        }
        
        /* Free the unused arrays */
        free((void *)handleLocks);
#endif
        free((void *)handleList);

        /* Update variables */
        handleCount = newCount;
        handleList  = newHandleList;
#ifndef VXWORKS
        handleLocks = newHandleLocks;
#endif
    }

    return S_SUCCESS;
}



/**
 * Routine to write a specified number of bytes to a TCP socket.
 *
 * @param fd   socket's file descriptor
 * @param vptr pointer to buffer from which bytes are supplied
 * @param n    number of bytes to write
 *
 * @return number of bytes written if successful
 * @return -1 if error and errno is set
 */
static int tcpWrite(int fd, const void *vptr, int n)
{
    int         nleft;
    int         nwritten;
    const char  *ptr;

    
    ptr = (char *) vptr;
    nleft = n;
  
    while (nleft > 0) {
        if ( (nwritten = write(fd, (char*)ptr, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0;       /* and call write() again */
            }
            else {
                return(nwritten);   /* error */
            }
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}


/**
 * Routine to read a specified number of bytes from a TCP socket.
 * If no error, this routine could block until the full number of
 * bytes is read;
 * 
 * @param fd   socket's file descriptor
 * @param vptr pointer to buffer into which bytes go
 * @param n    number of bytes to read
 * 
 * @return number of bytes read if successful
 * @return -1 if error and errno is set
 */
static int tcpRead(int fd, void *vptr, int n)
{
    int   nleft;
    int   nread;
    char  *ptr;

    
    ptr = (char *) vptr;
    nleft = n;
  
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            /*
            if (errno == EINTR)            fprintf(stderr, "call interrupted\n");
            else if (errno == EAGAIN)      fprintf(stderr, "non-blocking return, or socket timeout\n");
            else if (errno == EWOULDBLOCK) fprintf(stderr, "nonblocking return\n");
            else if (errno == EIO)         fprintf(stderr, "I/O error\n");
            else if (errno == EISDIR)      fprintf(stderr, "fd refers to directory\n");
            else if (errno == EBADF)       fprintf(stderr, "fd not valid or not open for reading\n");
            else if (errno == EINVAL)      fprintf(stderr, "fd not suitable for reading\n");
            else if (errno == EFAULT)      fprintf(stderr, "buffer is outside address space\n");
            else {perror("cMsgTcpRead");}
            */
            if (errno == EINTR) {
                nread = 0;      /* and call read() again */
            }
            else {
                return(nread);
            }
        }
        else if (nread == 0) {
            break;            /* EOF */
        }
    
        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);        /* return >= 0 */
}


/**
 * This routine trims white space and nonprintable characters
 * from front and back of the given string. Resulting string is
 * placed at the beginning of the argument. If resulting string
 * is of no length or error, return NULL.
 *
 * @param s    NULL-terminated string to be trimmed
 * @param skip number of bytes (or ASCII characters) to skip over or
 *             remove from string's front end before beginning the trimming
 * @return     argument pointer (s) if successful
 * @return     NULL if no meaningful string can be returned
 */
static char *evTrim(char *s, int skip) {
    int i, len, frontCount=0;
    char *firstChar, *lastChar;

    
    if (s == NULL) return NULL;
    if (skip < 0) skip = 0;

    len = strlen(s + skip);

    if (len < 1) return NULL;
    
    firstChar = s + skip;
    lastChar  = s + skip + len - 1;

    /* Find first front end nonwhite, printable char */
    while (*firstChar != '\0' && (isspace(*firstChar) || !isprint(*firstChar))) {
        firstChar++;
        /* Check to see if string all white space or nonprinting */
        if (firstChar > lastChar) {
            return NULL;
        }
    }
    frontCount = firstChar - s;

    /* Find first back end nonwhite, printable char */
    while (isspace(*lastChar) || !isprint(*lastChar)) {
        lastChar--;
    }

    /* Number of nonwhite chars */
    len = lastChar - firstChar + 1;

    /* Move chars to front of string */
    for (i=0; i < len; i++) {
        s[i] = s[frontCount + i];
    }
    /* Add null at end */
    s[len] = '\0';

    return s;
}


/**
 * Does the file exist or not?
 * @param filename the file's name
 * @return 1 if it exists, else 0
 */
static int fileExists(char *filename) {
    FILE *fp = fopen("filename","r");
    if (fp) {
        /* exists */
        fclose(fp);
        return 1;
    }
    return 0;
}


/**
 * This routine substitutes a given string for a specified substring.
 * Free the result if non-NULL.
 *
 * @param  orig    string to modify
 * @param  replace substring in orig arg to replace
 * @param  with    string to substitute for replace arg
 * @return resulting string which is NULL if there is a problem
 *         and needs to be freed if not NULL.
 */
char *evStrReplace(char *orig, const char *replace, const char *with) {
    
    char *result;  /* return string */
    char *ins;     /* next insert point */
    char *tmp;
    
    size_t len_rep;   /* length of rep  */
    size_t len_with;  /* length of with */
    size_t len_front; /* distance between rep and end of last rep */
    int count;        /* number of replacements */

    
    /* Check string we're changing */
    if (!orig) return NULL;
    ins = orig;
  
    /* What are we replacing? */
    if (!replace) replace = "";
    len_rep = strlen(replace);
    
    /* What are we replacing it with? */
    if (!with) with = "";
    len_with = strlen(with);

    for (count = 0; tmp = strstr(ins, replace); ++count) {
        ins = tmp + len_rep;
    }

    /* First time through the loop, all the variables are set correctly.
     * From here on,
     *    tmp  points to the end of the result string
     *    ins  points to the next occurrence of rep in orig
     *    orig points to the remainder of orig after "end of rep" */
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result) return NULL;

    while (count--) {
        ins = strstr(orig, replace);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; /* move to next "end of rep" */
    }
    
    strcpy(tmp, orig);
    return result;
}


/**
 * This routine finds constructs of the form $(ENV) and replaces it with
 * the value of the ENV environmental variable if it exists or with nothing
 * if it doesn't. Simple enough to do without regular expressions.
 * Free the result if non-NULL.
 *
 * @param orig string to modify
 * @return resulting string which is NULL if there is a problem
 *         and needs to be freed if not NULL.
 */
char *evStrReplaceEnvVar(const char *orig) {
    
    char *start;   /* place of $( */
    char *end;     /* place of  ) */
    char *env;     /* value of env variable */
    char *envVar;  /* env variable name */
    char *result, *tmp;
    char replace[256]; /* string that needs replacing */
    
    size_t len;   /* length of env variable name  */

    
    /* Check string we're changing */
    if (!orig) return NULL;
    
    /* Copy it */
    result = strdup(orig);
    if (!result) return NULL;

    /* Look for all "$(" occurrences */
    while ( (start = strstr(result, "$(")) ) {
        /* Is there an ending ")" ? */
        end = strstr(start, ")");
     
        /* If so ... */
        if (end) {
            /* Length of the env variable name */
            len = end - (start + 2);
            
            /* Memory for name */
            envVar = (char *)calloc(1, len+1);
            if (!envVar) return NULL;
            
            /* Create env variable name */
            strncpy(envVar, start+2, len);
            
            /* Look up value of env variable (NULL if none) */
            env = getenv(envVar);

/*printf("found env variable = %s, value = %s\n", envVar, env);*/
  
            /* Create string to be replaced */
            memset(replace, 0, 256);
            sprintf(replace, "%s%s%s", "$(", envVar, ")");

            /* Do a straight substitution */
            tmp = evStrReplace(result, replace, env);
            free(result);
            free(envVar);
            result = tmp;
        }
        else {
            /* No substitutions need to be made since no closing ")" */
            break;
        }
    }
  
    return result;
}


/**
 * This routine checks a string for C-style printing integer format specifiers.
 * More specifically, it checks for %nd and %nx where n can be multiple digits.
 * It makes sure that there is at least one digit between % and x/d and that the
 * first digit is a "0" so that generated filenames contain no white space.
 * It returns the modified string or NULL if error. Free the result if non-NULL.
 * It also returns the number of valid specifiers found in the orig argument.
 *
 * @param orig           string to be checked/modified
 * @param specifierCount pointer to int which gets filled with the
 *                       number of valid format specifiers in the orig arg
 *
 * @return resulting string which is NULL if there is a problem
 *         and needs to be freed if not NULL.
 */
char *evStrFindSpecifiers(const char *orig, int *specifierCount) {
    
    char *start;            /* place of % */
    char digits[20];        /* digits of format specifier after % */
    char oldSpecifier[25];  /* format specifier as it appears in orig */
    char newSpecifier[25];  /* format specifier desired */
    char c, *result;
    int i, debug=0;
    int count=0;            /* number of valid specifiers in orig */
    int digitCount;         /* number of digits in a specifier between % and x/d */
    

    /* Check args */
    if (!orig || specifierCount == NULL) return NULL;
    
    /* Duplicate orig for convenience */
    result = strdup(orig);
    if (!result) return NULL;
    start = result;
        
    /* Look for C integer printing specifiers (starting with % and using x & d) */
    while ( (start = strstr(start, "%")) ) {
        
        c = *(++start);
        memset(digits, 0, 20);
        digits[0] = c;
        if (debug) printf("Found %, first char = %c\n", c);
        
        /* Read all digits between the % and the first non-digit char */
        digitCount = 0;
        while (isdigit(c)) {
            digits[digitCount++] = c;
            c = *(++start);
            if (debug) printf("         next char = %c\n", c);
        }
        
        /* What is the ending char? */

        /* - skip over any %s specifiers */
        if (c == 's' && digitCount == 0) {
            start++;
            if (debug) printf("         skip over %%s\n");
            continue;
        }

        /* - any thing besides x & d are forbidden */
        if (c != 'x' && c != 'd') {
            /* This is not an acceptable C integer format specifier and
             * may generate a seg fault later, so just return an error. */
            if (debug) printf("         bad ending char = %c, return\n", c);
            free(result);
            return NULL;            
        }
        
        /* Store the x or d */
        digits[digitCount] = c;
        start++;
        
        /* If we're here, we have a valid specifier */
        count++;
        
        /* Is there a "0" as the first digit between the % and the x/d?
         * If not, make it so to avoid white space in generated file names. */
        if (debug) printf("         digit count = %d, first digit = %c\n", digitCount, digits[0]);
        if (digitCount < 1 || digits[0] != '0') {
            /* Recreate original specifier */
            memset(oldSpecifier, 0, 25);
            sprintf(oldSpecifier, "%%%s", digits);
           
            /* Create replacement specifier */
            memset(newSpecifier, 0, 25);
            sprintf(newSpecifier, "%%%c%s", '0', digits);
            if (debug) printf("         old specifier = %s, new specifier = %s\n",
                               oldSpecifier, newSpecifier);
       
            /* Replace old with new & start over */
            start = evStrReplace(result, oldSpecifier, newSpecifier);
            free(result);
            result = start;
            count = 0;
        }
    }
    
    *specifierCount = count;
    
    return result;    
}


/**
 * This routine checks a string for C-style printing integer format specifiers.
 * More specifically, it checks for %nd and %nx where n can be multiple digits.
 * It removes all such specifiers and returns the modified string or NULL
 * if error. Free the result if non-NULL.
 *
 * @param orig string to be checked/modified
 *
 * @return resulting string which is NULL if there is a problem
 *         and needs to be freed if not NULL.
 */
char *evStrRemoveSpecifiers(const char *orig) {
    
    char *pStart;           /* pointer to start of string */
    char *pChar;            /* pointer to char somewhere in string */
    char c, *result, *pSpec;
    int i, startIndex, endIndex, debug=0;
    int specLen;           /* number of chars in valid specifier in orig */
    int digitCount;        /* number of digits in a specifier between % and x/d */
    

    /* Check args */
    if (!orig) return NULL;
    
    /* Duplicate orig for convenience */
    pChar = pStart = result = strdup(orig);
    if (!result) return NULL;

    /* Look for C integer printing specifiers (starting with % and using x & d) */
    while (pChar = strstr(pChar, "%")) {
        /* remember where specifier starts */
        pSpec = pChar;
        
        c = *(++pChar);
        if (debug) printf("evStrRemoveSpecifiers found %, first char = %c\n", c);
        
        /* Read all digits between the % and the first non-digit char */
        digitCount = 0;
        while (isdigit(c)) {
            digitCount++;
            c = *(++pChar);
            if (debug) printf("         next char = %c\n", c);
        }
        
        /* What is the ending char? */

        /* - skip over any %s specifiers */
        if (c == 's' && digitCount == 0) {
            pChar++;
            if (debug) printf("         skip over %%s\n");
            continue;
        }

        /* - any thing besides x & d are forbidden */
        if (c != 'x' && c != 'd') {
            /* This is not an acceptable C integer format specifier and
            * may generate a seg fault later, so just return an error. */
            if (debug) printf("         bad ending char = %c, return\n", c);
            free(result);
            return NULL;
        }
        
        pChar++;

        /* # of chars in specifier */
        specLen = pChar - pSpec;
        if (debug) printf("         spec len = %d\n", specLen);

        /* shift chars to eliminate specifier */
        startIndex = pSpec-pStart;
        endIndex = strlen(result) + 1 - specLen - startIndex; /* include NULL in move */
        for (i=0; i < endIndex; i++) {
            result[i+startIndex] = result[i+startIndex+specLen];
        }
        if (debug) printf("         resulting string = %s\n", result);
    }
    
    return result;
}



/**
 * This routine generates a (base) file name from a name containing format specifiers
 * and enviromental variables.<p>
 *
 * The given name may contain up to 2, C-style integer format specifiers
 * (such as <b>%03d</b>, or <b>%x</b>). If more than 2 are found, an error is returned.
 * 
 * If no "0" precedes any integer between the "%" and the "d" or "x" of the format specifier,
 * it will be added automatically in order to avoid spaces in the final, generated
 * file name.
 * In the {@link #evGenerateFileName(String, int, int, int, int)} routine, the first
 * occurrence will be substituted with the given runNumber value.
 * If the file is being split, the second will be substituted with the split number.<p>
 *
 * The file name may contain characters of the form <b>$(ENV_VAR)</b>
 * which will be substituted with the value of the associated environmental
 * variable or a blank string if none is found.<p>
 *
 *
 * @param origName  file name to modify
 * @param baseName  pointer which gets filled with resulting file name
 * @param count     pointer to int filled with number of format specifiers found
 *
 * @return S_SUCCESS          if successful
 * @return S_FAILURE          if bad format specifiers or more that 2 specifiers found
 * @return S_EVFILE_BADARG    if args are null or origName is invalid
 * @return S_EVFILE_ALLOCFAIL if out-of-memory
 */
int evGenerateBaseFileName(char *origName, char **baseName, int *count) {

    char *name, *tmp;
    int   specifierCount=0;

    
    /* Check args */
    if (count == NULL    || baseName == NULL ||
        origName == NULL || strlen(origName) < 1) {
        return(S_EVFILE_BADARG);
    }

    /* Scan for environmental variables of the form $(env)
     * and substitute the values for them (blank string if not found) */
    tmp = evStrReplaceEnvVar(origName);
    if (tmp == NULL) {
        /* out-of-mem */
        return(S_EVFILE_ALLOCFAIL);
    }

    /* Check/fix C-style int specifiers in baseFileName.
     * How many specifiers are there? */
    name = evStrFindSpecifiers(tmp, &specifierCount);
    if (name == NULL) {
        free(tmp);
        /* out-of-mem or bad specifier */ 
        return(S_EVFILE_ALLOCFAIL);
    }
    free(tmp);

    if (specifierCount > 2) {
        return(S_FAILURE);
    }

    /* Return the base file name */
    *baseName = name;

    /* Return the number of C-style int format specifiers */
    *count = specifierCount;
    
    return(S_SUCCESS);
}


/**
 * This method generates a complete file name from the previously determined baseFileName
 * obtained from calling {@link #evGenerateBaseFileName} and stored in the evOpen handle.<p>
 *
 * All occurrences of the string "%s" in the baseFileName will be substituted with the
 * value of the runType arg or nothing if the runType is null.<p>
 * 
 * If evio data is to be split up into multiple files (split > 0), numbers are used to
 * distinguish between the split files with splitNumber.
 * If baseFileName contains C-style int format specifiers (specifierCount > 0), then
 * the first occurrence will be substituted with the given runNumber value.
 * If the file is being split, the second will be substituted with the splitNumber.
 * If 2 specifiers exist and the file is not being split, no substitutions are made.
 * If no specifier for the splitNumber exists, it is tacked onto the end of the file name.
 * It returns the final file name or NULL if error. Free the result if non-NULL.
 *
 * @param handle         evio handle (contains file name to use as a basis for the
 *                       generated file name)
 * @param specifierCount number of C-style int format specifiers in file name arg
 * @param runNumber      CODA run number
 * @param split          number of bytes at which to split off evio file (<= 0 if not)
 * @param splitNumber    number of the split file
 * @param runType        run type name
 *
 * @return NULL if error
 * @return generated file name (free if non-NULL)
 */
char *evGenerateFileName(EVFILE *a, int specifierCount, int runNumber,
                         int split, int splitNumber, char *runType) {

    char   *fileName, *name, *specifier;

    
    /* Check args */
    if ( (split > 0 && splitNumber < 0) || (runNumber < 1) ||
         (specifierCount < 0) || (specifierCount > 2)) {
        return(NULL); /* S_EVFILE_BADARG */
    }

    /* Check handle arg */
    if (a == NULL) {
        return(NULL); /* S_EVFILE_BADHANDLE */
    }

    /* Need to be writing to a file */
    if (a->rw != EV_WRITEFILE) {
        return(NULL); /* S_EVFILE_BADMODE */
    }

    /* Internal programming bug */
    if (a->baseFileName == NULL) {
        return(NULL); /* S_FAILURE */
    }
    
    /* Replace all %s occurrences with run type ("" if NULL).
     * This needs to be done before the run # & split # substitutions. */
    name = evStrReplace(a->baseFileName, "%s", runType);
    if (name == NULL) {
        /* out-of-mem */
        return(NULL); /* S_FAILURE */
    }
    free(a->baseFileName);
    a->baseFileName = name;

   /* As far as memory goes, allow 10 digits for the run number and 10 for the split.
     * That will cover 32 bit ints. */

    /* If we're splitting files ... */
    if (split > 0) {
        
        /* For no specifiers: tack split # on end of base file name */
        if (specifierCount < 1) {
            /* Create space for filename */
            fileName = (char *)calloc(1, strlen(a->baseFileName) + 12);
            if (!fileName) return(NULL); /* S_FAILURE */
            
            /* Create a printing format specifier for split # */
            specifier = (char *)calloc(1, strlen(a->baseFileName) + 6);
            if (!specifier) return(NULL); /* S_FAILURE */
            
            sprintf(specifier, "%s.%s", a->baseFileName, "%d");

            /* Create the filename */
            sprintf(fileName, specifier, splitNumber);
            free(specifier);
        }
        
        /* For 1 specifier: insert run # at specified location, then tack split # on end */
        else if (specifierCount == 1) {
            /* Create space for filename */
            fileName = (char *)calloc(1, strlen(a->baseFileName) + 22);
            if (!fileName) return(NULL); /* S_FAILURE */
            
            /* The first printfing format specifier is in a->filename already */
            
            /* Create a printing format specifier for split # */
            specifier = (char *)calloc(1, strlen(a->baseFileName) + 6);
            if (!specifier) return(NULL); /* S_FAILURE */
            
            sprintf(specifier, "%s.%s", a->baseFileName, "%d");

            /* Create the filename */
            sprintf(fileName, specifier, runNumber, splitNumber);
            free(specifier);
        }
        
        /* For 2 specifiers: insert run # and split # at specified locations */
        else {
            /* Create space for filename */
            fileName = (char *)calloc(1, strlen(a->baseFileName) + 21);
            if (!fileName) return(NULL); /* S_FAILURE */
            
            /* Both printfing format specifiers are in a->filename already */
                        
            /* Create the filename */
            sprintf(fileName, a->baseFileName, runNumber, splitNumber);
        }
    }
    /* If we're not splitting files ... */
    else {
        /* Still insert runNumber if requested */
        if (specifierCount == 1) {
            /* Create space for filename */
            fileName = (char *)calloc(1, strlen(a->baseFileName) + 11);
            if (!fileName) return(NULL); /* S_FAILURE */
            
            /* The printfing format specifier is in a->filename already */
                        
            /* Create the filename */
            sprintf(fileName, a->baseFileName, runNumber);
        }
        /* For 2 specifiers: insert run # and remove split # specifier */
        else if (specifierCount == 2) {
            fileName = (char *)calloc(1, strlen(a->baseFileName) + 11);
            if (!fileName) return(NULL); /* S_FAILURE */
            sprintf(fileName, a->baseFileName, runNumber);

            /* Get rid of remaining int specifiers */
            name = evStrRemoveSpecifiers(fileName);
            free(fileName);
            fileName = name;
        }
        else {
            fileName = strdup(a->baseFileName);
        }
    }

    return fileName;
}


/** Fortran interface to {@link evOpen}. */
#ifdef AbsoftUNIXFortran
int evopen
#else
int evopen_
#endif
(char *filename, char *flags, int *handle, int fnlen, int flen)
{
    char *fn, *fl;
    int status;
    fn = (char *) malloc(fnlen+1);
    strncpy(fn,filename,fnlen);
    fn[fnlen] = 0;      /* insure filename is null terminated */
    fl = (char *) malloc(flen+1);
    strncpy(fl,flags,flen);
    fl[flen] = 0;           /* insure flags is null terminated */
    status = evOpen(fn,fl,handle);
    free(fn);
    free(fl);
    return(status);
}



/**
 * This function opens a file for either reading or writing evio format data.
 * Works with all versions of evio for reading, but only writes version 4
 * format. A handle is returned for use with calling other evio routines.
 *
 * @param filename  name of file. Constructs of the form $(env) will be substituted
 *                  with the given environmental variable or removed if nonexistent.
 *                  Constructs of the form %s will be substituted with the run type
 *                  if specified in {@link evIoctl} or removed if nonexistent.
 *                  Up to 2, C-style int format specifiers are allowed. The first is
 *                  replaced with the run number (set in {@link evIoctl}). If splitting,
 *                  the second is replaced by the split number, otherwise it's removed.
 *                  If splitting and no second int specifier exists, a "." and split
 *                  number are automatically appended to the end of the file name.
 * @param flags     pointer to case-independent string of "w" for writing,
 *                  "r" for reading, "a" for appending, "ra" for random
 *                  access reading of a file, or "s" for splitting a file
 *                  while writing
 * @param handle    pointer to int which gets filled with handle
 *
 * @return S_SUCCESS          if successful
 * @return S_FAILURE          when splitting, if bad format specifiers or more that 2 specifiers found
 * @return S_EVFILE_BADARG    if filename, flags or handle arg is NULL; unrecognizable flags;
 * @return S_EVFILE_ALLOCFAIL if memory allocation failed
 * @return S_EVFILE_BADFILE   if error reading file, unsupported version,
 *                            or contradictory data in file
 * @return errno              if file could not be opened (handle = 0)
 */
int evOpen(char *filename, char *flags, int *handle)
{
    if (strcasecmp(flags, "w")  != 0 &&
        strcasecmp(flags, "s")  != 0 &&
        strcasecmp(flags, "r")  != 0 &&
        strcasecmp(flags, "a")  != 0 &&
        strcasecmp(flags, "ra") != 0)  {
        return(S_EVFILE_BADARG);
    }
    
    return(evOpenImpl(filename, 0, 0, flags, handle));
}



/**
 * This function allows for either reading or writing evio format data from a buffer.
 * Works with all versions of evio for reading, but only writes version 4
 * format. A handle is returned for use with calling other evio routines.
 *
 * @param buffer  pointer to buffer
 * @param bufLen  length of buffer in 32 bit ints
 * @param flags   pointer to case-independent string of "w", "r", "a", or "ra"
 *                for writing/reading/appending/random-access-reading to/from a buffer.
 * @param handle  pointer to int which gets filled with handle
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_BADARG    if buffer, flags or handle arg is NULL; unrecognizable flags;
 *                            buffer size too small
 * @return S_EVFILE_ALLOCFAIL if memory allocation failed
 * @return S_EVFILE_BADFILE   if error reading buffer, unsupported version,
 *                            or contradictory data
 */
int evOpenBuffer(char *buffer, uint32_t bufLen, char *flags, int *handle)
{
    char *flag;
    
    
    /* Check flags & translate them */
    if (strcasecmp(flags, "w") == 0) {
        flag = "wb";
    }
    else if (strcasecmp(flags, "r") == 0) {
        flag = "rb";
    }
    else if (strcasecmp(flags, "a") == 0) {
        flag = "ab";
    }
    else if (strcasecmp(flags, "ra") == 0) {
        flag = "rab";
    }
    else {
        return(S_EVFILE_BADARG);
    }
    
    return(evOpenImpl(buffer, bufLen, 0, flag, handle));
}


/**
 * This function allows for either reading or writing evio format data from a TCP socket.
 * Works with all versions of evio for reading, but only writes version 4
 * format. A handle is returned for use with calling other evio routines.
 *
 * @param sockFd  TCP socket file descriptor
 * @param flags   pointer to case-independent string of "w" & "r" for
 *                writing/reading to/from a socket
 * @param handle  pointer to int which gets filled with handle
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_BADARG    if flags or handle arg is NULL; bad file descriptor arg;
 *                            unrecognizable flags
 * @return S_EVFILE_ALLOCFAIL if memory allocation failed
 * @return S_EVFILE_BADFILE   if error reading socket, unsupported version,
 *                            or contradictory data
 * @return errno              if socket read error
 */
int evOpenSocket(int sockFd, char *flags, int *handle)
{
    char *flag;

    
    /* Check flags & translate them */
    if (strcasecmp(flags, "w") == 0) {
        flag = "ws";
    }
    else if (strcasecmp(flags, "r") == 0) {
        flag = "rs";
    }
    else {
        return(S_EVFILE_BADARG);
    }

    return(evOpenImpl((char *)NULL, 0, sockFd, flag, handle));
}

/* For test purposes only ... */
int evOpenFake(char *filename, char *flags, int *handle, char **evf)
{
    EVFILE *a;
    int ihandle;
    
    
    a = (EVFILE *)calloc(1, sizeof(EVFILE));
    structInit(a);

     for (ihandle=0; ihandle < handleCount; ihandle++) {
        if (handleList[ihandle] == 0) {
            handleList[ihandle] = a;
            *handle = ihandle+1;
            break;
        }
    }
    
    a->rw = EV_WRITEFILE;
    a->baseFileName = filename;
    *evf = (char *)a;
    
    return(S_SUCCESS);
}
/**/
    
/**
 * This function opens a file, socket, or buffer for either reading or writing
 * evio format data.
 * Works with all versions of evio for reading, but only writes version 4
 * format. A handle is returned for use with calling other evio routines.
 *
 * @param srcDest name of file if flags = "w", "s" or "r";
 *                pointer to buffer if flags = "wb" or "rb"
 * @param bufLen  length of buffer (srcDest) if flags = "wb" or "rb"
 * @param sockFd  socket file descriptor if flags = "ws" or "rs"
 * @param flags   pointer to case-independent string:
 *                "w", "s, "r", "a", & "ra"
 *                for writing/splitting/reading/append/random-access-reading to/from a file;
 *                "ws" & "rs"
 *                for writing/reading to/from a socket;
 *                "wb", "rb", "ab", & "rab"
 *                for writing/reading/append/random-access-reading to/from a buffer.
 * @param handle  pointer to int which gets filled with handle
 * 
 * @return S_SUCCESS          if successful
 * @return S_FAILURE          if code compiled so that block header < 8, 32bit words long;
 *                            when splitting, if bad format specifiers or more that 2 specifiers found
 * @return S_EVFILE_BADARG    if flags or handle arg is NULL;
 *                            if srcDest arg is NULL when using file or buffer;
 *                            if unrecognizable flags;
 *                            if buffer size too small when using buffer;
 *                            if specifying random access on vxWorks or Windows
 * @return S_EVFILE_ALLOCFAIL if memory allocation failed
 * @return S_EVFILE_BADFILE   if error reading file, unsupported version,
 *                            or contradictory data in file
 * @return S_EVFILE_UNXPTDEOF if reading buffer which is too small
 * @return errno              for file opening or socket reading problems
 */
static int evOpenImpl(char *srcDest, uint32_t bufLen, int sockFd, char *flags, int *handle)
{
    EVFILE *a;
    char *filename, *buffer, *baseName;

    uint32_t blk_size, temp, headerInfo, blkHdrSize, header[EV_HDSIZ];
    uint32_t rwBufSize, nBytes, bytesToRead;

    int i, err, version, ihandle;
    int debug=0, useFile=0, useBuffer=0, useSocket=0;
    int reading=0, randomAccess=0, append=0, splitting=0, specifierCount=0;

    
    /* Check args */
    if (flags == NULL || handle == NULL) {
        return(S_EVFILE_BADARG);
    }

    /* Check to see if someone set the length of the block header to be too small. */
    if (EV_HDSIZ < 8) {
if (debug) printf("EV_HDSIZ in evio.h set to be too small (%d). Must be >= 8.\n", EV_HDSIZ);
        return(S_FAILURE);
    }

    /* Are we dealing with a file, buffer, or socket? */
    if (strcasecmp(flags, "w")  == 0  ||
        strcasecmp(flags, "s")  == 0  ||
        strcasecmp(flags, "r")  == 0  ||
        strcasecmp(flags, "a")  == 0  ||
        strcasecmp(flags, "ra") == 0 )  {
        
        useFile = 1;
        
        if      (strcasecmp(flags,  "a") == 0) append = 1;
        else if (strcasecmp(flags,  "s") == 0) splitting = 1;
        else if (strcasecmp(flags, "ra") == 0) randomAccess = 1;

#if defined VXWORKS || defined _MSC_VER
        /* No random access support in vxWorks or Windows */
        if (randomAccess) {
            return(S_EVFILE_BADARG);
        }
#endif
        if (srcDest == NULL) {
            return(S_EVFILE_BADARG);
        }

        filename = strdup(srcDest);
        if (filename == NULL) {
            return(S_EVFILE_ALLOCFAIL);
        }
        
        /* Trim whitespace from filename front & back */
        evTrim(filename, 0);
    }
    else if (strcasecmp(flags, "wb")  == 0  ||
             strcasecmp(flags, "rb")  == 0  ||
             strcasecmp(flags, "ab")  == 0  ||
             strcasecmp(flags, "rab") == 0 )  {
        
        useBuffer = 1;
        
        if      (strcasecmp(flags, "ab")  == 0) append = 1;
        else if (strcasecmp(flags, "rab") == 0) randomAccess = 1;

        if (srcDest == NULL) {
            return(S_EVFILE_BADARG);
        }

        buffer    = srcDest;
        rwBufSize = 4*bufLen;
        /* Smallest possible evio V4 buffer with data =
         * block header (4*8) + evio bank (4*3) */
        if (rwBufSize < 4*11) {
            return(S_EVFILE_BADARG);
        }
    }
    else if (strcasecmp(flags, "ws") == 0 ||
             strcasecmp(flags, "rs") == 0)  {
        
        useSocket = 1;
        if (sockFd < 0) {
            return(S_EVFILE_BADARG);
        }
    }
    else {
        return(S_EVFILE_BADARG);
    }

    
    /* Are we reading or writing? */
    if (strncasecmp(flags, "r", 1) == 0) {
        reading = 1;
    }
  
    
    /* Allocate control structure (mem zeroed) or quit */
    a = (EVFILE *)calloc(1, sizeof(EVFILE));
    if (!a) {
        if (useFile) {
            free(filename);
        }
        return(S_EVFILE_ALLOCFAIL);
    }
    /* Initialize newly allocated structure */
    structInit(a);

    
    /*********************************************************/
    /* If we're reading a version 1-4 file/socket/buffer ... */
    /*********************************************************/
    if (reading) {
        
        if (useFile) {
#if defined VXWORKS || defined _MSC_VER
            /* No pipe or zip/unzip support in vxWorks */
            a->file = fopen(filename,"r");
            a->rw = EV_READFILE;
            a->randomAccess = randomAccess = 0;
#else
            a->rw = EV_READFILE;
            a->randomAccess = randomAccess;

            if (strcmp(filename,"-") == 0) {
                a->file = stdin;
            }
            /* If input filename is standard output of command ... */
            else if (filename[0] == '|') {
                /* Open a process by creating a unidirectional pipe, forking, and
                   invoking the shell. The "filename" is a shell command line.
                   This command is passed to /bin/sh using the -c flag.
                   Make sure we know to use pclose instead of fclose. */
                a->file = popen(filename+1,"r");
                a->rw = EV_READPIPE;
if (debug) printf("evOpen: reading from pipe %s\n", filename + 1);
            }
            else if (randomAccess) {
                err = memoryMapFile(a, filename);
                if (err != S_SUCCESS) {
                    free(filename);
                    return(errno);
                }
            }
            else {
                a->file = fopen(filename,"r");
            }
#endif
            if (randomAccess) {
                /* Read (copy) in header */
                nBytes = sizeof(header);
                memcpy((void *)header, (const void *)a->mmapFile, nBytes);
            }
            else {
                if (a->file == NULL) {
                    free(a);
                    *handle = 0;
                    free(filename);
                    return(errno);
                }

                /* Read in header */
                nBytes = fread(header, 1, sizeof(header), a->file);
            }
        }
        else if (useSocket) {
            a->sockFd = sockFd;
            a->rw = EV_READSOCK;
            
            /* Read in header */
            nBytes = tcpRead(sockFd, header, sizeof(header));
            if (nBytes < 0) return(errno);
        }
        else if (useBuffer) {
            a->rwBuf = buffer;
            a->rw = EV_READBUF;
            a->rwBufSize = rwBufSize;
            
            /* Read (copy) in header */
            nBytes = sizeof(header);
//TODO: get rid of this copy!
            memcpy((void *)header, (const void *)(a->rwBuf), nBytes);
            a->rwBytesIn += nBytes;
        }

        /**********************************/
        /* Run header through some checks */
        /**********************************/
if (debug) {
    int j;
    char *str;
    for (j=0; j < 8; j++) {
        printf("header[%d] = 0x%x\n", j, header[j]);
    }
    /*
    str = (char *)header;
    printf("header as string = %s\n", str);
    */
}

        /* Check to see if all bytes are there */
        if (nBytes != sizeof(header)) {
            /* Close file and free memory */
            if (useFile) {
                localClose(a);
                free(filename);
            }
            structDestroy(a);
            free(a);
            return(S_EVFILE_BADFILE);
        }

        /* Check endianness */
        if (header[EV_HD_MAGIC] != EV_MAGIC) {
            temp = EVIO_SWAP32(header[EV_HD_MAGIC]);
            if (temp == EV_MAGIC) {
                a->byte_swapped = 1;
            }
            else {
if (debug) printf("Magic # is a bad value\n");
                if (useFile) {
                    localClose(a);
                    free(filename);
                }
                structDestroy(a);
                free(a);
                return(S_EVFILE_BADFILE);
            }
        }
        else {
            a->byte_swapped = 0;
        }

        /* Check VERSION */
        headerInfo = header[EV_HD_VER];
        if (a->byte_swapped) {
            headerInfo = EVIO_SWAP32(headerInfo);
        }
        /* Only lowest 8 bits count in version 4's header word */
        version = headerInfo & EV_VERSION_MASK;
        if (version < 1 || version > 4) {
if (debug) printf("Header has unsupported evio version (%d), quit\n", version);
            if (useFile) {
                localClose(a);
                free(filename);
            }
            structDestroy(a);
            free(a);
            return(S_EVFILE_BADFILE);
        }
        a->version = version;

        /* Check the header's value for header size against our assumption. */
        blkHdrSize = header[EV_HD_HDSIZ];
        if (a->byte_swapped) {
            blkHdrSize = EVIO_SWAP32(blkHdrSize);
        }

        /* If actual header size not what we're expecting ... */
        if (blkHdrSize != EV_HDSIZ) {
            int restOfHeader = blkHdrSize - EV_HDSIZ;
if (debug) printf("Header size was assumed to be %d but it was actually %u\n", EV_HDSIZ, blkHdrSize);
            if (restOfHeader < 0) {
if (debug) printf("Header size is too small (%u), return error\n", blkHdrSize);
                if (useFile) {
                    localClose(a);
                    free(filename);
                }
                structDestroy(a);
                free(a);
                return(S_EVFILE_BADFILE);
            }
        }

        if (randomAccess) {
            /* Random access only available for version 4+ */
            if (version < 4) {
                return(S_EVFILE_BADFILE);
            }

            /* Find pointers to all the events (skips over any dictionary) */
            err = generatePointerTable(a);
            if (err != S_SUCCESS) {
                return(err);
            }
        }
        else {
            /**********************************/
            /* Allocate buffer to store block */
            /**********************************/
    
            /* size of block we're reading */
            blk_size = header[EV_HD_BLKSIZ];
            if (a->byte_swapped) {
                blk_size = EVIO_SWAP32(blk_size);
            }
            a->blksiz = blk_size;

            /* How big do we make this buffer? Use a minimum size. */
            a->bufSize = blk_size < EV_BLOCKSIZE_MIN ? EV_BLOCKSIZE_MIN : blk_size;
            a->buf = (uint32_t *)malloc(4*a->bufSize);
    
            /* Error if can't allocate buffer memory */
            if (a->buf == NULL) {
                if (useFile) {
                    localClose(a);
                    free(filename);
                }
                structDestroy(a);
                free(a);
                return(S_EVFILE_ALLOCFAIL);
            }
    
            /* Copy header (the part we read in) into block (swapping if necessary) */
            if (a->byte_swapped) {
                swap_int32_t((uint32_t *)header, EV_HDSIZ, (uint32_t *)a->buf);
            }
            else {
                memcpy(a->buf, header, 4*EV_HDSIZ);
            }
    
            /*********************************************************/
            /* Read rest of block & any remaining, over-sized header */
            /*********************************************************/
            bytesToRead = 4*(blk_size - EV_HDSIZ);
            if (useFile) {
                nBytes = fread(a->buf+EV_HDSIZ, 1, bytesToRead, a->file);
            }
            else if (useSocket) {
                nBytes = tcpRead(sockFd, a->buf+EV_HDSIZ, bytesToRead);
                if (nBytes < 0) {
                    free(a->buf);
                    free(a);
                    return(errno);
                }
            }
            else if (useBuffer) {
                nBytes = bytesToRead;
                memcpy(a->buf+EV_HDSIZ, (a->rwBuf + a->rwBytesIn), bytesToRead);
                a->rwBytesIn += bytesToRead;
            }
    
            /* Check to see if all bytes were read in */
            if (nBytes != bytesToRead) {
                if (useFile) {
                    localClose(a);
                    free(filename);
                }            
                structDestroy(a);
                free(a->buf);
                free(a);
                return(S_EVFILE_BADFILE);
            }
    
            if (version < 4) {
                /* Pointer to where start of first event header occurs. */
                a->next = a->buf + (a->buf)[EV_HD_START];
                /* Number of valid 32 bit words from start of first event to end of block */
                a->left = (a->buf)[EV_HD_USED] - (a->buf)[EV_HD_START];
            }
            else {
                /* Pointer to where start of first event header occurs =
                 * right after header for version 4+. */
                a->next = a->buf + blkHdrSize;
                
                /* Number of valid 32 bit words = (full block size - header size) in v4+ */
                a->left = (a->buf)[EV_HD_BLKSIZ] - blkHdrSize;
            
                /* Is this the last block? */
                a->isLastBlock = isLastBlock(a->buf);
                
                /* Pull out dictionary if there is one (works only after header is swapped). */
                if (hasDictionary(a->buf)) {
                    int status;
                    uint32_t buflen;
                    uint32_t *buf;
    
                    /* Read in first bank which will be dictionary */
                    status = evReadAllocImpl(a, &buf, &buflen);
                    if (status == S_SUCCESS) {
                        /* Trim off any whitespace/padding, skipping over event header (8 bytes) */
                        a->dictionary = evTrim((char *)buf, 8);
                    }
                    else if (debug) {
printf("ERROR retrieving DICTIONARY, status = %#.8x\n", status);
                    }
                }
            }
        }
    }
        
    /*************************/
    /* If we're writing  ... */
    /*************************/
    else {

        a->append = append;

        if (useFile) {
#if defined  VXWORKS || defined _MSC_VER
            a->fileName = strdup(filename);
            a->file = fopen(filename,"w");
            a->rw = EV_WRITEFILE;
#else
            a->rw = EV_WRITEFILE;
            if (strcmp(filename,"-") == 0) {
                a->file = stdout;
            }
            else if (filename[0] == '|') {
if (debug) printf("evOpen: writing to pipe %s\n", filename + 1);
                fflush(NULL); /* recommended for writing to pipe */
                a->file = popen(filename+1,"w");
                a->rw = EV_WRITEPIPE;	/* Make sure we know to use pclose */
            }
            else if (append) {
                /* Must be able to read & write since we may
                 * need to write over last block header. Do NOT
                 * truncate (erase) the file here! */
                a->file = fopen(filename,"r+");
                
                /* Read in header */
                nBytes = sizeof(header)*fread(header, sizeof(header), 1, a->file);
                
                /* Check to see if read the whole header */
                if (nBytes != sizeof(header)) {
                    /* Close file and free memory */
                    fclose(a->file);
                    free(filename);
                    free(a);
                    return(S_EVFILE_BADFILE);
                }
            }
            else {
                err = evGenerateBaseFileName(filename, &baseName, &specifierCount);
                if (err != S_SUCCESS) {
                    *handle = 0;
                    free(a);
                    free(filename);
                    return(err);
                }
                if (splitting) a->splitting = 1;
                a->baseFileName   = baseName;
                a->specifierCount = specifierCount;
            }
#endif
        }
        else if (useSocket) {
            a->sockFd = sockFd;
            a->rw = EV_WRITESOCK;
        }
        else if (useBuffer) {
            a->rwBuf = buffer;
            a->rw = EV_WRITEBUF;
            a->rwBufSize = rwBufSize;

            /* if appending, read in first header */
            if (append) {
                nBytes = sizeof(header);
                /* unexpected EOF or end-of-buffer in this case */
                if (rwBufSize < nBytes) {
                    free(a);
                    return(S_EVFILE_UNXPTDEOF);
                }
                /* Read (copy) in header */
                memcpy(header, a->rwBuf, nBytes);
            }
        }


        /*********************************************************************/
        /* If we're appending, we already read in the first block header     */
        /* so check a few things like version number and endianness.         */
        /*********************************************************************/
        if (append) {
            /* Check endianness */
            if (header[EV_HD_MAGIC] != EV_MAGIC) {
                temp = EVIO_SWAP32(header[EV_HD_MAGIC]);
                if (temp == EV_MAGIC) {
                    a->byte_swapped = 1;
                }
                else {
if (debug) printf("Magic # is a bad value\n");
                    if (useFile) {
                        if (a->rw == EV_WRITEFILE) {
                            fclose(a->file);
                        }
                        free(filename);
                    }
                    free(a);
                    return(S_EVFILE_BADFILE);
                }
            }
            else {
                a->byte_swapped = 0;
            }

            /* Check VERSION */
            headerInfo = header[EV_HD_VER];
            if (a->byte_swapped) {
                headerInfo = EVIO_SWAP32(headerInfo);
            }
            /* Only lowest 8 bits count in version 4's header word */
            version = headerInfo & EV_VERSION_MASK;
            if (version != EV_VERSION) {
if (debug) printf("File must be evio version %d (not %d) for append mode, quit\n", EV_VERSION, version);
                if (useFile) {
                    if (a->rw == EV_WRITEFILE) {
                        fclose(a->file);
                    }
                    free(filename);
                }
                free(a);
                return(S_EVFILE_BADFILE);
            }
        }
        
        /* Allocate memory for a block only if we are not writing to a buffer. */
        if (!useBuffer) {
            /* bufSize is EV_BLOCKSIZE_V4 by default, see structInit() */
            a->buf = (uint32_t *) calloc(1,4*a->bufRealSize);
            if (!(a->buf)) {
                if (useFile) {
                    if (a->file != NULL) {
                        if (a->rw == EV_WRITEFILE) {
                            fclose(a->file);
                        }
                        else if (a->rw == EV_WRITEPIPE) {
                            pclose(a->file);
                        }
                    }
                    free(filename);
                }
                free(a);
                return(S_EVFILE_ALLOCFAIL);
            }

            /* Initialize block header (as last block) */
            initLastBlockHeader(a->buf, 1);
        }
        /* If writing to buffer, skip step of writing to separate block buffer first. */
        else {
            /* If not appending, get the block header in the buffer set up.
             * The equivalent is done in toAppendPosition() when appending. */
            if (!append) {
                /* Block header is at beginning of buffer */
                a->buf = (uint32_t*) a->rwBuf;

                /* Initialize block header as the last one to be written */
                initLastBlockHeader(a->buf, 1);
                a->isLastBlock  = 1;

                /* # of bytes "written" - just the block header so far */
                a->rwBytesOut = 4*EV_HDSIZ;
            }
        }

        /* Set position in file stream / buffer for next write.
         * If not appending this is does nothing. */
        err = toAppendPosition(a);
        if (err != S_SUCCESS) {
            return(err);
        }
        
        a->currentHeader = a->buf;
   
        /* Pointer to where next to write. In this case, the start of the
         * first event header will be right after first block header. */
        a->next = a->buf + EV_HDSIZ;

    } /* if writing */
    
    /* Store general info in handle structure */
    if (!randomAccess) a->blknum = a->buf[EV_HD_BLKNUM];

    /* Don't let no one else get no "a" while we're openin' somethin' */
    getHandleLock();

    /* Do the first-time initialization */
    if (handleCount < 1 || handleList == NULL) {
        expandHandles();
    }

    {
        int gotHandle = 0;
        
        for (ihandle=0; ihandle < handleCount; ihandle++) {
            /* If a slot is available ... */
            if (handleList[ihandle] == NULL) {
                handleList[ihandle] = a;
                *handle = ihandle + 1;
                /* store handle in structure for later convenience */
                a->handle = ihandle + 1;
                gotHandle = 1;
                break;
            }
        }
    
        /* If no available handles left ... */
        if (!gotHandle) {
            /* Remember old handle count */
            int oldHandleLimit = handleCount;
            /* Create 50% more handles (and increase handleCount) */
            expandHandles();

            /* Use a newly created handle */
            ihandle = oldHandleLimit;
            handleList[ihandle] = a;
            *handle = ihandle + 1;
            a->handle = ihandle + 1;
        }
    }
    
    getHandleUnlock();
    
    if (useFile) free(filename);
    
    return(S_SUCCESS);
}


/**
 * This routine closes any open files and unmaps any mapped memory.
 * @param handle evio handle
 */
static void localClose(EVFILE *a)
{
    /* Close file */
    if (a->rw == EV_WRITEFILE) {
        if (a->file != NULL) fclose(a->file);
    }
    else if (a->rw == EV_READFILE) {
        if (a->randomAccess) {
            munmap(a->mmapFile, a->mmapFileSize);
        }
        else {
            fclose(a->file);
        }
    }
    /* Pipes requires special close */
    else if (a->rw == EV_READPIPE || a->rw == EV_WRITEPIPE) {
        pclose(a->file);
    }
}


/**
 * This function memory maps the given file as read only.
 * 
 * @param a         handle structure
 * @param fileName  name of file to map
 *
 * @return S_SUCCESS   if successful
 * @return S_FAILURE   if failure to open or memory map file (errno is set)
 */
static int memoryMapFile(EVFILE *a, const char *fileName)
{
#ifndef VXWORKS
    int        fd;
    uint32_t   *pmem;
    size_t      fileSize;
    mode_t      mode;
    struct stat fileInfo;

    
    /* user & user's group have read & write permission */
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    if ((fd = open(fileName, O_RDWR, mode)) < 0) {
        /* errno is set */
        return(S_FAILURE);
    }
    
    /* Find out how big the file is in bytes */
    fstat(fd, &fileInfo);
    fileSize = (size_t)fileInfo.st_size;

    /* Map file to local memory in read-only mode. */
    if ((pmem = (uint32_t *)mmap((void *)0, fileSize, PROT_READ,
                                   MAP_PRIVATE, fd, (off_t)0)) == MAP_FAILED) {
        /* errno is set */
        close(fd);
        return(S_FAILURE);
    }

    /* close fd for mapped mem since no longer needed */
    close(fd);
  
    a->mmapFile = pmem;
    a->mmapFileSize = fileSize;
#endif
    return(S_SUCCESS);
}


/**
 * This function returns a count of the number of events in a file or buffer.
 * If reading with random access, it returns the count taken when initially
 * generating the table of event pointers. If regular reading, the count is
 * generated when asked for in evIoctl. If writing, the count gets incremented
 * by 1 for each evWrite. If appending, the count is set when moving to the
 * correct file position during evOpen and is thereafter incremented with each
 * evWrite.
 *
 * @param a     handle structure
 * @param count pointer to int which gets filled with number of events
 *
 * @return S_SUCCESS          if successful
 * @return S_FAILURE          if using sockets 
 * @return S_EVFILE_BADFILE   if file too small or problem reading file
 * @return S_EVFILE_UNXPTDEOF if buffer too small
 * @return errno              if error in fseek, ftell
 */
static int getEventCount(EVFILE *a, uint32_t *count)
{
    int        i, usingBuffer = 0, nBytes;
    ssize_t    startingPosition;
    uint32_t   bytesUsed, blockEventCount, blockSize, blockHeaderSize, header[EV_HDSIZ];

    
    /* Already protected with read lock since it's called only by evIoctl */

    /* Reject if using sockets/pipes */
    if (a->rw == EV_WRITESOCK || a->rw == EV_READSOCK ||
        a->rw == EV_WRITEPIPE || a->rw == EV_READPIPE) {
        return(S_FAILURE);
    }

    /* If using random access, counting is already done. */
    if (a->randomAccess) {
        *count = a->eventCount;
        return(S_SUCCESS);
    }

    /* If we have a non-zero event count that means
     * it has already been found and is up-to-date. */
    if (a->eventCount > 0) {
        *count = a->eventCount;
        return(S_SUCCESS);
    }

    /* If we have a zero event count and we're writing (NOT in append mode), 
     * it means nothing has been written yet so nothing to read. */
    if (!a->append && (a->rw == EV_WRITEBUF || a->rw == EV_WRITEFILE)) {
        *count = a->eventCount;
        return(S_SUCCESS);
    }

    /* A zero event count may, in fact, be up-to-date. If it is, recounting is
     * not a big deal since there are no events. If it isn't, we need to count
     * the events. So go ahead and count the events now. */

    /* Buffer or file? */
    if (a->rw == EV_READBUF) {
        usingBuffer = 1;
    }

    /* Go to starting position */
    if (usingBuffer) {
        bytesUsed = 0;
    }
    else {
        /* Record starting position, return here when finished */
        if ((startingPosition = ftell(a->file)) < 0) return(errno);

        /* Go to back to beginning of file */
        if (fseek(a->file, 0L, SEEK_SET) < 0) return(errno);
    }

    while (1) {
        /* Read in EV_HDSIZ (8) ints of header */
        if (usingBuffer) {
            /* Is there enough data to read in header? */
            if (a->rwBufSize - bytesUsed < sizeof(header)) {
                /* unexpected EOF or end-of-buffer in this case */
                return(S_EVFILE_UNXPTDEOF);
            }
            /* Read (copy) in header */
            memcpy(header, (a->rwBuf + bytesUsed), 4*EV_HDSIZ);
        }
        else {
            nBytes = sizeof(header)*fread(header, sizeof(header), 1, a->file);
                
            /* Check to see if we read the whole header */
            if (nBytes != sizeof(header)) {
                return(S_EVFILE_BADFILE);
            }
        }

        /* Swap header if necessary */
        if (a->byte_swapped) {
            swap_int32_t(header, EV_HDSIZ, NULL);
        }

        /* Look at block header to get info */
        i               = header[EV_HD_VER];
        blockSize       = header[EV_HD_BLKSIZ];
        blockHeaderSize = header[EV_HD_HDSIZ];
        blockEventCount = header[EV_HD_COUNT];
/*if (debug) printf("getEventCount: ver = 0x%x, blk size = %u, blk hdr sz = %u, blk ev cnt = %u\n",
               i, blockSize, blockHeaderSize, blockEventCount); */
        
        /* Add to the number of events. Dictionary is NOT
         * included in the header's event count. */
        a->eventCount += blockEventCount;

        /* Stop at the last block */
        if (isLastBlockInt(i)) {
            break;
        }

        /* Hop to next block header */
        if (usingBuffer) {
            /* Is there enough buffer space to hop over block? */
            if (a->rwBufSize - bytesUsed < 4*blockSize) {
                /* unexpected EOF or end-of-buffer in this case */
                return(S_EVFILE_UNXPTDEOF);
            }
            bytesUsed += 4*blockSize;
        }
        else {
            if (fseek(a->file, 4*(blockSize-EV_HDSIZ), SEEK_CUR) < 0) return(errno);
        }
    }

    /* Reset file to original position (buffer needs no resetting) */
    if (!usingBuffer) {
        if (fseek(a->file, startingPosition, SEEK_SET) < 0) return(errno);
    }
    if (count != NULL) *count = a->eventCount;

    return(S_SUCCESS);
}


/**
 * This function steps through a memory mapped file or buffer and creates
 * a table containing pointers to the beginning of all events.
 *
 * @param a  handle structure
 *
 * @return S_SUCCESS          if successful or not random access handle (does nothing)
 * @return S_EVFILE_ALLOCFAIL if memory cannot be allocated
 * @return S_EVFILE_UNXPTDEOF if unexpected EOF or end-of-valid-data
 *                            while reading data (perhaps bad block header)
 */
static int generatePointerTable(EVFILE *a)
{
    int        i, usingBuffer=0, lastBlock=0, firstBlock=1;
    size_t     bytesLeft;
    uint32_t  *pmem, len, numPointers, blockEventCount, blockHdrSize, evIndex = 0L;

    
    /* Only random access handles need apply */
    if (!a->randomAccess) {
        return(S_SUCCESS);
    }

    if (a->rw == EV_READBUF) {
        usingBuffer = 1;
    }

    /* Start with space for 10,000 event pointers */
    numPointers = 10000;
    a->pTable = (uint32_t **) malloc(numPointers * sizeof(uint32_t *));

    if (usingBuffer) {
        pmem = (uint32_t *)a->rwBuf;
        bytesLeft = a->rwBufSize; /* limit on size only */
    }
    else {
        pmem = a->mmapFile;
        bytesLeft = a->mmapFileSize;
    }

    while (!lastBlock) {
        
        /* Look at block header to get info */
        i               = pmem[EV_HD_VER];
        blockHdrSize    = pmem[EV_HD_HDSIZ];
        blockEventCount = pmem[EV_HD_COUNT];
        
        /* Swap if necessary */
        if (a->byte_swapped) {
            i               = EVIO_SWAP32(i);
            blockHdrSize    = EVIO_SWAP32(blockHdrSize);
            blockEventCount = EVIO_SWAP32(blockEventCount);
        }
        lastBlock = isLastBlockInt(i);

        /* Hop over block header to data */
        pmem += blockHdrSize;
        bytesLeft -= 4*blockHdrSize;

        /* Check for a dictionary - the first event in the first block.
         * It's not included in the header block count, but we must take
         * it into account by skipping over it. */
        if (hasDictionaryInt(i) && firstBlock) {
            firstBlock = 0;
            
            /* Get its length */
            len = *pmem;
            if (a->byte_swapped) {
                len = EVIO_SWAP32(len);
            }
            /* bank's len does not include itself */
            len++;

            /* Skip over it */
            pmem += len;
            bytesLeft -= 4*len;
        }

        /* For each event in block, store its location */
        for (i=0; i < blockEventCount; i++) {
            /* Sanity check - must have at least 2 ints left */
            if (bytesLeft < 8) {
                free(a->pTable);
                return(S_EVFILE_UNXPTDEOF);
            }

            len = *pmem;
            if (a->byte_swapped) {
                len = EVIO_SWAP32(len);
            }
            /* bank's len does not include itself */
            len++;
            a->pTable[evIndex++] = pmem;

            /* Need more space for the table, increase by 10,000 pointers each time */
            if (evIndex >= numPointers) {
                numPointers += 10000;
                a->pTable = realloc(a->pTable, numPointers*sizeof(uint32_t *));
                if (a->pTable == NULL) {
                    free(a->pTable);
                    return(S_EVFILE_ALLOCFAIL);
                }
            }
        
            pmem += len;
            bytesLeft -= 4*len;
        }
    }
    
    a->eventCount = evIndex;

    return(S_SUCCESS);
}

/**
 * This function positions a file or buffer for the first {@link evWrite}
 * in append mode. It makes sure that the last block header is an empty one
 * with its "last block" bit set.
 *
 * @param a  handle structure
 *
 * @return S_SUCCESS          if successful or not random access handle (does nothing)
 * @return S_EVFILE_TRUNC     if not enough space in buffer to write ending header
 * @return S_EVFILE_BADFILE   if bad file format - cannot read
 * @return S_EVFILE_UNXPTDEOF if unexpected EOF or end-of-valid-data
 *                            while reading data (perhaps bad block header)
 * @return errno              if any file seeking/writing errors
 */
static int toAppendPosition(EVFILE *a)
{
    int         err, debug=0, usingBuffer=0;
    uint32_t    nBytes, bytesToWrite;
    uint32_t   *pmem, sixthWord, header[EV_HDSIZ], *pHeader;
    uint32_t    blockBitInfo, blockEventCount, blockSize, blockHeaderSize, blockNumber=1;
    
    
    /* Only for append mode */
    if (!a->append) {
        return(S_SUCCESS);
    }

    if (a->rw == EV_WRITEBUF) {
        usingBuffer = 1;
    }

    if (usingBuffer) {
        /* Go to back to beginning of buffer */
        a->rwBytesOut  = 0;  /* # bytes written to rwBuf so far (after each evWrite) */
    }
    else {
        /* Go to back to beginning of file */
        if (fseek(a->file, 0L, SEEK_SET) < 0) return(errno);
    }

    while (1) {
        /* Read in EV_HDSIZ (8) ints of header */
        if (usingBuffer) {
            /* Is there enough data to read in header? */
            if (a->rwBufSize - a->rwBytesOut < sizeof(header)) {
                /* unexpected EOF or end-of-buffer in this case */
                return(S_EVFILE_UNXPTDEOF);
            }

            /* Look for block header info here */
            a->buf = pHeader = (uint32_t *) (a->rwBuf + a->rwBytesOut);

            blockBitInfo    = pHeader[EV_HD_VER];
            blockSize       = pHeader[EV_HD_BLKSIZ];
            blockHeaderSize = pHeader[EV_HD_HDSIZ];
            blockEventCount = pHeader[EV_HD_COUNT];
            
            /* Swap header if necessary */
            if (a->byte_swapped) {
                blockBitInfo    = EVIO_SWAP32(blockBitInfo);
                blockSize       = EVIO_SWAP32(blockSize);
                blockHeaderSize = EVIO_SWAP32(blockHeaderSize);
                blockEventCount = EVIO_SWAP32(blockEventCount);
            }
        }
        else {
            nBytes = sizeof(header)*fread(header, sizeof(header), 1, a->file);
                
            /* Check to see if we read the whole header */
            if (nBytes != sizeof(header)) {
                return(S_EVFILE_BADFILE);
            }
            
            /* Swap header if necessary */
            if (a->byte_swapped) {
                swap_int32_t(header, EV_HDSIZ, NULL);
            }

            /* Look at block header to get info */
            blockBitInfo    = header[EV_HD_VER];
            blockSize       = header[EV_HD_BLKSIZ];
            blockHeaderSize = header[EV_HD_HDSIZ];
            blockEventCount = header[EV_HD_COUNT];
        }
        
        /* Add to the number of events */
        a->eventCount += blockEventCount;

        /* Next block has this number. */
        blockNumber++;

        /* Stop at the last block */
        if (isLastBlockInt(blockBitInfo)) {
            break;
        }

        /* Hop to next block header */
        if (usingBuffer) {
            /* Is there enough buffer space to hop over block? */
            if (a->rwBufSize - a->rwBytesOut < 4*blockSize) {
                /* unexpected EOF or end-of-buffer in this case */
                return(S_EVFILE_UNXPTDEOF);
            }
            a->rwBytesOut += 4*blockSize;
        }
        else {
            if (fseek(a->file, 4*(blockSize-EV_HDSIZ), SEEK_CUR) < 0) return(errno);
        }
    }

    /* If we're here, we've just read the last block header (at least EV_HDSIZ words of it).
     * Easiest to rewrite it in our own image at this point. But first we
     * must check to see if the last block contains data. If it does, we
     * change a bit so it's not labeled as the last block. Then we write
     * an empty last block after the data. If, on the other hand, this
     * last block contains no data, just write over it with a new one. */
    if (blockSize > blockHeaderSize) {
        /* Clear last block bit in 6th header word */
        sixthWord = clearLastBlockBitInt(blockBitInfo);
        
        /* Rewrite header word with bit info & hop over block */
        if (usingBuffer) {
/*if (debug) printf("toAppendPosition: writing over last block's 6th word for buffer\n");*/
            /* Write over 6th block header word */
            pmem = (uint32_t *) (a->rwBuf + a->rwBytesOut + 4*EV_HD_VER);
            *pmem = sixthWord;

            /* Hop over the entire block */
            a->rwBytesOut += 4*blockSize;

            /* If there's not enough space in the user-given buffer to
             * contain another (empty ending) block header, return an error. */
            if (a->rwBufSize < a->rwBytesOut + 4*EV_HDSIZ) {
                return(S_EVFILE_TRUNC);
            }

            /* Initialize bytes in a->rwBuf for a new block header */
            a->buf = (uint32_t *) (a->rwBuf + a->rwBytesOut);
            initBlockHeader(a->buf);
        }
        else {
            /* Back up to before 6th block header word */
if (debug) printf("toAppendPosition: writing over last block's 6th word, back up %d words\n",
                   (EV_HDSIZ - EV_HD_VER));
            if (fseek(a->file, -4*(EV_HDSIZ - EV_HD_VER), SEEK_CUR) < 0) return(errno);

            /* Write over 6th block header word */
            if (fwrite((const void *)&sixthWord, 1, sizeof(uint32_t), a->file) != sizeof(uint32_t)) {
                return(errno);
            }

            /* Hop over the entire block */
if (debug) printf("toAppendPosition: wrote over last block's 6th word, hop over %d words\n",
                  (blockSize - (EV_HD_VER + 1)));
            if (fseek(a->file, 4*(blockSize - (EV_HD_VER + 1)), SEEK_CUR) < 0) return(errno);
        }
    }
    else {
        /* We already read in the block header, now back up so we can overwrite it.
         * If using buffer, we never incremented the position, so we're OK position wise. */
        blockNumber--;
        if (usingBuffer) {
            initBlockHeader(a->buf);
        }
        else {
            long ppos;
            if (fseek(a->file, -4*EV_HDSIZ, SEEK_CUR) < 0) return(errno);
            ppos = ftell(a->file);
if (debug) printf("toAppendPosition: last block had no data, back up 1 header to pos = %ld (%ld words)\n", ppos, ppos/4);
        }
    }

    /* Write empty last block header. This function gets called right after
     * the handle's block header memory is initialized and other members of
     * the handle structure are also initialized. Some of the values need to
     * be set properly here - like the block number - since we've skipped over
     * all existing blocks. */
    a->buf[EV_HD_BLKNUM] = blockNumber;
    a->blknum = blockNumber + 1;
    
    /* Mark this block as the last one to be written. */
    setLastBlockBit(a->buf);
    a->isLastBlock = 1;

    a->buf[EV_HD_BLKSIZ] = EV_HDSIZ;
    bytesToWrite = 4*EV_HDSIZ;

    if (usingBuffer) {
        nBytes = bytesToWrite;
        /* How many bytes ended up in the buffer we're writing to? */
        a->rwBytesOut += bytesToWrite;
    }
    else {
        long ppos;
        
        /* Clear EOF and error indicators for file stream */
        clearerr(a->file);
        /* This last block is always EV_HDSIZ ints long. */
        nBytes = fwrite((const void *)a->buf, 1, bytesToWrite, a->file);
        /* Return any error condition of file stream */
        if (ferror(a->file)) return(ferror(a->file));

        /* Now that the ending header is in the file, back up and
         * prepare to write over it with whatever is coming next. */
        if (fseek(a->file, -4*EV_HDSIZ, SEEK_CUR) < 0) return(errno);
        ppos = ftell(a->file);
if (debug) printf("toAppendPosition: prepare to write, back up 1 header to pos = %ld (%ld words)\n", ppos, ppos/4);

    }

    /* Return any error condition of write attempt */
    if (nBytes != bytesToWrite) return(errno);
       
    /* We should now be in a state identical to that if we had
     * just now written everything currently in the file/buffer. */

    return(S_SUCCESS);
}


/**
 * This routine reads an evio bank from an evio format file/socket/buffer
 * opened with routines {@link evOpen}, {@link evOpenBuffer}, or
 * {@link evOpenSocket}, allocates a buffer and fills it with the bank.
 * Works with all versions of evio. A status is returned. Caller will need
 * to free buffer to avoid a memory leak.
 *
 * @param a      pointer to handle structure
 * @param buffer pointer to pointer to buffer gets filled with
 *               pointer to allocated buffer (caller must free)
 * @param buflen pointer to int gets filled with length of buffer in 32 bit words
 *               including the full (8 byte) header
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_BADMODE   if opened for writing or random-access reading
 * @return S_EVFILE_BADARG    if buffer or buflen is NULL
 * @return S_EVFILE_ALLOCFAIL if memory cannot be allocated
 * @return S_EVFILE_UNXPTDEOF if unexpected EOF or end-of-valid-data
 *                            while reading data (perhaps bad     block header)
 * @return EOF                if end-of-file or end-of-valid-data reached
 * @return errno              if file/socket read error
 * @return stream error       if file stream error
 */
static int evReadAllocImpl(EVFILE *a, uint32_t **buffer, uint32_t *buflen)
{
    uint32_t *buf, *pBuf;
    int       error, status;
    uint32_t  nleft, ncopy, len;


    /* Check args */
    if (buffer == NULL || buflen == NULL) {
        return(S_EVFILE_BADARG);
    }

    /* Need to be reading not writing */
    if (a->rw != EV_READFILE && a->rw != EV_READPIPE &&
        a->rw != EV_READBUF  && a->rw != EV_READSOCK) {
        return(S_EVFILE_BADMODE);
    }

    /* Cannot be random access reading */
    if (a->randomAccess) {
        return(S_EVFILE_BADMODE);
    }

    /* Lock mutex for multithreaded reads/writes/access */
    mutexLock(a);
    
    /* If no more data left to read from current block, get a new block */
    if (a->left < 1) {
        status = evGetNewBuffer(a);
        if (status != S_SUCCESS) {
            mutexUnlock(a);
            return(status);
        }
    }

    /* Find number of words to read in next event (including header) */
    if (a->byte_swapped) {
        /* Value at pointer to next event (bank) header = length of bank - 1 */
        nleft = EVIO_SWAP32(*(a->next)) + 1;
    }
    else {
        /* Length of next bank, including header, in 32 bit words */
        nleft = *(a->next) + 1;
    }

    /* Allocate buffer for event */
    len = nleft;
    buf = pBuf = (uint32_t *)malloc(4*len);
    if (buf == NULL) {
        mutexUnlock(a);
        return(S_EVFILE_ALLOCFAIL);
    }

    /* While there is more event data left to read ... */
    while (nleft > 0) {

        /* If no more data left to read from current block, get a new block */
        if (a->left < 1) {
            status = evGetNewBuffer(a);
            if (status != S_SUCCESS) {
                if (buf != NULL) free(buf);
                mutexUnlock(a);
                return(status);
            }
        }

        /* If # words left to read in event <= # words left in block,
         * copy # words left to read in event to buffer, else
         * copy # left in block to buffer.*/
        ncopy = (nleft <= a->left) ? nleft : a->left;
        
        memcpy(pBuf, a->next, ncopy*4);
        
        pBuf    += ncopy;
        nleft   -= ncopy;
        a->next += ncopy;
        a->left -= ncopy;
    }

    /* Unlock mutex for multithreaded reads/writes/access */
    mutexUnlock(a);

    /* Swap event in place if necessary */
    if (a->byte_swapped) {
        evioswap(buf, 1, NULL);
    }

    /* Return allocated buffer with event inside and its inclusive len (with full header) */
    *buffer = buf;
    *buflen = len;
    
    return(S_SUCCESS);
}


/**
 * This routine reads an evio bank from an evio format file/socket/buffer
 * opened with routines {@link evOpen}, {@link evOpenBuffer}, or
 * {@link evOpenSocket}, allocates a buffer and fills it with the bank.
 * Works with all versions of evio. A status is returned.
 *
 * @param handle evio handle
 * @param buffer pointer to pointer to buffer gets filled with
 *               pointer to allocated buffer (caller must free)
 * @param buflen pointer to int gets filled with length of buffer in 32 bit words
 *               including the full (8 byte) bank header
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_BADMODE   if opened for writing or random-access reading
 * @return S_EVFILE_BADARG    if buffer or buflen is NULL
 * @return S_EVFILE_BADHANDLE if bad handle arg
 * @return S_EVFILE_ALLOCFAIL if memory cannot be allocated
 * @return S_EVFILE_UNXPTDEOF if unexpected EOF or end-of-valid-data
 *                            while reading data (perhaps bad block header)
 * @return EOF                if end-of-file or end-of-valid-data reached
 * @return errno              if file/socket read error
 * @return stream error       if file stream error
 */
int evReadAlloc(int handle, uint32_t **buffer, uint32_t *buflen)
{
    EVFILE *a;
    int status;

    
    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }
    
    /* Don't allow simultaneous calls to evClose(), but do allow reads & writes. */
    handleReadLock(handle);

    /* Look up file struct (which contains block buffer) from handle */
    a = handleList[handle-1];

    if (a == NULL) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }

    status = evReadAllocImpl(a, buffer, buflen);
    
    handleReadUnlock(handle);

    return status;
}


/** Fortran interface to {@link evRead}. */
#ifdef AbsoftUNIXFortran
int evread
#else
int evread_
#endif
(int *handle, uint32_t *buffer, uint32_t *buflen)
{
    return(evRead(*handle, buffer, *buflen));
}


/**
 * This routine reads from an evio format file/socket/buffer opened with routines
 * {@link evOpen}, {@link evOpenBuffer}, or {@link evOpenSocket} and returns the
 * next event in the buffer arg. Works with all versions of evio. A status is
 * returned.
 *
 * @param handle evio handle
 * @param buffer pointer to buffer
 * @param buflen length of buffer in 32 bit words
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_BADMODE   if opened for writing or random-access reading
 * @return S_EVFILE_TRUNC     if buffer provided by caller is too small for event read
 * @return S_EVFILE_BADARG    if buffer is NULL or buflen < 3
 * @return S_EVFILE_BADHANDLE if bad handle arg
 * @return S_EVFILE_ALLOCFAIL if memory cannot be allocated
 * @return S_EVFILE_UNXPTDEOF if unexpected EOF or end-of-valid-data
 *                            while reading data (perhaps bad block header)
 * @return EOF                if end-of-file or end-of-valid-data reached
 * @return errno              if file/socket read error
 * @return stream error       if file stream error
 */
int evRead(int handle, uint32_t *buffer, uint32_t buflen)
{
    EVFILE   *a;
    int       error, status,  swap;
    uint32_t  nleft, ncopy;
    uint32_t *temp_buffer, *temp_ptr=NULL;


    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }
    
    if (buffer == NULL || buflen < 3) {
        return(S_EVFILE_BADARG);
    }

    /* Don't allow simultaneous calls to evClose(), but do allow reads & writes. */
    handleReadLock(handle);

    /* Look up file struct (which contains block buffer) from handle */
    a = handleList[handle-1];

    /* Check args */
    if (a == NULL) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }

    /* Need to be reading not writing */
    if (a->rw != EV_READFILE && a->rw != EV_READPIPE &&
        a->rw != EV_READBUF  && a->rw != EV_READSOCK) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }
    
    /* Cannot be random access reading */
    if (a->randomAccess) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }
    
    /* Lock mutex for multithreaded reads/writes/access */
    mutexLock(a);
    /* If no more data left to read from current block, get a new block */
    if (a->left < 1) {
        status = evGetNewBuffer(a);
        if (status != S_SUCCESS) {
            mutexUnlock(a);
            handleReadUnlock(handle);
            return(status);
        }
    }

    /* Find number of words to read in next event (including header) */
    if (a->byte_swapped) {
        /* Create temp buffer for swapping */
        temp_ptr = temp_buffer = (uint32_t *) malloc(buflen*sizeof(uint32_t));
        if (temp_ptr == NULL) return(S_EVFILE_ALLOCFAIL);
        /* Value at pointer to next event (bank) header = length of bank - 1 */
        nleft = EVIO_SWAP32(*(a->next)) + 1;
    }
    else {
        /* Length of next bank, including header, in 32 bit words */
        nleft = *(a->next) + 1;
    }

    /* Is there NOT enough room in buffer to store whole event? */
    if (nleft > buflen) {
        /* Buffer too small, just return error.
         * Previous evio lib tried to swap truncated event!? */
        if (temp_ptr != NULL) free(temp_ptr);
        mutexUnlock(a);
        handleReadUnlock(handle);
        return(S_EVFILE_TRUNC);
    }

    /* While there is more event data left to read ... */
    while (nleft > 0) {

        /* If no more data left to read from current block, get a new block */
        if (a->left < 1) {
            status = evGetNewBuffer(a);
            if (status != S_SUCCESS) {
                if (temp_ptr != NULL) free(temp_ptr);
                mutexUnlock(a);
                handleReadUnlock(handle);
                return(status);
            }
        }

        /* If # words left to read in event <= # words left in block,
         * copy # words left to read in event to buffer, else
         * copy # left in block to buffer.*/
        ncopy = (nleft <= a->left) ? nleft : a->left;

        if (a->byte_swapped) {
            memcpy(temp_buffer, a->next, ncopy*4);
            temp_buffer += ncopy;
        }
        else{
            memcpy(buffer, a->next, ncopy*4);
            buffer += ncopy;
        }
        
        nleft   -= ncopy;
        a->next += ncopy;
        a->left -= ncopy;
    }

    /* Store value locally so we can release lock before swapping. */
    swap = a->byte_swapped;
    
    /* Unlock mutex for multithreaded reads/writes/access */
    mutexUnlock(a);
    handleReadUnlock(handle);

    /* Swap event if necessary */
    if (swap) {
        evioswap((uint32_t*)temp_ptr, 1, (uint32_t*)buffer);
        free(temp_ptr);
    }
    
    return(S_SUCCESS);
}


/**
 * This routine reads from an evio format file/buffer/socket opened with routines
 * {@link evOpen}, {@link evOpenBuffer}, or {@link evOpenSocket} and returns a
 * pointer to the next event residing in an internal buffer.
 * If the data needs to be swapped, it is swapped in place. Any other
 * calls to read routines may cause the data to be overwritten.
 * No writing to the returned pointer is allowed.
 * Works only with evio version 4 and up. A status is returned.
 *
 * @param handle evio handle
 * @param buffer pointer to pointer to buffer gets filled with pointer to location in
 *               internal buffer which is guaranteed to be valid only until the next
 *               {@link evRead}, {@link evReadNoAlloc}, or {@link evReadNoCopy} call.
 * @param buflen pointer to int gets filled with length of buffer in 32 bit words
 *               including the full (8 byte) bank header
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_BADMODE   if opened for writing or random-access reading
 * @return S_EVFILE_BADARG    if buffer or buflen is NULL
 * @return S_EVFILE_BADFILE   if version < 4, unsupported or bad format
 * @return S_EVFILE_BADHANDLE if bad handle arg
 * @return S_EVFILE_ALLOCFAIL if memory cannot be allocated
 * @return S_EVFILE_UNXPTDEOF if unexpected EOF or end-of-valid-data
 *                               while reading data (perhaps bad block header)
 * @return EOF                if end-of-file or end-of-valid-data reached
 * @return errno              if file/socket read error
 * @return stream error       if file stream error
 */
int evReadNoCopy(int handle, const uint32_t **buffer, uint32_t *buflen)
{
    EVFILE    *a;
    int       status;
    uint32_t  nleft;


    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }
    
    if (buffer == NULL || buflen == NULL) {
        return(S_EVFILE_BADARG);
    }

    /* Don't allow simultaneous calls to evClose(), but do allow reads & writes. */
    handleReadLock(handle);

    /* Look up file struct (which contains block buffer) from handle */
    a = handleList[handle-1];

    /* Check args */
    if (a == NULL) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }

    /* Returning a pointer into a block only works in evio version 4 and
     * up since in earlier versions events may be split between blocks. */
    if (a->version < 4) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADFILE);
    }

    /* Need to be reading and not writing */
    if (a->rw != EV_READFILE && a->rw != EV_READPIPE &&
        a->rw != EV_READBUF  && a->rw != EV_READSOCK) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }
    
    /* Cannot be random access reading */
    if (a->randomAccess) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }
    
    /* Lock mutex for multithreaded reads/writes/access */
    mutexLock(a);

    /* If no more data left to read from current block, get a new block */
    if (a->left < 1) {
        status = evGetNewBuffer(a);
        if (status != S_SUCCESS) {
            mutexUnlock(a);
            handleReadUnlock(handle);
            return(status);
        }
    }

    /* Find number of words to read in next event (including header) */
    if (a->byte_swapped) {
        /* Length of next bank, including header, in 32 bit words */
        nleft = EVIO_SWAP32(*(a->next)) + 1;
                        
        /* swap data in block buffer */
        evioswap(a->next, 1, NULL);
    }
    else {
        /* Length of next bank, including header, in 32 bit words */
        nleft = *(a->next) + 1;
    }

    /* return location of place of event in block buffer */
    *buffer = a->next;
    *buflen = nleft;

    a->next += nleft;
    a->left -= nleft;
    
    mutexUnlock(a);
    handleReadUnlock(handle);

    return(S_SUCCESS);
}


/**
 * This routine does a random access read from an evio format file/buffer opened
 * with routines {@link evOpen} or {@link evOpenBuffer}. It returns a
 * pointer to the desired event residing in either a
 * memory mapped file or buffer when opened in random access mode.
 * 
 * If the data needs to be swapped, it is swapped in place.
 * No writing to the returned pointer is allowed.
 * Works only with evio version 4 and up. A status is returned.
 *
 * @param handle evio handle
 * @param buffer pointer which gets filled with pointer to event in buffer or
 *               memory mapped file
 * @param buflen pointer to int gets filled with length of buffer in 32 bit words
 *               including the full (8 byte) bank header
 * @param eventNumber the number of the event to be read (returned) starting at 1.
 *
 * @return S_SUCCESS          if successful
 * @return S_FAILURE          if no events found in file or failure to make random access map
 * @return S_EVFILE_BADMODE   if not opened for random access reading
 * @return S_EVFILE_BADARG    if pEvent arg is NULL
 * @return S_EVFILE_BADFILE   if version < 4, unsupported or bad format
 * @return S_EVFILE_BADHANDLE if bad handle arg
 */
int evReadRandom(int handle, const uint32_t **pEvent, uint32_t *buflen, uint32_t eventNumber)
{
    EVFILE   *a;
    uint32_t *pev;

    
    if (pEvent == NULL) {
        return(S_EVFILE_BADARG);
    }

    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }
    
    /* Don't allow simultaneous calls to evClose(), but do allow reads & writes. */
    handleReadLock(handle);

    /* Look up file struct (which contains block buffer) from handle */
    a = handleList[handle-1];

    /* Check args */
    if (a == NULL) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }

    /* Returning a pointer into a block only works in evio version 4 and
    * up since in earlier versions events may be split between blocks. */
    if (a->version < 4) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADFILE);
    }

    /* Need to be *** random access *** reading (not from socket or pipe) and not writing */
    if ((a->rw != EV_READFILE && a->rw != EV_READBUF) || !a->randomAccess) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }

    /* event not in file/buf */
    if (eventNumber > a->eventCount || a->pTable == NULL) {
        handleReadUnlock(handle);
        return(S_FAILURE);
    }

    pev = a->pTable[eventNumber - 1];

    /* event not in file/buf */
    if (pev == NULL) {
        handleReadUnlock(handle);
        return(S_FAILURE);
    }

    /* Lock mutex for multithreaded swap */
    mutexLock(a);

    /* Find number of words to read in next event (including header) */
    /* and swap data in buf/mem-map if necessary */
    if (a->byte_swapped) {
        /* Length of bank, including header, in 32 bit words */
        *buflen = EVIO_SWAP32(*pev) + 1;
                        
        /* swap data in buf/mem-map buffer */
        evioswap(pev, 1, NULL);
    }
    else {
        /* Length of bank, including header, in 32 bit words */
        *buflen = *pev + 1;
    }
    
    mutexUnlock(a);
   
    /* return pointer to event in memory map / buffer */
    *pEvent = pev;
    
    handleReadUnlock(handle);

    return(S_SUCCESS);
}


/**
 * Routine to get the next block.
 *
 * @param a pointer to file structure
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_ALLOCFAIL if memory cannot be allocated
 * @return S_EVFILE_UNXPTDEOF if unexpected EOF or end-of-valid-data
 *                            while reading data (perhaps bad block header
 *                            or reading from a too-small buffer)
 * @return EOF                if end-of-file or end-of-valid-data reached
 * @return errno              if file/socket read error
 * @return stream error       if file stream error
 */
static int evGetNewBuffer(EVFILE *a)
{
    uint32_t *newBuf, blkHdrSize;
    size_t    nBytes, bytesToRead;
    int       debug=0, status = S_SUCCESS;

    
    /* See if we read in the last block the last time this was called (v4) */
    if (a->version > 3 && a->isLastBlock) {
        return(EOF);
    }

    /* First read block header from file/sock/buf */
    bytesToRead = 4*EV_HDSIZ;
    if (a->rw == EV_READFILE) {
        /* If end-of-file, return EOF as status */
        if (feof(a->file)) {
            return(EOF);
        }

        /* Clear EOF and error indicators for file stream */
        clearerr(a->file);

        /* Read block header */
        nBytes = fread(a->buf, 1, bytesToRead, a->file);
        
        /* Return end-of-file if so */
        if (feof(a->file)) {
            return(EOF);
        }
    
        /* Return any error condition of file stream */
        if (ferror(a->file)) {
            return(ferror(a->file));
        }
    }
    else if (a->rw == EV_READSOCK) {
        nBytes = tcpRead(a->sockFd, a->buf, bytesToRead);
    }
    else if (a->rw == EV_READPIPE) {
        nBytes = fread(a->buf, 1, bytesToRead, a->file);
    }
    else if (a->rw == EV_READBUF) {
        if (a->rwBufSize < a->rwBytesIn + bytesToRead) {
            return(S_EVFILE_UNXPTDEOF);
        }
        memcpy(a->buf, (a->rwBuf + a->rwBytesIn), bytesToRead);
        nBytes = bytesToRead;
        a->rwBytesIn += bytesToRead;
    }

    /* Return any read error */
    if (nBytes != bytesToRead) {
        return(errno);
    }

    /* Swap header in place if necessary */
    if (a->byte_swapped) {
        swap_int32_t((uint32_t *)a->buf, EV_HDSIZ, NULL);
    }
    
    /* It is possible that the block header size is > EV_HDSIZ.
     * I think that the only way this could happen is if someone wrote
     * out an evio file "by hand", that is, not writing using the evio libs
     * but writing the bits directly to file. Be sure to check for it
     * and read any extra words in the header. They may need to be swapped. */
    blkHdrSize = a->buf[EV_HD_HDSIZ];
    if (blkHdrSize > EV_HDSIZ) {
        /* Read rest of block header from file/sock/buf ... */
        bytesToRead = 4*(blkHdrSize - EV_HDSIZ);
if (debug) printf("HEADER IS TOO BIG, reading an extra %lu bytes\n", bytesToRead);
        if (a->rw == EV_READFILE) {
            nBytes = fread(a->buf + EV_HDSIZ, 1, bytesToRead, a->file);
        
            if (feof(a->file)) return(EOF);
            if (ferror(a->file)) return(ferror(a->file));
        }
        else if (a->rw == EV_READSOCK) {
            nBytes = tcpRead(a->sockFd, a->buf + EV_HDSIZ, bytesToRead);
        }
        else if (a->rw == EV_READPIPE) {
            nBytes = fread(a->buf + EV_HDSIZ, 1, bytesToRead, a->file);
        }
        else if (a->rw == EV_READBUF) {
            if (a->rwBufSize < a->rwBytesIn + bytesToRead) return(S_EVFILE_UNXPTDEOF);
            memcpy(a->buf + EV_HDSIZ, a->rwBuf + a->rwBytesIn, bytesToRead);
            nBytes = bytesToRead;
            a->rwBytesIn += bytesToRead;
        }

        /* Return any read error */
        if (nBytes != bytesToRead) return(errno);
                
        /* Swap in place if necessary */
        if (a->byte_swapped) {
            swap_int32_t(a->buf + EV_HDSIZ, bytesToRead/4, NULL);
        } 
    }
    
    /* Each block may be different size so find it. */
    a->blksiz = a->buf[EV_HD_BLKSIZ];

    /* Do we have room to read the rest of the block data?
     * If not, allocate a bigger block buffer. */
    if (a->bufSize < a->blksiz) {
#ifdef DEBUG
        fprintf(stderr,"evGetNewBuffer: increase internal buffer size to %d bytes\n", 4*a->blksiz);
#endif
        newBuf = (uint32_t *)malloc(4*a->blksiz);
        if (newBuf == NULL) {
            return(S_EVFILE_ALLOCFAIL);
        }
        
        /* copy header into new buf */
        memcpy((void *)newBuf, (void *)a->buf, 4*blkHdrSize);
        
        /* bookkeeping */
        a->bufSize = a->blksiz;
        free(a->buf);
        a->buf = newBuf;
    }

    /* Read rest of block */
    bytesToRead = 4*(a->blksiz - blkHdrSize);
    if (a->rw == EV_READFILE) {
        nBytes = fread((a->buf + blkHdrSize), 1, bytesToRead, a->file);
        if (feof(a->file))   {return(EOF);}
        if (ferror(a->file)) {return(ferror(a->file));}
    }
    else if (a->rw == EV_READSOCK) {
        nBytes = tcpRead(a->sockFd, (a->buf + blkHdrSize), bytesToRead);
    }
    else if (a->rw == EV_READSOCK) {
        nBytes = fread((a->buf + blkHdrSize), 1, bytesToRead, a->file);
    }
    else if (a->rw == EV_READBUF) {
        if (a->rwBufSize < a->rwBytesIn + bytesToRead) {
            return(S_EVFILE_UNXPTDEOF);
        }
        memcpy(a->buf + blkHdrSize, a->rwBuf + a->rwBytesIn, bytesToRead);
        nBytes = bytesToRead;
        a->rwBytesIn += bytesToRead;
    }

    /* Return any read error */
    if (nBytes != bytesToRead) {
        return(errno);
    }

    /* Keep track of the # of blocks read */
    a->blknum++;
    
    /* Is our block # consistent with block header's? */
    if (a->buf[EV_HD_BLKNUM] != a->blknum + a->blkNumDiff) {
        /* Record the difference so we don't print out a message
        * every single time if things get out of sync. */
        a->blkNumDiff = a->buf[EV_HD_BLKNUM] - a->blknum;
#ifdef DEBUG
        fprintf(stderr,"evGetNewBuffer: block # read(%d) is different than expected(%d)\n",
                a->buf[EV_HD_BLKNUM], a->blknum);
#endif
    }

    /* Check to see if we just read in the last block (v4) */
    if (a->version > 3 && isLastBlock(a->buf)) {
        a->isLastBlock = 1;
    }

    /* Start out pointing to the data right after the block header.
     * If we're in the middle of reading an event, this will allow
     * us to continue reading it. If we've looking to read a new
     * event, this should point to the next one. */
    a->next = a->buf + blkHdrSize;

    /* Find number of valid words left to read (w/ evRead) in block */
    if (a->version < 4) {
        a->left = (a->buf)[EV_HD_USED] - blkHdrSize;
    }
    else {
        a->left = a->blksiz - blkHdrSize;
    }

    /* If there are no valid data left in block ... */
    if (a->left < 1) {
        /* Hit end of valid data in file/sock/buf in v4 */
        if (a->isLastBlock) {
            return(EOF);
        }
        return(S_EVFILE_UNXPTDEOF);
    }

    return(status);
}


/**
 * Calculates the sixth word of the block header which has the version number
 * in the lowest 8 bits (1-8). The arg hasDictionary is set in the 9th bit and
 * isEnd is set in the 10th bit.
 * Four bits of an int (event type) are set in bits 11-14.
 *
 * @param version evio version number
 * @param hasDictionary does this block include an evio xml dictionary as the first event?
 * @param isEnd is this the last block of a file or a buffer?
 * @param eventType 4 bit type of events header is containing
 * @return generated sixth word of this header.
 */
static int generateSixthWord(int version, int hasDictionary, int isEnd, int eventType)
{
    version  =  hasDictionary ? (version | EV_DICTIONARY_MASK) : version;
    version  =  isEnd         ? (version | EV_LASTBLOCK_MASK)  : version;
    version |= ((eventType & 0xf) << 10);

    return version;
}


/**
 * Write a block header into the given buffer.
 *
 * @param a             pointer to file structure
 * @param wordSize      size in number of 32 bit words
 * @param eventCount    number of events in block
 * @param blockNumber   number of block
 * @param hasDictionary does this block have a dictionary?
 * @param isLast        is this the last block?
 * @param currentHeader should this header be considered the current header?
 * @param absoluteMode  should this header be written so the internal
 *                      buffer's position does <b></b>not</b> change?
 * 
 * @return S_SUCCESS    if successful
 * @return S_FAILURE    if not enough space in buffer for header
 */
static int writeNewHeader(EVFILE *a, uint32_t wordSize,
                          uint32_t eventCount, uint32_t blockNumber,
                          int hasDictionary,   int isLast,
                          int isCurrentHeader, int absoluteMode)
{
    uint32_t *pos, debug=0;
    
    
    /* If no room left for a header to be written in buffer ... */
    if ((a->bufSize - a->bytesToBuf/4) < 8) {
if (debug) printf("writeNewHeader(): no room in buffer, return, buf size = %u, bytes to buf = %u\n",
               a->bufSize, a->bytesToBuf/4);
        return (S_FAILURE);
    }

    /* Record where beginning of header is so we can
     * go back and update block size and event count. */
    if (isCurrentHeader) {
        a->currentHeader = a->next;
    }

/*if (debug) printf("writeNewHeader(): words = %d, block# = %d, ev Cnt = %d, 6th wd = 0x%x\n",
    wordSize, blockNumber, eventCount, generateSixthWord(4, hasDictionary, isLast, 0)); */

    /* Write header words, some of which will be
     * overwritten later when the values are determined. */
    pos    = a->next;
    pos[0] = wordSize;     /* actual size in 32-bit words (Ints) */
    pos[1] = blockNumber;  /* incremental count of blocks */
    pos[2] = 8;            /* header size always 8 */
    pos[3] = eventCount;   /* number of events in block */
    pos[4] = 0;            /* unused / sourceId for coda event building */
                           /* version = 4, no event type info */
    pos[5] = generateSixthWord(4, hasDictionary, isLast, 0);
    pos[6] = 0;            /* unused */
    pos[7] = EV_MAGIC;     /* MAGIC_NUMBER */
        
    if (!absoluteMode) {
        a->next   += 8;
        a->left   -= 8;
        a->blksiz  = 8;
        a->blkEvCount = 0;
    }
    
    a->bytesToBuf += 4*EV_HDSIZ;

    return (S_SUCCESS);
}


/**
 * This routine writes an empty, last block (just the header).
 * This ensures that if the data taking software crashes in the mean time,
 * we still have a file or buffer in the proper format.
 * Make sure that the "last block" bit is set. If we're not closing,
 * the block header simply gets over written in the next write.
 * The position of the buffer or file channel is not changed in preparation
 * for writing over the last block header.
 *
 * @param a             pointer to file structure
 * @param blockNumber   block number to use in header
 * @return S_SUCCESS    if successful
 * @return S_FAILURE    if not enough space in buffer for header
 */
static int writeEmptyLastBlockHeader(EVFILE *a, int blockNumber)
{
    return writeNewHeader(a, 8, 0, blockNumber, 0, 1, 0, 1);
}


/** Fortran interface to {@link evWrite}. */
#ifdef AbsoftUNIXFortran
int evwrite
#else
int evwrite_
#endif
(int *handle, const uint32_t *buffer)
{
    return(evWrite(*handle, buffer));
}


/**
 * This routine writes an evio event to an internal buffer containing evio data.
 * If that internal buffer is full, it is flushed to the final destination
 * file/socket/buffer/pipe opened with routines {@link evOpen}, {@link evOpenBuffer},
 * or {@link evOpenSocket}.
 * It writes data in evio version 4 format and returns a status.
 *
 * @param handle evio handle
 * @param buffer pointer to buffer containing event to write
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_BADMODE   if opened for reading or appending to opposite endian file/buffer.
 * @return S_EVFILE_TRUNC     if not enough room writing to a user-supplied buffer
 * @return S_EVFILE_BADARG    if buffer is NULL
 * @return S_EVFILE_BADHANDLE if bad handle arg
 * @return S_EVFILE_ALLOCFAIL if cannot allocate memory
 * @return errno              if file/socket write error
 * @return stream error       if file stream error
 */
int evWrite(int handle, const uint32_t *buffer)
{
    return evWriteImpl(handle, buffer, 1, 0);
}


/**
 * This routine expands the size of the internal buffer used when
 * writing to files/sockets/pipes. Some variables are updated.
 * Assumes 1 block header of space has been (or shortly will be) used.
 *
 * @param a pointer to data structure
 * @param newSize size in bytes to make the new buffer
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_ALLOCFAIL if cannot allocate memory
 */
static int expandBuffer(EVFILE *a, uint32_t newSize)
{
    int debug = 0;
    uint32_t *biggerBuf = NULL;
    
    /* No need to increase it. */
    if (newSize <= 4*a->bufSize) {
if (debug) printf("    expandBuffer: buffer is big enough\n");
        return(S_SUCCESS);
    }
    /* The memory is already there, just not currently utilized */
    else if (newSize <= 4*a->bufRealSize) {
if (debug) printf("    expandBuffer: expand, but memory already there\n");
        a->bufSize = newSize/4;
        return(S_SUCCESS);
    }
    
    biggerBuf = (uint32_t *) malloc(newSize);
    if (!biggerBuf) {
        return(S_EVFILE_ALLOCFAIL);
    }
    
if (debug) printf("    expandBuffer: increased buffer size to %u bytes\n", newSize);
    
    /* Use the new buffer from here on */
    free(a->buf);
    a->buf = biggerBuf;
    a->currentHeader = biggerBuf;

    /* Update free space size, pointer to writing space, & buffer size */
    a->left = newSize/4 - EV_HDSIZ;
    a->next = a->buf + EV_HDSIZ;
    a->bufRealSize = a->bufSize = newSize/4;
    
    return(S_SUCCESS);
}


/**
 * This routine writes an event into the internal buffer
 * and does much of the bookkeeping associated with it.
 * 
 * @param a             pointer to data structure
 * @param buffer        buffer containing event to be written
 * @param wordsToWrite  number of 32-bit words to write from buffer
 * @param isDictionary  true if is this a dictionary being written
 */
static void writeEventToBuffer(EVFILE *a, const uint32_t *buffer,
                               uint32_t wordsToWrite, int isDictionary)
{
    int debug = 0;
    
    
if (debug) printf("  writeEventToBuffer: before write, bytesToBuf = %u\n",
                         a->bytesToBuf);
    
    /* Write event to internal buffer */
    memcpy((void *)a->next, (const void *)buffer, 4*wordsToWrite);
    
    /* Update the current block header's size, event count, ... */
    a->blksiz     +=   wordsToWrite;
    a->bytesToBuf += 4*wordsToWrite;
    a->next       +=   wordsToWrite;
    a->left       -=   wordsToWrite;
    a->blkEvCount++;
    a->eventsToBuf++;
    a->currentHeader[EV_HD_BLKSIZ] = a->blksiz;
   
    if (isDictionary) {
        /* We are writing a dictionary in this (single) file */
        a->wroteDictionary = 1;
        /* Set bit in block header that there is a dictionary */
        setDictionaryBit(a->buf);
        /* Do not include dictionary in header event count.
         * Dictionaries are written in their own block. */
        a->currentHeader[EV_HD_COUNT] = 0;
    }
    else {
        a->eventCount++;
        a->currentHeader[EV_HD_COUNT] = a->blkEvCount;
        /* If we wrote a dictionary and it's the first block, don't count dictionary ... */
        if (a->wroteDictionary && a->blknum == 2 && (a->blkEvCount - 1 > 0)) {
if (debug) printf("  writeEventToBuffer: substract ev cnt since in dictionary's block\n");
            a->currentHeader[EV_HD_COUNT]--;
        }
        
        /* Signifies that we wrote an event. Used in evIoctl
         * when determined whether an event was appended already. */
        if (a->append) a->append = 2;
    }

    /* If we're writing over the last empty block header for the
     * first time (first write after opening file or flush), increment block #,
     * clear last block bit */
    if (isLastBlock(a->currentHeader)) {
        /* Always end up here if writing a dictionary */
if (debug) printf("  writeEventToBuffer: IS LAST BLOCK\n");
        clearLastBlockBit(a->currentHeader);
        /* Here is where blknum goes from 1 to 2 */
        a->blknum++;
    }
   
if (debug) printf("  writeEventToBuffer: after write,  bytesToBuf = %u, blksiz = %u, blkEvCount = %u\n",
                      a->bytesToBuf, a->blksiz, a->blkEvCount);
    
    /* (Re)Write the last, empty block header so it can be
     * flushed at any time and still create a valid file. */
    writeEmptyLastBlockHeader(a, a->blknum);
}


/**
 * This routine writes a dictionary to the internal buffer.
 * It's a simpified version of {@link #evWriteImpl()}.
 *
 * @param handle   evio handle
 * @param buffer   pointer to buffer containing event to write
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_TRUNC     if not enough room writing to a user-given buffer in {@link evOpen}
 * @return S_EVFILE_ALLOCFAIL if cannot allocate memory
 */
static int evWriteDictImpl(EVFILE *a, const uint32_t *buffer)
{
    uint32_t nToWrite, size;
    int status, debug=0;


    /* Number of words left to write = full event size + bank header */
    nToWrite = buffer[0] + 1;
    
if (debug) printf("evWriteDict: bufSize = %u <? bytesToWrite = (dict) %u + (2 hdr) 64 = %u \n",
    (4*a->bufSize), (4*nToWrite), (4*nToWrite + 64));
    
    /* Is this event (by itself) too big for the current internal buffer?
     * Internal buffer needs room for first block header, event, and ending empty block. */
    if (4*a->bufSize < 4*(nToWrite + 2*EV_HDSIZ)) {

        /* Cannot increase size of user-given buffer */
        if (a->rw == EV_WRITEBUF) {
            return(S_EVFILE_TRUNC);
        }

if (debug) printf("evWriteDict: buf size (bytes) = %d, needed for event + headers = %d\n",
        (4*a->bufSize), (4*(nToWrite + 2*EV_HDSIZ)));

        /* Increase buffer size to at least this. */
        size = 4*(nToWrite + 2*EV_HDSIZ);

        /* Increase buffer size and DON"T copy first header from old to new */
        status = expandBuffer(a, size);
        if (status != S_SUCCESS) {
            return status;
        }
        
        /* Init buffer with last empty block -
         * just like when initially calling evOpen() */
        resetBuffer(a);
    }

    /********************************************************/
    /* Now we have enough room for the event in the buffer. */
    /* So write the event to the internal buffer.           */
    /********************************************************/
    writeEventToBuffer(a, buffer, nToWrite, 1);

if (debug) {
        printf("evWriteDictImpl: after last header written, Events written to:\n");
        printf("                 cnt total (no dict) = %u\n", a->eventCount);
        printf("                 file cnt total = %u\n", a->eventsToFile);
        printf("                 internal buffer cnt = %u\n", a->eventsToBuf);
        printf("                 block cnt = %u\n", a->blkEvCount);
        printf("                 bytes-to-buf = %u\n", a->bytesToBuf);
        printf("                 block # = %u\n", a->blknum);
}

    return(S_SUCCESS);
}



/**
 * This routine initializes the internal buffer
 * as if evOpen was just called and resets some
 * evio handle structure variables.
 *
 * @param handle   evio handle
 * @param buffer   pointer to buffer containing event to write
 */
static void resetBuffer(EVFILE *a) {

    if (0) printf("    resetBuffer()\n");
    
    /* Initialize block header for next write -
     * just like when initially calling evOpen() */
    initLastBlockHeader(a->buf, a->blknum);
    a->currentHeader = a->buf;

    /* Pointer to where next to write. In this case, the start
     * of first event header will be right after header. */
    a->next = a->buf + EV_HDSIZ;
    /* Space in number of words, not in header, left for writing in block buffer */
    a->left = a->bufSize - EV_HDSIZ;
    /* Total written size (words) = block header size so far */
    a->blksiz = EV_HDSIZ;
    /* No events in block yet */
    a->blkEvCount = 0;
    
    /* Reset buffer values for reuse */
    a->bytesToBuf  = 4*EV_HDSIZ; /* First block header */
    a->eventsToBuf = 0;
}


/**
 * This routine writes an evio event to an internal buffer containing evio data.
 * If the internal buffer is full, it is flushed to the final destination
 * file/socket/buffer/pipe opened with routines {@link evOpen}, {@link evOpenBuffer},
 * or {@link evOpenSocket}. The file will possibly be split into multiple files if
 * a split size was given by calling evIoctl. Note that the split file size may be
 * <b>bigger</b> than the given limit by 32 bytes using the algorithm below.
 * It writes data in evio version 4 format and returns a status.
 *
 * @param handle   evio handle
 * @param buffer   pointer to buffer containing event to write
 * @param useMutex if != 0, use mutex locking, else no locking
 * @param isDictionary if != 0, bank being written is the dictionary
 *
 * @return S_SUCCESS          if successful
 * @return S_FAILURE          if internal programming error writing block header, or
 *                            if file/socket write error
 * @return S_EVFILE_BADMODE   if opened for reading or appending to opposite endian file/buffer.
 * @return S_EVFILE_TRUNC     if not enough room writing to a user-supplied buffer
 * @return S_EVFILE_BADARG    if buffer is NULL
 * @return S_EVFILE_BADHANDLE if bad handle arg
 * @return S_EVFILE_ALLOCFAIL if cannot allocate memory
 */
static int evWriteImpl(int handle, const uint32_t *buffer, int useMutex, int isDictionary)
{
    EVFILE   *a;
    uint32_t nToWrite, size;
    int status, debug=0, headerBytes = 4*EV_HDSIZ, splittingFile=0;
    int doFlush = 0;
    int roomInBuffer = 1;
    int needBiggerBuffer = 0;
    int writeNewBlockHeader = 1;
    int fileActuallySplit = 0;

    
    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }
    
    if (buffer == NULL) {
        return(S_EVFILE_BADARG);
    }

    /* Don't allow simultaneous calls to evClose(), but do allow reads & writes. */
    if (useMutex) handleReadLock(handle);

    /* Look up file struct (which contains block buffer) from handle */
    a = handleList[handle-1];

    /* Check args */
    if (a == NULL) {
        if (useMutex) handleReadUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }

    /* If appending and existing file/buffer is opposite endian, return error */
    if (a->append && a->byte_swapped) {
        if (useMutex) handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }
    
    /* Special case when writing directly to buffer (no block buffer) */
    /*
    if (a->rw == EV_WRITEBUF) {
        status = evWriteBuffer(a, buffer, useMutex);
        if (useMutex) handleReadUnlock(handle);
        return status;
    }
    */

    /* Need to be open for writing not reading */
    if (a->rw != EV_WRITEFILE && a->rw != EV_WRITEBUF &&
        a->rw != EV_WRITESOCK && a->rw != EV_WRITEPIPE) {
        if (useMutex) handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }

    /* Number of words left to write = full event size + bank header */
    nToWrite = buffer[0] + 1;

    /* Lock mutex for multithreaded reads/writes/access */
    if (useMutex) mutexLock(a);

    /* Use other function to write dictionary */
    if (isDictionary) {
        status = evWriteDictImpl(a, a->dictBuf);
        if (useMutex) {
            mutexUnlock(a);
            handleReadUnlock(handle);
        }
        return (status);
    }
    
    if (debug && a->splitting) {
printf("evWrite: splitting, bytesToFile = %lu (bytes), event bytes = %u, bytesToBuf = %u, split = %lu\n",
               a->bytesToFile, (4*nToWrite), a->bytesToBuf, a->split );
    }
    
    /* If we have enough room in the current block and have not exceeded
     * the number of allowed events, or this is the first event,
     * write it in the current block. */
    if ( ( ((nToWrite + a->blksiz) <= a->blkSizeTarget) &&
             a->blkEvCount < a->blkEvMax) || a->blkEvCount < 1) {
if (debug) printf("evWrite: do NOT need a new blk header\n");
        writeNewBlockHeader = 0;
    }
    else if (debug) {
printf("evWrite: DO need a new blk header: blkTarget = %u, will use %u (words)\n",
                 a->blkSizeTarget, (nToWrite + a->blksiz + EV_HDSIZ) );
        if (a->blkEvCount >= a->blkEvMax) {
printf("evWrite: too many events in block, already have %u\n", a->blkEvCount );
        }
    }

    
    /* Are we splitting files in general? */
    while (a->splitting) {
        int headerCount=0;
        uint64_t totalSize;
        
        /* If all that is written so far is a dictionary, don't split after writing it */
        if (a->wroteDictionary && (a->blknum - 1) == 1 && a->eventsToBuf < 2) {
if (debug) printf("evWrite: don't split file cause only dictionary written so far\n");
            break;
        }
        
        /* Is this event (together with the current buffer, current file,
         * and various block headers) large enough to split the file? */
        totalSize = a->bytesToFile + 4*nToWrite + a->bytesToBuf;

        /* If we have to add another block header, account for it.
         * But only if it doesn't write over an existing ending block.*/
        if (writeNewBlockHeader && a->bytesToFile < 1) {
            totalSize += headerBytes;
            headerCount++;
if (debug) printf("evWrite: account for another block header when splitting\n");
        }
        
        /* If an ending empty block was not added yet (first time thru), account for it */
        if (isLastBlock(a->currentHeader)) {
if (debug) printf("evWrite: account for adding empty last block when splitting\n");
            totalSize += headerBytes;
            headerCount++;
        }

/* If dictionary was written, do NOT include that when deciding to split */
/* totalSize -= a->dictLength; */

if (debug) printf("evWrite: splitting = %s: total size = %lu >? split = %lu\n",
                          (totalSize > a->split ? "True" : "False"),
                          totalSize, a->split);

if (debug) printf("evWrite: total size components: bytesToFile = %lu, bytesToBuf = %u, ev bytes = %u, additional headers = %d * 32, dictlen = %u\n",
        a->bytesToFile, a->bytesToBuf, 4*nToWrite, headerCount, a->dictLength);

        /* If we're going to split the file ... */
        if (totalSize > a->split) {
            /* Yep, we're gonna to do it */
            splittingFile = 1;

            /* Flush the current buffer if any events contained and prepare
             * for a new file (split) to hold the current event. */
            if (a->eventsToBuf > 0) {
                doFlush = 1;
            }
        }
        
        break;
    }

    
if (debug) printf("evWrite: bufSize = %u <? bytesToWrite = %u + 64 = %u \n",
                        (4*a->bufSize), (4*nToWrite), (4*nToWrite + 64));

    
    /* Is this event (by itself) too big for the current internal buffer?
     * Internal buffer needs room for first block header, event, and ending empty block. */
    if (4*a->bufSize < 4*nToWrite + 2*headerBytes) {
        /* Not enough room in user-supplied buffer for this event */
        if (a->rw == EV_WRITEBUF) {
            if (useMutex) {
                mutexUnlock(a);
                handleReadUnlock(handle);
            }
            return(S_EVFILE_TRUNC);
        }
        
        roomInBuffer = 0;
        needBiggerBuffer = 1;
if (debug) printf("evWrite: NEED another buffer & block for 1 big event, bufferSize = %d bytes\n",
               (4*a->bufSize));
    }
    /* Is this event, in combination with events previously written
     * to the current internal buffer, too big for it? Remember, if we're here,
     * events were previously written to this block and therefore an ending
     * empty block has already been written and included in a->bytesToBuf.
     * Also, if we're here, this event is not a dictionary. */
    else if ((!writeNewBlockHeader && ((4*a->bufSize - a->bytesToBuf) < 4*nToWrite)) ||
             ( writeNewBlockHeader && ((4*a->bufSize - a->bytesToBuf) < 4*nToWrite + headerBytes)))  {
        /* Not enough room in user-supplied buffer for this event */
        if (a->rw == EV_WRITEBUF) {
            if (useMutex) {
                mutexUnlock(a);
                handleReadUnlock(handle);
            }
            return(S_EVFILE_TRUNC);
        }
        
        if (debug) {
printf("evWrite: NEED to flush buffer and re-use, ");
            if (writeNewBlockHeader) {
printf(" buf room = %d, needed = %d\n", (4*a->bufSize - a->bytesToBuf), (4*nToWrite + headerBytes));
            }
            else {
printf(" buf room = %d, needed = %d\n", (4*a->bufSize - a->bytesToBuf), (4*nToWrite));
            }
        }
        roomInBuffer = 0;
    }
    
    /* If there is no room in the buffer for this event ... */
    if (!roomInBuffer) {
        /* If we need more room for a single event ... */
        if (needBiggerBuffer) {
            /* We're here because there is not enough room in the internal buffer
             * to write this single large event. Increase buffer to match. */
            size = 4*(nToWrite + 2*EV_HDSIZ);
if (debug) printf("         must expand, bytes needed for 1 big ev + 2 hdrs = %d\n", size);
        }
        
        /* Flush what we have to file (if anything) */
        doFlush = 1;
    }

    /* Do we flush? */
    if (doFlush) {
        status = evFlush(a);
        if (status != S_SUCCESS) {
            if (useMutex) {
                mutexUnlock(a);
                handleReadUnlock(handle);
            }
            return status;
        }
    }

    /* Do we split the file? */
    if (splittingFile) {
        fileActuallySplit = splitFile(a);
        if (fileActuallySplit == S_FAILURE) {
            if (useMutex) {
                mutexUnlock(a);
                handleReadUnlock(handle);
            }
            return S_FAILURE;
        }
    }

    /* Do we expand buffer? */
    if (needBiggerBuffer) {
        /* If here, we just flushed. */
        status = expandBuffer(a, size);
        if (status != S_SUCCESS) {
            if (useMutex) {
                mutexUnlock(a);
                handleReadUnlock(handle);
            }
            return status;
        }
    }

    /* If we either flushed events or split the file, reset the
     * internal buffer to prepare it for writing another event. */
    if (doFlush || splittingFile) {
        resetBuffer(a);
        /* We have a newly initialized buffer ready to write
         * to, so we don't need a new block header. */
        writeNewBlockHeader = 0;
    }
    
    /*********************************************************************/
    /* Now we have enough room for the event in the buffer, block & file */
    /*********************************************************************/

    /*********************************************************************/
    /* Before we go on, if the file was actually split, we must add any
     * existing dictionary as the first event & block in the new file
     * before we write the event. */
    /*********************************************************************/
    if (fileActuallySplit && a->dictionary != NULL) {
        /* Memory needed to write: dictionary + 3 block headers
         * (beginning, after dict, and ending) + event */
        uint32_t neededBytes = a->dictLength + 3*4*EV_HDSIZ + 4*nToWrite;
if (debug) printf("evWrite: write DICTIONARY after splitting, needed bytes = %u\n", neededBytes);

        /* Write block header after dictionary */
        writeNewBlockHeader = 1;

        /* Give us more buffer memory if we need it. */
        status = expandBuffer(a, neededBytes);
        if (status != S_SUCCESS) {
            if (useMutex) {
                mutexUnlock(a);
                handleReadUnlock(handle);
            }
            return status;
        }

        resetBuffer(a);

        /* Write dictionary to the internal buffer */
        status = evWriteDictImpl(a, a->dictBuf);
        if (status != S_SUCCESS) {
            if (useMutex) {
                mutexUnlock(a);
                handleReadUnlock(handle);
            }
            return status;
        }

        /* Now continue with writing the event... */
    }

    /* Write new block header if required */
    if (writeNewBlockHeader) {
        status = writeNewHeader(a, 8, 1, a->blknum++, 0, 0, 1, 0);
         if (status != S_SUCCESS) {
             if (useMutex) {
                 mutexUnlock(a);
                 handleReadUnlock(handle);
             }
             return(status);
         }
         a->bytesToBuf -= 4*EV_HDSIZ; /* write over last empty block */
if (debug) printf("evWrite: wrote new block header, bytesToBuf = %u\n", a->bytesToBuf);
    }
    else {
if (debug) printf("evWrite: did NOT write new block header, current header info word = 0x%x\n",
                          a->currentHeader[EV_HD_VER]);
        
        /* Write over last empty block ... only if not first write */
        if (!isLastBlock(a->currentHeader)) {
if (debug) printf("evWrite: no block header WRITE OVER LAST EMPTY BLOCK\n");
            a->bytesToBuf -= 4*EV_HDSIZ;
        }
    }

    /******************************************/
    /* Write the event to the internal buffer */
    /******************************************/
    writeEventToBuffer(a, buffer, nToWrite, isDictionary);

if (debug) {
        printf("evWrite: after last header written, Events written to:\n");
        printf("         cnt total (no dict) = %u\n", a->eventCount);
        printf("         file cnt total = %u\n", a->eventsToFile);
        printf("         internal buffer cnt = %u\n", a->eventsToBuf);
        printf("         block cnt = %u\n", a->blkEvCount);
        printf("         bytes-to-buf  = %u\n", a->bytesToBuf);
        printf("         bytes-to-file = %u\n", a->bytesToFile);
        printf("         block # = %u\n", a->blknum);
}

    if (useMutex) {
        mutexUnlock(a);
        handleReadUnlock(handle);
    }

    return(S_SUCCESS);
}


/**
 * This routine writes any existing evio format data in an internal buffer
 * (written to with {@link evWrite}) to the final destination
 * file/socket opened with routines {@link evOpen} or {@link evOpenSocket}.
 * It writes data in evio version 4 format and returns a status.
 * If writing to a file, it always places an empty block
 * at the end - marked as the last block. If more events are written, the
 * last block is overwritten.<p>
 * This will not overwrite an existing file if splitting is enabled.
 *
 * @param a  pointer to data structure
 *
 * @return S_SUCCESS  if successful
 * @return S_FAILURE  if file could not be opened for writing;
 *                    if file exists already and splitting;
 *                    if file name could not be generated;
 *                    if error writing event;
 */
static int evFlush(EVFILE *a) {
    long pos;
    uint32_t nBytes, debug=0, bytesToWrite=0, blockHeaderBytes = 4*EV_HDSIZ;

    
    /* If nothing to write ... */
    if (a->eventsToBuf < 1) {
if (debug) printf("    evFlush: no events to write\n");
        return(S_SUCCESS);
    }

    /* How much data do we write? */
    bytesToWrite = a->bytesToBuf;
    
    /* Write internal buffer out to socket, file, or pipe */
    if (a->rw == EV_WRITESOCK) {
        /* Unlike the file, do NOT write out the last, empty block now.
         * Otherwise the receiving socket will end it's reading of
         * events. Do that only when closing. */
        bytesToWrite = a->bytesToBuf - 4*EV_HDSIZ;
if (debug) printf("    evFlush: write %u events to SOCKET\n", a->eventsToBuf);
        nBytes = tcpWrite(a->sockFd, a->buf, bytesToWrite);
    }
    else if (a->rw == EV_WRITEPIPE) {
        /* Unlike the file, do NOT write out the last, empty block now.
         * Otherwise the receiving program will end it's reading of
         * events. Do that only when closing. */
        bytesToWrite = a->bytesToBuf - 4*EV_HDSIZ;
if (debug) printf("    evFlush: write %u events to PIPE\n", a->eventsToBuf);
        /* Write block to file */
        nBytes = fwrite((const void *)a->buf, 1, bytesToWrite, a->file);
        /* Return any error condition of file stream */
        if (ferror(a->file)) return(S_FAILURE);
    }
    else if (a->rw == EV_WRITEFILE) {
if (debug) printf("    evFlush: write %u events to FILE\n", a->eventsToBuf);
        /* Clear EOF and error indicators for file stream */
        if (a->file != NULL) clearerr(a->file);

        /* If nothing has been written to file yet ... */
        if (a->bytesToFile < 1) {

            /* Create the file now if necessary */
            if (a->file == NULL) {

                /* Generate the file name if not done yet (very first file) */
                if (a->fileName == NULL) {
                    /* Generate actual file name from base name */
                    char *fname = evGenerateFileName(a, a->specifierCount, a->runNumber,
                                                     (a->splitting? a->split : 0),
                                                      a->splitNumber++, a->runType);
                    if (fname == NULL) {
                        return(S_FAILURE);
                    }
                    a->fileName = fname;
if (debug) printf("    evFlush: generate first file name = %s\n", a->fileName);
                }

if (debug) printf("    evFlush: create file = %s\n", a->fileName);

                /* If splitting, don't overwrite a file ... */
                if (a->splitting) {
                    if (fileExists(a->fileName)) {
if (1) printf("    evFlush: will not overwrite file = %s\n", a->fileName);
                        return(S_FAILURE);
                    }
                }
        
                a->file = fopen(a->fileName,"w");
                if (a->file == NULL) {
                    return(S_FAILURE);
                }
            }
        }
        /* If appending to existing data, write over last block header (back up 32 bytes) */
        else {
if (debug) printf("    evFlush: append\n");
            /* Carefully change unsigned int (blockHeaderBytes) into neg # */
            pos = -1L * blockHeaderBytes;
            fseek(a->file, pos, SEEK_CUR);
        }
if (debug) printf("    evFlush: write %d bytes\n", bytesToWrite);

        /* Write block to file */
        nBytes = fwrite((const void *)a->buf, 1, bytesToWrite, a->file);

// I suspect this line kills performance so just remove it for now.
//        fflush(a->file);
        
        /* Return for error condition of file stream */
        if (ferror(a->file)) return(S_FAILURE);
    }

    /* Return any error condition of write attempt */
    if (nBytes != bytesToWrite) {
if (debug) printf("    evFlush: did NOT write all bytes!!!\n");
        return(S_FAILURE);
    }

    a->bytesToFile  += bytesToWrite;
    a->eventsToFile += a->eventsToBuf;
    
if (debug) printf("    evFlush: file cnt total = %u\n", a->eventsToFile);

    return(S_SUCCESS);
}


/**
 * This routine splits the file being written to.
 * Does nothing when output destination is not a file.
 * It resets file variables, closes the old file, and opens the new.
 *
 * @param a  pointer to data structure
 *
 * @return  1  if file actually split
 * @return  0  if no error but file not split
 * @return -1  if mapped memory does not unmap;
 *             if failure to generate file name;
 *             if failure to close file;
 */
static int splitFile(EVFILE *a) {
    char *fname;
    int err, status=1, debug=0;

    
    /* Only makes sense when writing to files */
    if (a->rw != EV_WRITEFILE) {
        return(0);
    }

    /* Reset first-block & file values for reuse */
    a->blknum = 1;
    a->bytesToFile  = 0;
    a->eventsToFile = 0;

    /* Close file */
    if (a->rw == EV_WRITEFILE) {
        if (a->randomAccess) {
            err = munmap(a->mmapFile, a->mmapFileSize);
            if (err < 0) {
if (debug) printf("splitFile: error unmapping memory, %s\n", strerror(errno));
                status = -1;
            }
            
            if (a->pTable != NULL) {
                free(a->pTable);
            }
        }
        else {
            if (a->file != NULL) {
                err = fclose(a->file);
            }
            if (err == EOF) {
if (debug) printf("splitFile: error closing file, %s\n", strerror(errno));
                status = -1;
            }
        }
    }

    /* Right now no file is open for writing */
    a->file = NULL;

    /* Create the next file's name */
    fname = evGenerateFileName(a, a->specifierCount, a->runNumber,
                               a->split, a->splitNumber++, a->runType);
    if (fname == NULL) {
        return(-1);
    }

    if (a->fileName != NULL) {
        free(a->fileName);
    }
    
    a->fileName = fname;
    
if (debug) printf("    splitFile: generate next file name = %s\n", a->fileName);
    
    return(status);
}


/** Fortran interface to {@link evClose}. */
#ifdef AbsoftUNIXFortran
int evclose
#else
int evclose_
#endif
(int *handle)
{
    return(evClose(*handle));
}


/**
 * This routine flushes any existing evio format data in an internal buffer
 * (written to with {@link evWrite}) to the final destination
 * file/socket/buffer opened with routines {@link evOpen}, {@link evOpenBuffer},
 * or {@link evOpenSocket}.
 * It also frees up the handle so it cannot be used any more without calling
 * {@link evOpen} again.
 * Any data written is in evio version 4 format and any opened file is closed.
 * If reading, nothing is done.
 *
 * @param handle evio handle
 *
 * @return S_SUCCESS          if successful
 * @return S_FAILURE          if mapped memory does not unmap or pclose() failed
 * @return S_EVFILE_TRUNC     if not enough room writing to a user-given buffer in {@link evOpen}
 * @return S_EVFILE_BADHANDLE if bad handle arg
 * @return fclose error       if fclose error
 */
int evClose(int handle)
{
    EVFILE *a;
    int debug=0, status = S_SUCCESS, status2 = S_SUCCESS;
    int nBytes, bytesToWrite;
    

    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }

if (debug) printf("evClose: try to grab evClose() mutex\n");
    /* Don't let no one get no "a" while we're closing anything */
    handleWriteLock(handle);

    /* Look up file struct from handle */
    a = handleList[handle-1];

    /* Check arg */
    if (a == NULL) {
        handleWriteUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }

if (debug) printf("  evClose: in\n");

    /* Flush everything to file/socket if writing */
    if (a->rw == EV_WRITEFILE || a->rw == EV_WRITEPIPE || a->rw == EV_WRITESOCK)  {
        /* Write last block. If no events left to write,
         * just write an empty block (header only). */
        evFlush(a);
    }

    /* For writing to sockets or pipes, send the last empty block now */
    if (a->rw == EV_WRITEPIPE || a->rw == EV_WRITESOCK) {
        /* Mark this block as the last one to be written. */
        resetBuffer(a);
        bytesToWrite = 4*EV_HDSIZ;
       
        if (a->rw == EV_WRITESOCK) {
if (debug) printf("    evClose: send last empty block over socket\n");
            nBytes = tcpWrite(a->sockFd, a->buf, bytesToWrite);
            if (nBytes != bytesToWrite) {
if (debug) printf("    evClose: didn't write all bytes, err = %s\n", strerror(errno));
                return(errno);
            }
        }
        else if (a->rw == EV_WRITEPIPE) {
if (debug) printf("    evClose: send last empty block over pipe\n");
            nBytes = fwrite((const void *)a->buf, 1, bytesToWrite, a->file);
            if (nBytes != bytesToWrite) {
if (debug) printf("    evClose: didn't write all bytes, err = %s\n", strerror(errno));
                return(errno);
            }
        }
    }


    /* Close file */
    if (a->rw == EV_WRITEFILE || a->rw == EV_READFILE) {
        if (a->randomAccess) {
            status2 = munmap(a->mmapFile, a->mmapFileSize);
            if (a->pTable != NULL) free(a->pTable);
        }
        else {
            if (a->file != NULL) status2 = fclose(a->file);
        }
    }
    /* Pipes requires special close */
    else if (a->rw == EV_READPIPE || a->rw == EV_WRITEPIPE) {
        if (a->file != NULL) {
            status2 = pclose(a->file);
            if (status2 < 0) status2 = S_FAILURE;
            else status2 = S_SUCCESS;
        }
    }

    /* Free up resources */
    if (a->buf != NULL && a->rw != EV_WRITEBUF) free((void *)(a->buf));
    if (a->dictBuf      != NULL) free(a->dictBuf);
    if (a->dictionary   != NULL) free(a->dictionary);
    if (a->fileName     != NULL) free(a->fileName);
    if (a->baseFileName != NULL) free(a->baseFileName);
    if (a->runType      != NULL) free(a->runType);
    structDestroy(a);
    free((void *)a);
    
    /* Remove this handle from the list */
    getHandleLock();
    handleList[handle-1] = 0;
    getHandleUnlock();

    handleWriteUnlock(handle);
if (debug) printf("evClose: release close mutex\n");
    
    if (status == S_SUCCESS) {
        status = status2;
    }
    
    return(status);
}


/**
 * This routine returns the number of bytes written into a buffer so
 * far when given a handle provided by calling {@link evOpenBuffer}.
 * After the handle is closed, this no longer returns anything valid.
 *
 * @param handle evio handle
 * @param length pointer to int which gets filled with number of bytes
 *               written to buffer so far
 *
 * @return S_SUCCESS          if successful
 * @return S_EVFILE_BADHANDLE if bad handle arg
 */
int evGetBufferLength(int handle, uint32_t *length)
{
    EVFILE *a;

    
    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }

    /* Don't allow simultaneous calls to evClose(), but do allow reads & writes. */
    handleReadLock(handle);

    /* Look up file struct (which contains block buffer) from handle */
    a = handleList[handle-1];

    /* Check arg */
    if (a == NULL) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }
    
    if (length != NULL) {
        *length = a->rwBytesOut;
    }
    
    handleReadUnlock(handle);
 
    return(S_SUCCESS);
}


/** Fortran interface to {@link evIoctl}. */
#ifdef AbsoftUNIXFortran
int evioctl
#else
int evioctl_
#endif
(int *handle, char *request, void *argp, int reqlen)
{
    char *req;
    int32_t status;
    req = (char *)malloc(reqlen+1);
    strncpy(req,request,reqlen);
    req[reqlen]=0;		/* insure request is null terminated */
    status = evIoctl(*handle,req,argp);
    free(req);
    return(status);
}


/**
 * This routine changes various evio parameters used in reading and writing.<p>
 *
 * It changes the target block size (in 32-bit words) for writes if request = "B".
 * If setting block size fails, writes can still continue with original
 * block size. Minimum size = 1K + 8(header) words. Max size = 5120000 words.<p>
 *
 * It changes size of buffer (in 32-bit words) for writing to file/socket/pipe
 * if request = "W". Must be >= block size + ending header (8).
 * Max size = 5120008 words.<p>
 *
 * It changes the maximum number of events/block if request = "N".
 * It only goes up to EV_EVENTS_MAX or 100,000.
 * Used only in version 4.<p>
 *
 * It changes the number of bytes at which to split a file
 * being written to if request = "S". If unset with this function,
 * it defaults to EV_SPLIT_SIZE (2GB). NOTE: argp must point to
 * 64 bit integer (not 32 bit)!
 * Used only in version 4.<p>
 *
 * It sets the run number used when auto naming while splitting files
 * being written to if request = "R".
 * Used only in version 4.<p>
 *
 * It sets the run type used when auto naming while splitting files
 * being written to if request = "T".
 * Used only in version 4.<p>
 *
 * It returns the version number if request = "V".<p>
 * 
 * It returns the total number of events in a file/buffer
 * opened for reading or writing if request = "E". Includes any
 * event added with {@link evWrite} call. Used only in version 4.<p>
 * 
 * It returns a pointer to the EV_HDSIZ block header ints if request = "H".
 * This pointer must be freed by the caller to avoid a memory leak.<p>
 * 
 * NOTE: all request strings are case insensitive. All version 4 commands to
 * version 3 files are ignored.
 *
 * @param handle  evio handle
 * @param request case independent string value of:
 * <OL type=1>
 * <LI>  "B"  for setting target block size for writing in words
 * <LI>  "W"  for setting writing (to file) internal buffer size in words
 * <LI>  "N"  for setting max # of events/block
 * <LI>  "R"  for setting run number (used in file splitting)
 * <LI>  "T"  for setting run type   (used in file splitting)
 * <LI>  "S"  for setting file split size in bytes
 * <LI>  "V"  for getting evio version #
 * <LI>  "H"  for getting 8 ints of block header info
 * <LI>  "E"  for getting # of events in file/buffer
 * </OL>
 *
 * @param argp
 * <OL type=1>
 * <LI> pointer to uin32_t containing new block size in 32-bit words if request = B, or
 * <LI> pointer to uin32_t containing new buffer size in 32-bit words if request = W, or
 * <LI> pointer to uin32_t containing new max # of events/block if request = N, or
 * <LI> pointer to uin32_t containing run number if request = R, or
 * <LI> pointer to character containing run type if request = T, or
 * <LI> pointer to int32_t returning version # if request = V, or
 * <LI> pointer to <b>uint64_t</b> containing max size in bytes of split file if request = S, or
 * <LI> address of pointer to uint32_t returning pointer to 8
 *              uint32_t's of block header if request = H. This pointer must be
 *              freed by caller since it points to allocated memory
 * <LI> pointer to uin32_t returning total # of original events in existing
 *              file/buffer when reading or appending if request = E, or
 * </OL>
 *
 * @return S_SUCCESS           if successful
 * @return S_FAILURE           if using sockets when request = E
 * @return S_EVFILE_BADARG     if request is NULL or argp is NULL
 * @return S_EVFILE_BADFILE    if file too small or problem reading file when request = E
 * @return S_EVFILE_BADHANDLE  if bad handle arg
 * @return S_EVFILE_ALLOCFAIL  if cannot allocate memory
 * @return S_EVFILE_UNXPTDEOF  if buffer too small when request = E
 * @return S_EVFILE_UNKOPTION  if unknown option specified in request arg
 * @return S_EVFILE_BADSIZEREQ  when setting block/buffer size - if currently reading,
 *                              have already written events with different block size,
 *                              or is smaller than min allowed size (header size + 1K),
 *                              or is larger than the max allowed size (5120000 words);
 *                              when setting max # events/blk - if val < 1
 * @return errno                if error in fseek, ftell when request = E
 */
int evIoctl(int handle, char *request, void *argp)
{
    EVFILE   *a;
    uint32_t *newBuf, *pHeader;
    int       err, debug=0;
    char     *runType;
    int32_t   lockOff;
    uint32_t  eventsMax, blockSize, bufferSize, runNumber;
    uint64_t  splitSize;

    
    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }
    
    /* Don't allow simultaneous calls to evClose, reads & writes. */
    handleWriteLock(handle);

    /* Look up file struct from handle */
    a = handleList[handle-1];

    /* Check args */
    if (a == NULL) {
        handleWriteUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }
    
    if (request == NULL) {
        handleWriteUnlock(handle);
        return(S_EVFILE_BADARG);
    }

    switch (*request) {
        /*******************************/
        /* Specifing target block size */
        /*******************************/
        case 'b':
        case 'B':
            /* Need to specify block size */
            if (argp == NULL) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADARG);
            }

            /* Need to be writing not reading */
            if (a->rw != EV_WRITEFILE &&
                a->rw != EV_WRITEPIPE &&
                a->rw != EV_WRITESOCK &&
                a->rw != EV_WRITEBUF) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }

            /* If not appending AND events already written ... */
            if (a->append == 0 && (a->blknum != 1 || a->blkEvCount != 0)) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }
            /* Else appending AND events already appended ... */
            else if (a->append > 1) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }

            /* Read in requested target block size */
            blockSize = *(uint32_t *) argp;

            /* If there is no change, return success */
            if (blockSize == a->blkSizeTarget) {
                handleWriteUnlock(handle);
                return(S_SUCCESS);
            }
            
            /* If it's too small, return error */
            if (blockSize < EV_BLOCKSIZE_MIN) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }

            /* If it's too big, return error */
            if (blockSize > EV_BLOCKSIZE_MAX) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }

            /* If we need a bigger buffer ... */
            if (blockSize + EV_HDSIZ > a->bufRealSize && a->rw != EV_WRITEBUF) {
                /* Allocate buffer memory for increased block size. If this fails
                 * we can still (theoretically) continue with writing. */
if (debug) printf("INcreasing block size to %u words\n", (blockSize + EV_HDSIZ));
                newBuf = (uint32_t *) malloc(4*(blockSize + EV_HDSIZ));
                if (newBuf == NULL) {
                    handleWriteUnlock(handle);
                    return(S_EVFILE_ALLOCFAIL);
                }
            
                /* Free allocated memory for current buffer */
                free(a->buf);

                /* New buffer stored here */
                a->buf = newBuf;
                /* Current header is at top of new buffer */
                a->currentHeader = a->buf;

                /* Initialize block header */
                initLastBlockHeader(a->buf, 1);
                
                /* Remember size of new buffer */
                a->bufRealSize = a->bufSize = (blockSize + EV_HDSIZ);
            }
            else if (blockSize + EV_HDSIZ > a->bufSize && a->rw != EV_WRITEBUF) {
                /* Remember how much of buffer is actually being used */
                a->bufSize = blockSize + EV_HDSIZ;
            }
            else if (debug) {
printf("DEcreasing block size to %u words\n", blockSize);
            }
                  
            /* Reset some file struct members */

            /* Recalculate how many words are left to write in block */
            a->left = blockSize - EV_HDSIZ;
            /* Store new target block size (final size,
             * a->blksiz, may be larger or smaller) */
            a->blkSizeTarget = blockSize;
            /* Next word to write is right after header */
            a->next = a->buf + EV_HDSIZ;
            
            break;

            /****************************************************/
            /* Specifing buffer size for writing file/sock/pipe */
            /****************************************************/
        case 'w':
        case 'W':
            /* Need to specify buffer size */
            if (argp == NULL) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADARG);
            }

            /* Need to be writing file/sock/pipe, not reading */
            if (a->rw != EV_WRITEFILE &&
                a->rw != EV_WRITEPIPE &&
                a->rw != EV_WRITESOCK) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }

            /* If not appending AND events already written ... */
            if (a->append == 0 && (a->blknum != 1 || a->blkEvCount != 0)) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }
            /* Else appending AND events already appended ... */
            else if (a->append > 1) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }

            /* Read in requested buffer size */
            bufferSize = *(uint32_t *) argp;

            /* If there is no change, return success */
            if (bufferSize == a->bufSize) {
                handleWriteUnlock(handle);
                return(S_SUCCESS);
            }

            /* If it's too small, return error */
            if (bufferSize < a->blkSizeTarget + EV_HDSIZ) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }

            /* If it's too big, return error */
            if (bufferSize > EV_BLOCKSIZE_MAX + EV_HDSIZ) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }

            /* If we need a bigger buffer ... */
            if (bufferSize > a->bufRealSize && a->rw != EV_WRITEBUF) {
                /* Allocate buffer memory for increased size. If this fails
                 * we can still (theoretically) continue with writing. */
if (debug) printf("INcreasing internal buffer size to %u words\n", bufferSize);
                newBuf = (uint32_t *) malloc(bufferSize*4);
                if (newBuf == NULL) {
                    handleWriteUnlock(handle);
                    return(S_EVFILE_ALLOCFAIL);
                }

                /* Free allocated memory for current block */
                free(a->buf);

                /* New buffer stored here */
                a->buf = newBuf;
                /* Current header is at top of new buffer */
                a->currentHeader = a->buf;

                /* Remember size of new buffer */
                a->bufRealSize = bufferSize;

                /* Initialize block header */
                initLastBlockHeader(a->buf, 1);
            }
            else if (debug) {
printf("DEcreasing internal buffer size to %u words\n", bufferSize);
            }
            
            /* Reset some file struct members */

            /* Remember how much of buffer is actually being used */
            a->bufSize = bufferSize;

            /* Recalculate how many words are left to write in block */
            a->left = bufferSize - EV_HDSIZ;
            /* Next word to write is right after header */
            a->next = a->buf + EV_HDSIZ;

            break;
        

        /**************************/
        /* Getting version number */
        /**************************/
        case 'v':
        case 'V':
            /* Need to pass version back in pointer to int */
            if (argp == NULL) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADARG);
            }

            *((int32_t *) argp) = a->buf[EV_HD_VER] & EV_VERSION_MASK;
            
            break;

        /*****************************/
        /* Getting block header info */
        /*****************************/
        case 'h':
        case 'H':
            /* Need to pass header data back in allocated int array */
            if (argp == NULL) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADARG);
            }
            
            pHeader = (uint32_t *)malloc(EV_HDSIZ*sizeof(int));
            if (pHeader == NULL) {
                handleWriteUnlock(handle);
                return(S_EVFILE_ALLOCFAIL);
            }
            
            memcpy((void *)pHeader, (const void *)a->buf, EV_HDSIZ*sizeof(int));
            *((uint32_t **) argp) = pHeader;
            
            break;

        /**********************************************/
        /* Setting maximum number of events per block */
        /**********************************************/
        case 'n':
        case 'N':
            /* Need to specify # of events */
            if (argp == NULL) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADARG);
            }
            
            eventsMax = *(uint32_t *) argp;
            if (eventsMax < 1) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }

            if (eventsMax > EV_EVENTS_MAX) {
                eventsMax = EV_EVENTS_MAX;
            }
            
            a->blkEvMax = eventsMax;
            break;

        /**************************************************/
        /* Setting number of bytes at which to split file */
        /**************************************************/
        case 's':
        case 'S':
            /* Need to specify split size */
            if (argp == NULL) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADARG);
            }
            
            splitSize = *(uint64_t *) argp;

            /* Make sure it is at least 32 bytes below the max file size
             * on this platform. The algorithm used to split is only
             * accurate to within +1 block header. */

            /* If this is a 32 bit operating system ... */
            if (8*sizeof(0L) == 32) {
                uint64_t max = 0x00000000ffffffff;
                if (splitSize > max - 32) {
                    splitSize = max - 32;
                }
            }
            /* If this is a 64 bit operating system or higher ... */
            else {
                uint64_t max = UINT64_MAX;
                if (splitSize > max - 32) {
                    splitSize = max - 32;
                }
            }

            /* Smallest possible evio format file = 18 32-bit ints.
             * Must also be bigger than a single buffer. */
            if (splitSize < 4*18 || splitSize < 4*a->bufSize) {
if (debug) printf("evIoctl: split file size is too small! (%lu bytes), must be min %u\n",
                  splitSize, 4*a->bufSize);
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }
            
            a->split = splitSize;
if (debug) printf("evIoctl: split file at %lu (0x%x) bytes\n", splitSize, splitSize);
            break;

        /************************************************/
        /* Setting run number for file splitting/naming */
        /************************************************/
        case 'r':
        case 'R':
            /* Need to specify run # */
            if (argp == NULL) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADARG);
            }
            
            runNumber = *(uint32_t *) argp;
            if (runNumber < 1) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADSIZEREQ);
            }
            
            a->runNumber = runNumber;
            break;

        /************************************************/
        /* Setting run type for file splitting/naming   */
        /************************************************/
        case 't':
        case 'T':
            /* Need to specify run type */
            if (argp == NULL) {
                runType = NULL;
            }
            else {
                runType = strdup((char *) argp);
                if (runType == NULL) {
                    handleWriteUnlock(handle);
                    return(S_EVFILE_BADSIZEREQ);
                }
            }
            
            a->runType = runType;
            break;

        /****************************/
        /* Getting number of events */
        /****************************/
        case 'e':
        case 'E':
            /* Need to pass # of events bank in pointer to int */
            if (argp == NULL) {
                handleWriteUnlock(handle);
                return(S_EVFILE_BADARG);
            }

            err = getEventCount(a, (uint32_t *) argp);
            if (err != S_SUCCESS) {
                handleWriteUnlock(handle);
                return(err);
            }

            break;

        default:
            handleWriteUnlock(handle);
            return(S_EVFILE_UNKOPTION);
    }

    handleWriteUnlock(handle);

    return(S_SUCCESS);
}



/**
 * This routine gets an array of event pointers if the handle was opened in
 * random access mode. User must not change the pointers in the array or the
 * data being pointed to (hence the consts in the second parameter).
 *
 * @param handle  evio handle
 * @param table   pointer to array of uint32_t pointers which gets filled
 *                with an array of event pointers.
 *                If this arg = NULL, error is returned.
 * @param len     pointer to int which gets filled with the number of
 *                pointers in the array. If this arg = NULL, error is returned.
 *                   
 * @return S_SUCCESS           if successful
 * @return S_EVFILE_BADMODE    if handle not opened in random access mode
 * @return S_EVFILE_BADARG     if table or len arg(s) is NULL
 * @return S_EVFILE_BADHANDLE  if bad handle arg
 */
int evGetRandomAccessTable(int handle, const uint32_t ***table, uint32_t *len) {
    EVFILE *a;

    
    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }
    /* Don't allow simultaneous calls to evClose(), but do allow reads & writes. */
    handleReadLock(handle);

    /* Look up file struct from handle */
    a = handleList[handle-1];

    /* Check args */
    if (a == NULL) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }

    if (table == NULL || len == NULL) {
        handleReadUnlock(handle);
        return (S_EVFILE_BADARG);
    }

    /* Must be in random access mode */
    if (!a->randomAccess) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }
            
    *table = a->pTable;
    *len = a->eventCount;

    handleReadUnlock(handle);

    return S_SUCCESS;
}



/**
 * This routine gets the dictionary associated with this handle
 * if there is any. Memory must be freed by caller if a dictionary
 * was successfully returned.
 *
 * @param handle     evio handle
 * @param dictionary pointer to string which gets filled with dictionary
 *                   string (xml) if it exists, else gets filled with NULL.
 *                   Memory for dictionary allocated here, must be freed by
 *                   caller.
 * @param len        pointer to int which gets filled with dictionary
 *                   string length if there is one, else filled with 0.
 *                   If this arg = NULL, no len is returned.
 *
 * @return S_SUCCESS           if successful
 * @return S_EVFILE_BADARG     if dictionary arg is NULL
 * @return S_EVFILE_ALLOCFAIL  if cannot allocate memory
 * @return S_EVFILE_BADHANDLE  if bad handle arg
 */
int evGetDictionary(int handle, char **dictionary, uint32_t *len) {
    EVFILE *a;
    char *dictCpy;

    
    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }
    /* Don't allow simultaneous calls to evClose(), but do allow reads & writes. */
    handleReadLock(handle);

    /* Look up file struct from handle */
    a = handleList[handle-1];

    /* Check args */
    if (a == NULL) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }

    if (dictionary == NULL) {
        handleReadUnlock(handle);
        return (S_EVFILE_BADARG);
    }

    /* Lock mutex for multithreaded reads/writes/access */
    mutexLock(a);

    /* If we have a dictionary ... */
    if (a->dictionary != NULL) {
        /* Copy it and return it */
        dictCpy = strdup(a->dictionary);
        if (dictCpy == NULL) {
            mutexUnlock(a);
            handleReadUnlock(handle);
            return(S_EVFILE_ALLOCFAIL);
        }
        *dictionary = dictCpy;

        /* Send back the length if caller wants it */
        if (len != NULL) {
            *len = strlen(a->dictionary);
        }
    }
    else {
        *dictionary = NULL;
        if (len != NULL) {
            *len = 0;
        }
    }
    
    mutexUnlock(a);
    handleReadUnlock(handle);

    return S_SUCCESS;
}


/**
 * This routine writes an optional dictionary as the first event of an
 * evio file/socket/buffer. The dictionary is <b>not</b> included in
 * any event count.
 *
 * @param handle        evio handle
 * @param xmlDictionary string containing xml format dictionary or
 *                      NULL to remove previously specified dictionary
 *
 * @return S_SUCCESS           if successful
 * @return S_FAILURE           if already written events/dictionary
 * @return S_EVFILE_BADMODE    if reading or appending
 * @return S_EVFILE_BADARG     if dictionary in wrong format
 * @return S_EVFILE_ALLOCFAIL  if cannot allocate memory
 * @return S_EVFILE_BADHANDLE  if bad handle arg
 * @return errno               if file/socket write error
 * @return stream error        if file stream error
 */
int evWriteDictionary(int handle, char *xmlDictionary)
{
    EVFILE   *a;
    char     *pChar;
    uint32_t *dictBuf;
    int       i, status, dictionaryLen, padSize, bufSize, pads[] = {4,3,2,1};

    
    if (handle < 1 || handle > handleCount) {
        return(S_EVFILE_BADHANDLE);
    }
    /* Don't allow simultaneous calls to evClose(), but do allow reads & writes. */
    handleReadLock(handle);

    /* Look up file struct from handle */
    a = handleList[handle-1];

    /* Check args */
    if (a == NULL) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADHANDLE);
    }
    
    /* Minimum length of dictionary xml to be in proper format is 35 chars. */
    if (strlen(xmlDictionary) < 35) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADARG);
    }
            
    /* Need to be writing not reading */
    if (a->rw != EV_WRITEFILE && a->rw != EV_WRITEPIPE &&
        a->rw != EV_WRITEBUF  && a->rw != EV_WRITESOCK) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }
    
    /* Cannot be appending */
    if (a->append) {
        handleReadUnlock(handle);
        return(S_EVFILE_BADMODE);
    }

    /* Lock mutex for multithreaded reads/writes/access */
    mutexLock(a);

    /* Cannot have already written dictionary or a single event */
    if (a->blknum != 1 || a->eventCount > 0 || a->wroteDictionary) {
        mutexUnlock(a);
        handleReadUnlock(handle);
        return(S_FAILURE);
    }

    /* Clear any previously specified dictionary (should never happen) */
    if (a->dictionary != NULL) {
        free(a->dictionary);
    }

    /* Store dictionary */
    a->dictionary = strdup(xmlDictionary);
    if (a->dictionary == NULL) {
        mutexUnlock(a);
        handleReadUnlock(handle);
        return(S_EVFILE_ALLOCFAIL);
    }
    dictionaryLen = strlen(xmlDictionary);

    /* Wrap dictionary in bank. Format for string array requires that
    * each string is terminated by a NULL and there must be at least
    * one ASCII char = '\4' for padding at the end (tells us that this
    * is the new format capable of storing arrays and not just a
    * single string). Add any necessary padding to 4 byte boundaries
    * with char = '\4'. */
    padSize = pads[ (dictionaryLen + 1)%4 ];

    /* Size (in bytes) = 2 header ints + dictionary chars + NULL + padding */
    bufSize = 2*sizeof(int32_t) + dictionaryLen + 1 + padSize;

    /* Allocate memory */
    dictBuf = (uint32_t *) malloc(bufSize);
    if (dictBuf == NULL) {
        free(a->dictionary);
        mutexUnlock(a);
        handleReadUnlock(handle);
        return(S_EVFILE_ALLOCFAIL);
    }

    /* Write bank length (in 32 bit ints) */
    dictBuf[0] = bufSize/4 - 1;
    /* Write tag(0), type(8 bit char *), & num(0) (strings are self-padding) */
    dictBuf[1] = 0x3 << 8;
    /* Write chars */
    pChar = (char *) (dictBuf + 2);
    memcpy((void *) pChar, (const void *)xmlDictionary, dictionaryLen);
    pChar += dictionaryLen;
    /* Write NULL */
    *pChar = '\0';
    pChar++;
    /* Write padding */
    for (i=0; i < padSize; i++) {
        *pChar = '\4';
        pChar++;
    }

    /* Store bank containing dictionary and its length (bytes) */
    a->dictBuf = dictBuf;
    a->dictLength = bufSize;

    /* Write dictionary */
    status = evWriteDictImpl(a, dictBuf);

    mutexUnlock(a);
    handleReadUnlock(handle);

    return status;
}


/**
 * This routine returns a string representation of an evio type.
 *
 * @param type numerical value of an evio type
 * @return string representation of the given type
 */
const char *evGetTypename(int type) {

    switch (type) {

        case 0x0:
            return("unknown32");
            break;

        case 0x1:
            return("uint32");
            break;

        case 0x2:
            return("float32");
            break;

        case 0x3:
            return("string");
            break;

        case 0x4:
            return("int16");
            break;

        case 0x5:
            return("uint16");
            break;

        case 0x6:
            return("int8");
            break;

        case 0x7:
            return("uint8");
            break;

        case 0x8:
            return("float64");
            break;

        case 0x9:
            return("int64");
            break;

        case 0xa:
            return("uint64");
            break;

        case 0xb:
            return("int32");
            break;

        case 0xe:
        case 0x10:
            return("bank");
            break;

        case 0xd:
        case 0x20:
            return("segment");
            break;

        case 0xc:
            return("tagsegment");
            break;

        case 0xf:
            return("composite");
            break;

        default:
            return("unknown");
            break;
    }
}



/**
 * This routine return true (1) if given type is a container, else returns false (0).
 *
 * @param type numerical value of an evio type
 * @return 1 if given type is a container, else 0.
 */
int evIsContainer(int type) {

    switch (type) {
    
        case (0xc):
        case (0xd):
        case (0xe):
        case (0x10):
        case (0x20):
            return(1);

        default:
            return(0);
    }
}


/**
 * This routine returns a string describing the given error value.
 * The returned string is a static char array. This means it is not
 * thread-safe and will be overwritten on subsequent calls.
 *
 * @param error error condition
 *
 * @returns error string
 */
char *evPerror(int error) {

    static char temp[256];

    switch(error) {

        case S_SUCCESS:
            sprintf(temp, "S_SUCCESS:  action completed successfully\n");
            break;

        case S_FAILURE:
            sprintf(temp, "S_FAILURE:  action failed\n");
            break;

        case S_EVFILE:
            sprintf(temp, "S_EVFILE:  evfile.msg event file I/O\n");
            break;

        case S_EVFILE_TRUNC:
            sprintf(temp, "S_EVFILE_TRUNC:  event truncated, insufficient buffer space\n");
            break;

        case S_EVFILE_BADBLOCK:
            sprintf(temp, "S_EVFILE_BADBLOCK:  bad block (header) number\n");
            break;

        case S_EVFILE_BADHANDLE:
            sprintf(temp, "S_EVFILE_BADHANDLE:  bad handle (closed?) or no memory to create new handle\n");
            break;

        case S_EVFILE_BADFILE:
            sprintf(temp, "S_EVFILE_BADFILE:  bad file format\n");
            break;

        case S_EVFILE_BADARG:
            sprintf(temp, "S_EVFILE_BADARG:  invalid function argument\n");
            break;

        case S_EVFILE_ALLOCFAIL:
            sprintf(temp, "S_EVFILE_ALLOCFAIL:  failed to allocate memory\n");
            break;

        case S_EVFILE_UNKOPTION:
            sprintf(temp, "S_EVFILE_UNKOPTION:  unknown option specified\n");
            break;
        
        case S_EVFILE_UNXPTDEOF:
            sprintf(temp, "S_EVFILE_UNXPTDEOF:  unexpected end-of-file or end-of-valid_data while reading\n");
            break;

        case S_EVFILE_BADSIZEREQ:
            sprintf(temp, "S_EVFILE_BADSIZEREQ:  invalid buffer size request to evIoct\n");
            break;

        case S_EVFILE_BADMODE:
            sprintf(temp, "S_EVFILE_BADMODE:  invalid operation for current evOpen() mode\n");
            break;

        default:
            sprintf(temp, "?evPerror...no such error: %d\n",error);
            break;
    }

    return(temp);
}




