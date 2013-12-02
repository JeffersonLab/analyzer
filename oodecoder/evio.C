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
 * Revision History:
 *   $Log$
 *   Revision 1.10  2011/05/25 20:58:58  ole
 *   Bugfixes to evOpen:
 *   - Fix incorrect free() if leading whitespace was trimmed from file name.
 *   - Correctly handle file names with embedded whitespace
 *
 *   Revision 1.9  2009/01/06 20:12:19  ole
 *   Add support for large input files (>2GB).
 *
 *   Without additional modifications, the evio code now supports files up
 *   to blocksize * 2GB in size. The default CODA blocksize is 16k, but may
 *   be different depending on how the file was written. This should be
 *   tested further. Also, how does this mesh with newer CODA?
 *
 *   Revision 1.8  2005/10/24 15:00:31  feuerbac
 *   Modified evio routines to pass pointers (void*) instead of int's casted
 *   between pointers. The problem was noticed on 64-bit Linux AMD machines
 *   under gcc4, where int's are 4-bytes but the pointers are 8-bytes.
 *
 *   Revision 1.7  2004/05/09 21:43:28  ole
 *   Remove non-standard malloc.h include.
 *   Change C includes to C++ naming (stdlib.h -> cstdlib etc.)
 *
 *   Revision 1.6  2004/03/26 16:11:33  ole
 *   Make buffer argument for CODA write functions const since the buffer
 *   is never modified, just written out.
 *
 *   Revision 1.5  2002/09/13 18:59:36  rom
 *   fix bug (apparently recent) in malloc for fn in evOpen
 *
 *   Revision 1.4  2002/09/09 18:11:02  ole
 *   Trim whitespace from beginning and end of filename in evOpen routine.
 *
 *   Revision 1.3  2002/08/30 15:02:44  ole
 *   Remove leading spaces from file name in evOpen
 *
 *   Revision 1.2  2002/03/26 22:36:30  ole
 *   Fix compliation warnings about signed/unsigned comparisons.
 *
 *   Revision 1.1.1.1  2001/11/14 21:02:51  ole
 *   Initial import Release 0.61
 *
 *   Revision 1.1.1.1  2001/09/14 19:31:49  rom
 *   initial import of hana decoder (v 1.6)
 *
 *   Revision 1.3  1998/09/21 15:06:39  abbottd
 *   Changes for compile for vxWorks
 *
 *   Revision 1.2  1997/05/12 14:19:17  heyes
 *   remove evfile_msg.h
 *
 *   Revision 1.1.1.1  1996/09/19 18:25:20  chen
 *   original port to solaris
 *
*	  Revision 1.1  95/01/20  14:00:16  14:00:16  abbottd (David Abbott)
*	  Initial revision
*	  
 *	  Revision 1.5  1994/08/15  15:45:09  chen
 *	  add evnum to EVFILE structure. Keep event number when call evWrite
 *
 *	  Revision 1.4  1994/08/12  17:14:55  chen
 *	  check event type explicitly
 *
 *	  Revision 1.3  1994/06/17  16:11:12  chen
 *	  fix a bug when there is a single special event in the last block
 *
 *	  Revision 1.2  1994/05/12  15:38:37  chen
 *	  In case a wrong data format, close file and free memory
 *
 *	  Revision 1.1  1994/04/11  13:07:06  chen
 *	  Initial revision
 *
*	  Revision 1.6  1993/11/16  20:57:27  chen
*	  stronger type casting for swapped_memecpy
*
*	  Revision 1.5  1993/11/09  18:21:58  chen
*	  fix a bug
*
*	  Revision 1.4  1993/11/09  18:16:49  chen
*	  add binary search routines
*
*	  Revision 1.3  1993/11/03  16:40:55  chen
*	  cosmatic change
*
*	  Revision 1.2  1993/11/03  16:39:39  chen
*	  add bytpe_swapped flag to EVFILE struct
*
 *
 *
 *
 * Routines
 * --------
 *
 *	evOpen(const char *filename,const char *flags,void* *descriptor)
 *	evWrite(void* descriptor,int *data,int datalen)
 *	evRead(void* descriptor,int *data,int *datalen)
 *	evClose(void* descriptor)
 *	evIoctl(void* descriptor,char *request, void *argp)
 *
 * Modifications
 * -------------
 *  17-dec-91 cw started coding streams version with local buffers
 */

#ifdef VXWORKS
#include <vxWorks.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cctype>

#include "evio.h"

#define PMODE 0644

#ifndef EVFILE_header
#ifndef S_SUCCESS
#define S_SUCCESS 0
#define S_FAILURE -1
#endif

#define S_EVFILE    	0x00730000	/* evfile.msg Event File I/O */
#define S_EVFILE_TRUNC	0x40730001	/* Event truncated on read */
#define S_EVFILE_BADBLOCK	0x40730002	/* Bad block number encountered */
#define S_EVFILE_BADHANDLE	0x80730001	/* Bad handle (file/stream not open) */
#define S_EVFILE_ALLOCFAIL	0x80730002	/* Failed to allocate event I/O structure */
#define S_EVFILE_BADFILE	0x80730003	/* File format error */
#define S_EVFILE_UNKOPTION	0x80730004	/* Unknown option specified */
#define S_EVFILE_UNXPTDEOF	0x80730005	/* Unexpected end of file while reading event */
#define S_EVFILE_BADSIZEREQ	0x80730006	/* Invalid buffer size request to evIoct */
#endif


typedef struct evBinarySearch{
  int sbk;
  int ebk;
  int found_bk;
  int found_evn;
  int last_evn;
} EVBSEARCH;

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

#define evGetStructure() (EVFILE *)malloc(sizeof(EVFILE))

static  int  findLastEventWithinBlock(EVFILE *);
static  int  copySingleEvent(EVFILE *, int *, int, int);
static  int  evSearchWithinBlock(EVFILE *, EVBSEARCH *, int *, int, int *, int, int *);
static  void evFindEventBlockNum(EVFILE *, EVBSEARCH *, int *);
static  int  evGetEventNumber(EVFILE *, int);
static  int  evGetEventType(EVFILE *);
static  int  isRealEventsInsideBlock(EVFILE *, int, int);
static  int  physicsEventsInsideBlock(EVFILE *);

extern  int  int_swap_byte (int input);
extern  void onmemory_swap (char* buffer);
extern  int  swapped_fread (int *ptr,int size,int n_items,FILE *stream);
extern "C" void swapped_intcpy(char* des, char* source, int nbytes);
extern  void swapped_memcpy(char *buffer,char *source,int size);

#ifndef VXWORKS
int evopen_(const char *filename,const char *flags,void** handle,int fnlen,int flen)
{
  char *fn, *fl;
  int status;
  fn = (char *) malloc(fnlen+1);
  strncpy(fn,filename,fnlen);
  fn[fnlen] = 0;		/* insure filename is null terminated */
  fl = (char *) malloc(flen+1);
  strncpy(fl,flags,flen);
  fl[flen] = 0;			/* insure flags is null terminated */
  status = evOpen(fn,fl,handle);
  free(fn);
  free(fl);
  return(status);
}
#endif

int evOpen(const char *filename,const char *flags,void* *handle)
{
  // Open CODA file and check magic header bytes. When reading, determine if
  // the file is byte-swapped. "flags" must be "", "r" or "R" for reading,
  // or "w" or "W" for writing.
  EVFILE* a = evGetStructure(); /* allocate control structure or quit */
  if (!a) return(S_EVFILE_ALLOCFAIL);
  int header[EV_HDSIZ];
  int temp, blk_size = 0;
  char *fn = (char*)malloc(strlen(filename)+1), *fp = fn;
  strcpy(fn,filename);
  /* remove leading whitespace */
  while(isspace(*fp)) ++fp; 
  if (*fp!='\0') {
    /* remove trailing whitespace */
    char *cp = fp+strlen(fp)-1;
    while(isspace(*cp)) --cp;
    *(cp+1) = '\0';
  }
  switch (*flags)
  case '\0': case 'r': case 'R': {
    a->file = fopen(fp,"r"); free(fn);
    a->rw = EV_READ;
    if (a->file) {
      fread(header,sizeof(header),1,a->file); /* update: check nbytes return */
      if (header[EV_HD_MAGIC] != (int)EV_MAGIC) {
	temp = int_swap_byte(header[EV_HD_MAGIC]);
	if(temp == (int)EV_MAGIC)
	  a->byte_swapped = 1;
	else{ /* close file and free memory */
	  fclose(a->file);
	  free (a);
	  return(S_EVFILE_BADFILE); 
	}
      }
      else
	a->byte_swapped = 0;

      if(a->byte_swapped){
	blk_size = int_swap_byte(header[EV_HD_BLKSIZ]);
	a->buf = (int *)malloc(blk_size*4);
      }
      else {
	a->buf = (int *) malloc(header[EV_HD_BLKSIZ]*4);
      }

      if (!(a->buf)) {
	free(a);		/* if can't allocate buffer, give up */
	return(S_EVFILE_ALLOCFAIL);
      }
      if(a->byte_swapped){
	swapped_intcpy((char*)a->buf,(char *)header,EV_HDSIZ*4);
	fread(&(a->buf[EV_HDSIZ]),4,blk_size-EV_HDSIZ,a->file);
      } else {
	memcpy(a->buf,header,EV_HDSIZ*4);
	fread(a->buf+EV_HDSIZ,4,
	      header[EV_HD_BLKSIZ]-EV_HDSIZ,
	      a->file);		/* read rest of block */
      }
      a->next = a->buf + (a->buf)[EV_HD_START];
      a->left = (a->buf)[EV_HD_USED] - (a->buf)[EV_HD_START];
    }
    break;
  case 'w': case 'W':
    // FIXME: byte order?
    a->file = fopen(fp,"w"); free(fn);
    a->rw = EV_WRITE;
    if (a->file) {
      a->buf = (int *) malloc(EVBLOCKSIZE*4);
      if (!(a->buf)) {
	free(a);
	return(S_EVFILE_ALLOCFAIL);
      }
      a->buf[EV_HD_BLKSIZ] = EVBLOCKSIZE;
      a->buf[EV_HD_BLKNUM] = 0;
      a->buf[EV_HD_HDSIZ] = EV_HDSIZ;
      a->buf[EV_HD_START] = 0;
      a->buf[EV_HD_USED] = EV_HDSIZ;
      a->buf[EV_HD_VER] = EV_VERSION;
      a->buf[EV_HD_RESVD] = 0;
      a->buf[EV_HD_MAGIC] = EV_MAGIC;
      a->next = a->buf + EV_HDSIZ;
      a->left = EVBLOCKSIZE - EV_HDSIZ;
      a->evnum = 0;
    }
    break;
  default:
    free(a);
    free(fn);
    return(S_EVFILE_UNKOPTION);
  }
  if (a->file) {
    a->magic = EV_MAGIC;
    a->blksiz = a->buf[EV_HD_BLKSIZ];
    a->blknum = a->buf[EV_HD_BLKNUM];
    *handle = (void*) a;
    return(S_SUCCESS);
  } else {
    free(a);
    /*
    fprintf(stderr,"evOpen: Error opening file %s, flag %s\n",
	    filename,flags);
    perror(NULL);
    */
    *handle = 0;
    return(errno);
  }
}

#ifndef VXWORKS
int evread_(void* *handle,int *buffer,int *buflen)
{
  return(evRead(*handle,buffer,*buflen));
}
#endif

int evRead(void* handle,int *buffer,int buflen)
{
  EVFILE *a;
  int nleft,ncopy,error,status;
  int *temp_buffer = (int *) NULL;
  int *temp_ptr = (int *) NULL;

  a = (EVFILE *)handle;
  if (a->magic != (int)EV_MAGIC) return(S_EVFILE_BADHANDLE);
  if (a->left<=0) {
    error = evGetNewBuffer(a);
    if (error) return(error);
  }
  if (a->byte_swapped) {
    temp_buffer = (int *)malloc(buflen*sizeof(int));
    temp_ptr = temp_buffer;
    nleft = int_swap_byte(*(a->next)) + 1;
  } else
    nleft = *(a->next) + 1;	/* inclusive size */
  if (nleft < buflen) {
    status = S_SUCCESS;
  } else {
    status = S_EVFILE_TRUNC;
    nleft = buflen;
  }
  while (nleft>0) {
    if (a->left<=0) {
      error = evGetNewBuffer(a);
      if (error) {
	free(temp_ptr);
	return(error);
      }
    }
    ncopy = (nleft <= a->left) ? nleft : a->left;
    if (a->byte_swapped){
      memcpy(temp_buffer,a->next,ncopy*4);
      temp_buffer += ncopy;
    }
    else{
      memcpy(buffer,a->next,ncopy*4);
      buffer += ncopy;
    }
    nleft -= ncopy;
    a->next += ncopy;
    a->left -= ncopy;
  }
  if (a->byte_swapped){
    swapped_memcpy((char *)buffer,(char *)temp_ptr,buflen*sizeof(int));  
    free(temp_ptr);
  }
  return(status);
}

int evGetNewBuffer(EVFILE *a) {
  int i,nread,status;
  status = S_SUCCESS;
  if (feof(a->file)) return(EOF);
  clearerr(a->file);
  a->buf[EV_HD_MAGIC] = 0;
  nread = fread(a->buf,4,a->blksiz,a->file);
  if (a->byte_swapped){
    for(i=0;i<EV_HDSIZ;i++)
      onmemory_swap((char*)&(a->buf[i]));
  }
  if (feof(a->file)) return(EOF);
  if (ferror(a->file)) return(ferror(a->file));
  if (nread != a->blksiz) return(errno);
  if (a->buf[EV_HD_MAGIC] != (int)EV_MAGIC) {
    /* fprintf(stderr,"evRead: bad header\n"); */
    return(S_EVFILE_BADFILE);
  }
  a->blknum++;
  if (a->buf[EV_HD_BLKNUM] != a->blknum) {
    /* fprintf(stderr,"evRead: bad block number %x should be %x\n",
	    a->buf[EV_HD_BLKNUM],a->blknum); */
    status = S_EVFILE_BADBLOCK;
  }
  a->next = a->buf + (a->buf)[EV_HD_HDSIZ];
  a->left = (a->buf)[EV_HD_USED] - (a->buf)[EV_HD_HDSIZ];
  if (a->left<=0)
    return(S_EVFILE_UNXPTDEOF);
  else
    return(status);
}

#ifndef VXWORKS
int evwrite_(void* *handle,const int *buffer)
{
  return(evWrite(*handle,buffer));
}
#endif

int evWrite(void* handle,const int *buffer)
{
  EVFILE *a;
  int nleft,ncopy,error;
  a = (EVFILE *)handle;
  if (a->magic != (int)EV_MAGIC) {
    return(S_EVFILE_BADHANDLE);
  }

  if (a->buf[EV_HD_START]==0) {
    a->buf[EV_HD_START] = a->next - a->buf;
  }
  a->evnum = a->evnum + 1;
  nleft = buffer[0] + 1;	/* inclusive length */

  while (nleft>0) {
    ncopy = (nleft <= a->left) ? nleft : a->left;
    memcpy(a->next,buffer,ncopy*4);
    buffer += ncopy;
    nleft -= ncopy;
    a->next += ncopy;
    a->left -= ncopy;
    if (a->left<=0) {
      error = evFlush(a);
      if (error) return(error);
    }
  }
  return(S_SUCCESS);
}

int evFlush(EVFILE *a)
{
  int nwrite;
  clearerr(a->file);
  a->buf[EV_HD_USED] = a->next - a->buf;
  a->buf[EV_HD_RESVD] = a->evnum;
  nwrite = fwrite(a->buf,4,a->blksiz,a->file);
  if (ferror(a->file)) return(ferror(a->file));
  if (nwrite != a->blksiz) return(errno);
  a->blknum++;
  a->buf[EV_HD_BLKSIZ] = a->blksiz;
  a->buf[EV_HD_BLKNUM] = a->blknum;
  a->buf[EV_HD_HDSIZ] = EV_HDSIZ;
  a->buf[EV_HD_START] = 0;
  a->buf[EV_HD_USED] = EV_HDSIZ;
  a->buf[EV_HD_VER] = EV_VERSION;
  a->buf[EV_HD_RESVD] = 0;
  a->buf[EV_HD_MAGIC] = EV_MAGIC;
  a->next = a->buf + EV_HDSIZ;
  a->left = a->blksiz - EV_HDSIZ;
  return(S_SUCCESS);
}

#ifndef VXWORKS
int evioctl_(void* *handle,char *request,void *argp,int reqlen)
{
  char *req;
  int status;
  req = (char *)malloc(reqlen+1);
  strncpy(req,request,reqlen);
  req[reqlen]=0;		/* insure request is null terminated */
  status = evIoctl(*handle,req,argp);
  free(req);
  return(status);
}
#endif

int evIoctl(void* handle,char *request,void *argp)
{
  EVFILE *a;
  a = (EVFILE *)handle;
  if (a->magic != (int)EV_MAGIC) return(S_EVFILE_BADHANDLE);
  switch (*request) {
  case 'b': case 'B':
    if (a->rw != EV_WRITE) return(S_EVFILE_BADSIZEREQ);
    if (a->blknum != 0) return(S_EVFILE_BADSIZEREQ);
    if (a->buf[EV_HD_START] != 0) return(S_EVFILE_BADSIZEREQ);
    free (a->buf);
    a->blksiz = *(int *) argp;
    a->left = a->blksiz - EV_HDSIZ;
    a->buf = (int *) malloc(a->blksiz*4);
    if (!(a->buf)) {
      a->magic = 0;
      free(a);		/* if can't allocate buffer, give up */
      return(S_EVFILE_ALLOCFAIL);
    }
    a->buf[EV_HD_BLKSIZ] = EVBLOCKSIZE;
    a->buf[EV_HD_BLKNUM] = 0;
    a->buf[EV_HD_HDSIZ] = EV_HDSIZ;
    a->buf[EV_HD_START] = 0;
    a->buf[EV_HD_USED] = EV_HDSIZ;
    a->buf[EV_HD_VER] = EV_VERSION;
    a->buf[EV_HD_RESVD] = 0;
    a->buf[EV_HD_MAGIC] = EV_MAGIC;
    break;
  default:
    return(S_EVFILE_UNKOPTION);
  }
  return(S_SUCCESS);
}

#ifndef VXWORKS
int evclose_(long int *handle)
{
  return(evClose((void*)*handle));
}
#endif

int evClose(void* handle)
{
  EVFILE *a;
  int status = 0, status2;
  a = (EVFILE *)handle;
  if (a->magic != (int)EV_MAGIC) return(S_EVFILE_BADHANDLE);
  if(a->rw == EV_WRITE) {
    status = evFlush(a);
  }
  status2 = fclose(a->file);
  free(a->buf);
  free(a);
  if (status==0) status = status2;
  return(status);
}


/******************************************************************
 *         int evOpenSearch(void*, void* *)                       *
 * Description:                                                   *
 *     Open for binary search on data blocks                      *
 *     return last physics event number                           *
 *****************************************************************/
int evOpenSearch(void* handle, void* *b_handle)
{
  EVFILE *a;
  EVBSEARCH *b;
  int    found = 0, temp, i = 1;
  int    last_evn,  bknum;
  int    header[EV_HDSIZ];
  
  a = (EVFILE *)handle;
  b = (EVBSEARCH *)malloc(sizeof(EVBSEARCH));
  if(b == NULL){
    fprintf(stderr,"Cannot allocate memory for EVBSEARCH structure!\n");
    exit(1);
  }
  fseeko(a->file, 0L, SEEK_SET);
  fread(header, sizeof(header), 1, a->file);
  if(a->byte_swapped)
    temp = int_swap_byte(header[EV_HD_BLKNUM]);
  else
    temp = header[EV_HD_BLKNUM];
  b->sbk = temp;
  /* jump to the end of file */
  fseeko(a->file, 0L, SEEK_END);
  while(!found){
    /* jump back to the beginning of the block */
    fseeko(a->file, (-1)*a->blksiz*4*i, SEEK_END);
    if((bknum = physicsEventsInsideBlock(a)) >= 0){
      b->ebk = bknum;
      break;
    }
    else
      i++;
  }
  /* the file pointer will point to the first physics event in the block */
  last_evn = findLastEventWithinBlock(a);
  b->found_bk = -1;
  b->found_evn = -1;
  b->last_evn = last_evn;
  *b_handle = (void*)b;
  return last_evn;
}
  
/*********************************************************************
 *       static int findLastEventWithinBlock(EVFILE *)               *
 * Description:                                                      *
 *     Doing sequential search on a block pointed by a               *
 *     return last event number in the block                         *
 *     the pointer to the file has been moved to the beginning       *
 *     of the fisrt event already by evOpenSearch()                  *
 ********************************************************************/
static int findLastEventWithinBlock(EVFILE *a)
{
  int header, found = 0;
  int ev_size, evn = 0, last_evn = 0;
  int ev_type;
  int first_time = 0;
  
  while(!found){
    fread(&header, sizeof(int), 1, a->file);
    if(a->byte_swapped)
      ev_size = int_swap_byte(header) + 1;
    else
      ev_size = header + 1;
    /* read event type */
    ev_type = evGetEventType(a);
    a->left = a->left - ev_size;  /* file pointer stays */
    if(a->left <= 0){ /* no need to distinguish the special event */
      if(ev_type < 16){ /* physics event */
	first_time++;
	fseeko(a->file, 3*4, SEEK_CUR);
	fread(&header, sizeof(int), 1, a->file);
	if(a->byte_swapped)
	  evn = int_swap_byte(header);
	else
	  evn = header;
	found = 1;
      }
      else{
	if (first_time == 0){
	  evn = -1;
	  found = 1;
	}
	else{
	  evn = last_evn;
	  found = 1;
	}
      }
    }
    else{
      if(ev_type < 16){
	first_time++;
	fseeko(a->file, 3*4, SEEK_CUR);
	fread(&header, sizeof(int), 1, a->file);
	if(a->byte_swapped)
	  evn = int_swap_byte(header);
	else
	  evn = header;
	last_evn = evn;
	fseeko(a->file,(ev_size - 5)*4, SEEK_CUR);
      }
      else{
	fseeko(a->file, (ev_size - 1)*4, SEEK_CUR);
      }
    }
  }
  return evn;
}

/********************************************************************
 *      int evSearch(void*, void*, int, int *, int, int *)          *
 * Description:                                                     *
 *    Doing binary search for event number evn, -1 failure          *
 *    Copy event to buffer with buffer length buflen                *
 *    This routine must be called after evOpenSearch()              *
 *    return 0: found the event                                     *
 *    return -1: the event number bigger than largest ev number     *
 *    return 1:  cannot find the event number                       *
 *******************************************************************/
int evSearch(void* handle, void* b_handle, int evn, int *buffer, int buflen, int *size)
{
  EVFILE    *a;
  EVBSEARCH *b;
  int       start,end, mid;
  int       found;

  a = (EVFILE *)handle;
  b = (EVBSEARCH *)b_handle;

  if(evn > b->last_evn)
    return -1;

  if(b->found_bk < 0){
    start = b->sbk;
    end   = b->ebk;
    mid   = (start + end)/2;
  }
  else{
    if(evn >= b->found_evn){
      start = b->found_bk;
      end   = b->ebk;
      mid   = (start + end)/2;
    }
    else{
      start = b->sbk;
      end   = b->found_bk;
      mid   = (start + end)/2;
    }
  }
  while(start <= end){
    found = evSearchWithinBlock(a, b, &mid, evn, buffer, buflen, size);
    if(found < 0){ /* lower block */
      end = mid - 1;
      mid = (start + end)/2;
    }
    else if(found > 0){ /* upper block */
      start = mid + 1;
      mid = (start + end)/2;
    }
    else if(found == 0){ /*found block and evn */
      break;
    }
    else
      return found;
  }
  if(start <= end){
    b->found_bk = mid;
    b->found_evn = evn;
    return 0;
  }
  else{
    b->found_bk = -1;
    return 1;
  }
}


/****************************************************************************
 *   static int evSearchWithinBlock(EVFILE *, EVBSEARCH *, int *,int, int * *
 *                                  int, int *                )             *
 * Description:                                                             *
 *    Doing sequential search on a particular block to find out event       *
 *    number evn                                                            *
 *    return 0: found                                                       *
 *    return -1: evn < all events in the block                              *
 *    return 1:  evn > all events in the block                              *
 ***************************************************************************/
static int evSearchWithinBlock(EVFILE *a, EVBSEARCH *b, int *bknum, 
			       int evn, int *buffer, int buflen, int *size)
{
  int temp, ev_size;
  int status;
  int found = 0, t_evn;
  int ev_type;

  evFindEventBlockNum(a, b, bknum);

  /* check first event, if its event number is greater than
   * requested event number, return -1 
   * the pointer pointing to file has been moved in the previous
   * subroutines
   */
  fread(&temp,sizeof(int),1,a->file);
  if(a->byte_swapped)
    ev_size = int_swap_byte(temp) + 1;
  else
    ev_size = temp + 1;

  a->left = a->left - ev_size;
  t_evn = evGetEventNumber(a, ev_size); /* file pointer stays here */

  if(t_evn == evn){
    fseeko(a->file, (-1)*4, SEEK_CUR);   /* go to top of event */
    *size = ev_size;
    status = copySingleEvent(a, buffer, buflen, ev_size);
    return status;
  }
  else if (t_evn > evn) {
    /* no need to search any more */
    return -1;
  } else {
    /* need to search more        */
    if(a->left <=0) {
      /* no more events left */
      return 1;
    } else {
      fseeko(a->file, (ev_size-1)*4, SEEK_CUR);
      while (!found && a->left > 0) {
	fread(&temp, sizeof(int), 1, a->file);
	if (a->byte_swapped) {
	  ev_size = int_swap_byte(temp) + 1;
	} else {
	  ev_size = temp + 1;
	}
	/* read event type */
	ev_type = evGetEventType(a); /* file pointer fixed here */

	a->left = a->left - ev_size;
	if(a->left <= 0){ /* this is the last event */
	  if(ev_type < 16){ /* physics event here */
	    t_evn = evGetEventNumber(a, ev_size);  /* pinter stays */
	    /* check current event number */ 
	    if(t_evn == evn){
	      fseeko(a->file, (-1)*4, SEEK_CUR);
	      found = 1;
	      *size = ev_size;
	      return (copySingleEvent(a, buffer, buflen, ev_size));
	    }
	    else
	      return 1;
	  }
	  else /* last event is not a physics event, no match */
	    return 1;
	}
	else{ /* not last event */
	  if(ev_type < 16){
	    t_evn = evGetEventNumber(a, ev_size);
	    /* check current event number */
	    if(t_evn == evn){
	      fseeko(a->file, (-1)*4, SEEK_CUR);
	      *size = ev_size;
	      found = 1;
	      return (copySingleEvent(a, buffer, buflen, ev_size));
	    }
	    else{ /* go to next event */
	      fseeko(a->file, (ev_size-1)*4, SEEK_CUR);
	    }
	  }
	  else /* special event go to next event */
	    fseeko(a->file, (ev_size - 1)*4, SEEK_CUR);
	}      /* end of not last event case*/
      }        /* end of search event loop*/
    }          
  }
  return (0);
}


/********************************************************************
 *   static void evFindEventBlockNum(EVFILE *, EVBSEARCH *, int *)  *
 * Description:                                                     *
 *    find out real block number in the case of this block          *
 *    has one big event just crossing it                            *
 *******************************************************************/
static void evFindEventBlockNum(EVFILE *a, EVBSEARCH *b, int *bknum)
{
  int header[EV_HDSIZ], block_num;
  int buf[EV_HDSIZ];
  int nleft;

  block_num = *bknum;
  while(block_num <= b->ebk){
    fseeko(a->file, a->blksiz*block_num*4, SEEK_SET);
    fread(header, sizeof(header), 1, a->file);
    if(a->byte_swapped)
      swapped_intcpy((char*)buf, (char *)header, EV_HDSIZ*4);
    else
      memcpy(buf, header, EV_HDSIZ*4);
    if(buf[EV_HD_START] > 0){
      fseeko(a->file, 4*(buf[EV_HD_START]-EV_HDSIZ), SEEK_CUR);
      nleft = buf[EV_HD_USED] - buf[EV_HD_START];
      if(isRealEventsInsideBlock(a,block_num,nleft)){
	*bknum = block_num;
	return;
      }
      block_num++;
    }
    else
      block_num++;
  }
  /* cannot find right block this way, try reverse direction */
  block_num = *bknum;
  while(block_num >= b->sbk){
    fseeko(a->file, a->blksiz*block_num*4, SEEK_SET);
    fread(header, sizeof(header), 1, a->file);
    if(a->byte_swapped)
      swapped_intcpy((char*)buf,(char *)header, EV_HDSIZ*4);
    else
      memcpy((char *)buf, (char *)header, EV_HDSIZ*4);
    if(buf[EV_HD_START] > 0){
      fseeko(a->file, 4*(buf[EV_HD_START]-EV_HDSIZ), SEEK_CUR);
      nleft = buf[EV_HD_USED] - buf[EV_HD_START];
      if(isRealEventsInsideBlock(a,block_num, nleft)){
	*bknum = block_num;
	return;
      }
      block_num--;
    }
    else
      block_num--;
  }
  fprintf(stderr,"Cannot find out event offset in any of the blocks, Exit!\n");
  exit(1);
}

/*************************************************************************
 *   static int isRealEventInsideBlock(EVFILE *, int, int)               *
 * Description:                                                          *
 *     Find out whether there is a real event inside this block          *
 *     return 1: yes, return 0: no                                       *
 ************************************************************************/
static int isRealEventsInsideBlock(EVFILE *a, int bknum, int old_left)
{
  int nleft = old_left;
  int ev_size, temp, ev_type;

  while(nleft > 0){
    fread(&temp, sizeof(int), 1, a->file);
    if(a->byte_swapped)
      ev_size = int_swap_byte(temp) + 1;
    else
      ev_size = temp + 1;

    ev_type = evGetEventType(a);  /* file pointer stays */
    if(ev_type < 16){
      fseeko(a->file, (-1)*sizeof(int), SEEK_CUR); /* rewind to head of this event */
      break;
    }
    else{
      nleft = nleft - ev_size;
      fseeko(a->file, 4*(ev_size - 1), SEEK_CUR);
    }
  }
  if(nleft <= 0)
    return 0;
  else{
    a->left = nleft;
    return 1;
  }
}

/*****************************************************************************
 *    static int copySingleEvent(EVFILE *, int *, int, int)                  *
 * Description:                                                              *
 *    copy a single event to buffer by using fread.                          *
 *    starting point is given by EVFILE *a                                   *
 ****************************************************************************/      
static int copySingleEvent(EVFILE *a, int *buffer, int buflen, int ev_size)
{
  int *temp_buffer = NULL, *temp_ptr = NULL, *ptr = NULL;
  int status, nleft, block_left;
  int ncopy;


  if(a->byte_swapped){
    temp_buffer = (int *)malloc(buflen*sizeof(int));
    temp_ptr = temp_buffer;
  }
  else{
    ptr = buffer;
  }

  if(buflen < ev_size){
    status = S_EVFILE_TRUNC;
    nleft  = buflen;
  }
  else{
    status = S_SUCCESS;
    nleft  = ev_size;
  }
  if(a->left < 0){
    block_left = ev_size + a->left;
    if(nleft <= block_left){
      if(a->byte_swapped){
	fread((char *)temp_ptr, nleft*4, 1, a->file);
      }
      else
	fread((char *)ptr, nleft*4, 1, a->file);
    }
    else{
      ncopy = block_left;
      while(nleft > 0){
	if(a->byte_swapped){
	  fread((char *)temp_ptr, ncopy*4, 1, a->file);
	  temp_ptr = temp_ptr + ncopy;
	}	
	else{
	  fread((char *)ptr, ncopy*4, 1, a->file);      
	  ptr = ptr + ncopy;
	}
	nleft = nleft - ncopy;
	if(nleft > a->blksiz - EV_HDSIZ){
	  fseeko(a->file, EV_HDSIZ*4, SEEK_CUR);
	  ncopy = a->blksiz - EV_HDSIZ;
	}
	else if(nleft > 0){
	  fseeko(a->file, EV_HDSIZ*4,SEEK_CUR);
	  ncopy = nleft;
	}
      }
      if(a->byte_swapped)
	temp_ptr = temp_buffer;
      else
	ptr = buffer;
    }
  }
  else{
    if(a->byte_swapped){
      fread(temp_ptr, ev_size*4, 1, a->file);
    }
    else{
      fread(ptr, ev_size*4, 1, a->file);
    }
  }
  
  if(a->byte_swapped){
    swapped_memcpy((char *)buffer, (char *)temp_ptr, buflen*sizeof(int));
    free(temp_ptr);
  }
  return (status);
}

/***********************************************************************
 *   int evCloseSearch(int )                                           *
 * Description:                                                        *
 *     Close evSearch process, release memory                          *
 * bugbug - RWM: why return an int? nothing can fail.                  *
 **********************************************************************/
int evCloseSearch(void* b_handle)
{
  EVBSEARCH *b;

  b = (EVBSEARCH *)b_handle;
  free(b);
  return(0);
}

/**********************************************************************
 *     static int evGeteventNumber(EVFILE *, int)                     *
 * Description:                                                       *
 *     get event number starting from event head.                     *
 *********************************************************************/
static int evGetEventNumber(EVFILE *a, int ev_size)
{
  int temp, evn, nleft;

  nleft = a->left + ev_size;
  if(nleft >= 5)
    fseeko(a->file, 3*4, SEEK_CUR);
  else
    fseeko(a->file, (EV_HDSIZ+3)*4, SEEK_CUR);
  fread(&temp, sizeof(int), 1, a->file);
  if(a->byte_swapped)
    evn = int_swap_byte(temp);
  else
    evn = temp;

  if(nleft >= 5)
    fseeko(a->file, (-1)*4*4, SEEK_CUR);
  else
    fseeko(a->file,(-1)*(EV_HDSIZ + 4)*4, SEEK_CUR);

  return evn;
}

static int evGetEventType(EVFILE *a)
{
  int ev_type, temp, t_temp;
  
  if(a->left == 1) /* event type long word is in the following block */
    fseeko(a->file, (EV_HDSIZ)*4,SEEK_CUR);
  if(a->byte_swapped){
    fread(&t_temp, sizeof(int), 1, a->file);
    swapped_intcpy((char*)&temp, (char *)&t_temp, sizeof(int));
  }
  else
    fread(&temp, sizeof(int), 1, a->file);
  ev_type = (temp >> 16)&(0x0000ffff);

  if(a->left == 1)
    fseeko(a->file, (-1)*(EV_HDSIZ + 1)*4, SEEK_CUR);
  else
    fseeko(a->file, (-1)*4, SEEK_CUR);

  return ev_type;
}


/*************************************************************************
 *   static int physicsEventsInsideBlock(a)                              *
 * Description:                                                          *
 *     Check out whether this block pointed by a contains any physics    *
 *     events                                                            *
 *     return -1: contains no physics                                    *
 *     return >= 0 : yes, contains physics event, with block number      *
 *     the file pointer will stays at the begining of the first physics  *
 *     event    inside the block                                         *
 ************************************************************************/
static int physicsEventsInsideBlock(EVFILE *a)
{
  int header[EV_HDSIZ], buf[EV_HDSIZ];
  int nleft, temp, ev_size, ev_type;
  
  /* copy block header information */
  if(a->byte_swapped){
    fread(header, sizeof(header), 1, a->file);
    swapped_intcpy((char*)buf, (char *)header, EV_HDSIZ*4);
  }
  else
    fread(buf, sizeof(buf), 1, a->file);
  /* search first event inside this block */
  if (buf[EV_HD_START] < 0)
    return 0;
  else{
    /* jump to the first event */
    fseeko(a->file, 4*(buf[EV_HD_START] - EV_HDSIZ), SEEK_CUR);
    nleft = buf[EV_HD_USED] - buf[EV_HD_START];
    while (nleft > 0){
      fread(&temp, sizeof(int), 1, a->file);
      if(a->byte_swapped)
	ev_size = int_swap_byte(temp) + 1;
      else
	ev_size = temp + 1;
      /* check event type and file pointer stays */
      ev_type = evGetEventType(a);
      if(ev_type < 16) { /*physics event */
	fseeko(a->file, (-1)*sizeof(int), SEEK_CUR);
	a->left = nleft;
	return buf[EV_HD_BLKNUM];
      }
      else{
	nleft = nleft - ev_size;
	fseeko(a->file, 4*(ev_size - 1), SEEK_CUR);
      }
    }
  }
  return 0;
}


