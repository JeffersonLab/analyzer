#ifndef EVIO_H
#define EVIO_H

#include <stdio.h>
#include <iostream>

typedef struct evfilestruct {
  FILE *file;
  int *buf;
  int *next;
  int left;
  int blksiz;
  int blknum;
  int rw;
  int magic;
  int evnum;         /* last events with evnum so far */
  int byte_swapped;
} EVFILE;


extern int evOpen(const char* filename, const char* flags, int *handle);
extern int evRead(int handle, int *buffer, int buflen);
extern int evGetNewBuffer(EVFILE *a);
extern int evWrite(int handle,const int *buffer);
extern int evFlush(EVFILE *a);
extern int evIoctl(int handle,char *request,void *argp);
extern int evClose(int handle);
extern int evOpenSearch(int handle, int *b_handle);
extern int evSearch(int handle, int b_handle, int evn, int *buffer, int buflen, int *size);
extern int evCloseSearch(int b_handle);

// The following was originally defined in evio.cpp (seems like it
// should have been in the original evio.h, but was not).

#define  S_SUCCESS 0
#define  S_FAILURE -1

#define S_EVFILE    	0x00730000	/* evfile.msg Event File I/O */
#define S_EVFILE_TRUNC	0x40730001	/* Event truncated on read */
#define S_EVFILE_BADBLOCK	0x40730002	/* Bad block number encountered */
#define S_EVFILE_BADHANDLE	0x80730001	/* Bad handle (file/stream not open) */
#define S_EVFILE_ALLOCFAIL	0x80730002	/* Failed to allocate event I/O structure */
#define S_EVFILE_BADFILE	0x80730003	/* File format error */
#define S_EVFILE_UNKOPTION	0x80730004	/* Unknown option specified */
#define S_EVFILE_UNXPTDEOF	0x80730005	/* Unexpected end of file while reading event */
#define S_EVFILE_BADSIZEREQ	0x80730006	/* Invalid buffer size request to evIoct */

#define EVBLOCKSIZE 8192*4
#define EV_READ 0
#define EV_WRITE 1
#define EV_VERSION 1
#define EV_MAGIC 0xc0da0100
#define EV_HDSIZ 8

#define EV_HD_BLKSIZ 0		/* size of block in longwords */
#define EV_HD_BLKNUM 1		/* number, starting at 0 */
#define EV_HD_HDSIZ  2		/* size of header in longwords (=8) */
#define EV_HD_START  3		/* first start of event in this block */
#define EV_HD_USED   4		/* number of words used in block (<= BLKSIZ) */
#define EV_HD_VER    5		/* version of file format (=1) */
#define EV_HD_RESVD  6		/* (reserved) */
#define EV_HD_MAGIC  7		/* magic number for error detection */


#endif



