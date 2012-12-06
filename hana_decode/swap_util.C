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
 *	Byte swapping utilities
 *	
 * Author:  Jie Chen, CEBAF Data Acquisition Group
 *
 * Revision History:
 *   $Log$
 *   Revision 1.5  2004/06/12 20:04:41  ole
 *   Add forgotten "using namespace std". This is C++ code now.
 *
 *   Revision 1.4  2003/09/10 18:36:05  ole
 *   Fix compilation warnings.
 *
 *   Revision 1.3  2003/06/16 17:07:25  ole
 *   Move Release-070 branch onto the CVS trunk.
 *
 *   Revision 1.2.2.1  2003/04/27 04:33:19  ole
 *   Fix warnings about uninitialized variables.
 *
 *   Revision 1.2  2002/03/26 22:36:31  ole
 *   Fix compliation warnings about signed/unsigned comparisons.
 *
 *   Revision 1.1.1.1  2001/11/14 21:02:51  ole
 *   Initial import Release 0.61
 *
 *   Revision 1.1.1.1  2001/09/14 19:31:49  rom
 *   initial import of hana decoder (v 1.6)
 *
*	  Revision 1.1  95/01/20  14:00:33  14:00:33  abbottd (David Abbott)
*	  Initial revision
*	  
 *	  Revision 1.4  1994/08/12  17:15:18  chen
 *	  handle char string data type correctly
 *
 *	  Revision 1.3  1994/05/17  14:24:19  chen
 *	  fix memory leaks
 *
 *	  Revision 1.2  1994/04/12  18:02:20  chen
 *	  fix a bug when there is no event wrapper
 *
 *	  Revision 1.1  1994/04/11  13:09:18  chen
 *	  Initial revision
 *
*	  Revision 1.2  1993/11/05  16:54:50  chen
*	  change comment
*
*	  Revision 1.1  1993/10/27  09:39:44  heyes
*	  Initial revision
*
 *	  Revision 1.1  93/08/30  19:13:49  19:13:49  chen (Jie chen)
 *	  Initial revision
 *	  
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory.h>
#include <cerrno>

using namespace std;

typedef struct _stack
{
  int length;      /* inclusive size */
  int posi;        /* event start position */
  int type;        /* data type */
  int tag;         /* tag value */
  int num;         /* num field */
  struct _stack *next;
}evStack;

typedef struct _lk
{
  int head_pos;
  int type;
}LK_AHEAD;         /* find out header */

static evStack *init_evStack(),*evStack_top(evStack *);
static void    evStack_popoff(evStack *);
static void    evStack_pushon(int,int,int,int,int,evStack *);
static void    evStack_free(evStack *);
extern "C" void swapped_intcpy( char*, char*, int );

/*********************************************************
 *             int int_swap_byte(int input)              *
 * get integer 32 bit input and output swapped byte      *
 * integer                                               *
 ********************************************************/
int int_swap_byte(int input) 
{
  int temp,i,len;
  char *buf,*temp_buf;

  len = sizeof(int);
  buf = (char *)malloc(len);
  temp_buf = (char *)malloc(len);
  memcpy(temp_buf,&input,len);
  for(i=0;i<len;i++)
    buf[i] = temp_buf[len-i-1];
  temp = *(int *)(buf);
  free(buf);free(temp_buf);
  return temp;
}

/********************************************************
 * void onmemory_swap(char *buffer)                     *
 * swap byte order of buffer, buffer will be changed    *
 ********************************************************/
void onmemory_swap(char *buffer)
{
  char temp[4],des_temp[4];
  int  i,int_len;

  int_len = sizeof(int);
  memcpy(temp,buffer,int_len);
  for(i=0;i<int_len;i++)
    des_temp[i] = temp[int_len-i-1];
  memcpy(buffer,des_temp,int_len);
}

/********************************************************
 * void swapped_intcpy(void *des,void *source, int size)*
 * copy source with size size to des, but with byte     *
 * order swapped in the unit of byte                    * 
 *
 * NOTE:
 * New, more efficient version from Ole Hansen, July 2000
 * and compiled by gcc as a separate module.
 *******************************************************/





/*******************************************************
 * void swapped_shortcpy(char *des, char *source, size)*
 * copy short integer or packet with swapped byte order*
 * ****************************************************/
void swapped_shortcpy(char *des,char *source,int size)
{
  char temp[2],des_temp[2];
  int  i, j, short_len;
  
  short_len = sizeof(short);
  i = 0;
  while(i < size){
    memcpy(temp,&source[i],short_len);
    for(j=0; j<short_len;j++)
      des_temp[j] = temp[short_len -j -1];
    memcpy(&(des[i]),des_temp,short_len);
    i += 2;
  }
}

/*******************************************************
 * void swapped_longcpy(char *des, char *source, size) *
 * copy 64 bit with swapped byte order                 *
 * ****************************************************/
void swapped_longcpy(char *des,char *source,int size)
{
  char temp[8],des_temp[8];
  int  i, j, long_len;
  
  long_len = 8;
  i = 0;
  while(i < size){
    memcpy(temp,&source[i],long_len);
    for(j=0; j< long_len;j++)
      des_temp[j] = temp[long_len -j -1];
    memcpy(&(des[i]),des_temp,long_len);
    i += 8;
  }
}

/*************************************************************
 *   int swapped_fread(void *ptr, int size, int n_itmes,file)*
 *   fread from a file stream, but return swapped result     *
 ************************************************************/ 
int swapped_fread(void *ptr,int size,int n_items,FILE *stream)
{
  char *temp_ptr;
  int  nbytes;

  temp_ptr = (char *)malloc(size*n_items);
  nbytes = fread(temp_ptr,size,n_items,stream);
  if(nbytes > 0){
    swapped_intcpy((char*)ptr,temp_ptr,n_items*size);
  }
  free(temp_ptr);
  return(nbytes);
}
	   
/***********************************************************
 *    void swapped_memcpy(char *buffer,char *source,size)  *
 * swapped memory copy from source to buffer accroding     *
 * to data type                                            *
 **********************************************************/
void swapped_memcpy(char *buffer,char *source,int size)
{
  evStack  *head, *p;
  LK_AHEAD lk;
  int      int_len, short_len, long_len;
  int      i, j, current_type = 0;
  int      header1, header2;
  int      ev_size, ev_tag, ev_num, ev_type;
  int      bk_size, bk_tag, bk_num, bk_type;
  int      sg_size, sg_tag, sg_type;
  // int      depth;
  short    pk_size, pack;
  // short    pk_tag;
  char     temp[4],temp2[2];

  int_len = sizeof(int);
  short_len = sizeof(short);
  long_len = 8;
  head = init_evStack();
  i = 0;   /* index pointing to 16 bit word */
  swapped_intcpy(temp,source,int_len);
  ev_size = *(int *)(temp);                  /*ev_size in unit of 32 bit*/
  memcpy(&(buffer[i*2]),temp,int_len);
  i += 2;
  swapped_intcpy(temp,&(source[i*2]),int_len);
  header2 = *(int *)(temp);
  ev_tag =(header2 >> 16) & (0x0000ffff);
  ev_type=(header2 >> 8) & (0x000000ff);
  ev_num = (header2) & (0x000000ff);
  memcpy(&(buffer[i*2]),temp,int_len);
  i += 2;

  if(ev_type >= 0x10){/* data type must be 0x10 bank type */
    evStack_pushon((ev_size+1)*2,i-4,ev_type,ev_tag,ev_num,head);
    lk.head_pos = i;
    lk.type = ev_type;
    if(lk.type == 0x10)
      ev_size = ev_size + 1;
  }
  else{              /* sometimes event has no wrapper */
    lk.head_pos = i + 2*(ev_size - 1);
    lk.type = ev_type;
    current_type = ev_type;
  }

/* get into the loop */
  while (i < ev_size*2){
    if ((p = evStack_top(head)) != NULL){
      while(((p = evStack_top(head)) != NULL) && i == (p->length + p->posi)){
	evStack_popoff(head);
	head->length -= 1;
      }
    }
    if (i == lk.head_pos){      /* dealing with header */
      if((p = evStack_top(head)) != NULL)
	lk.type = (p->type);
      switch(lk.type){
      case 0x10:
	swapped_intcpy(temp,&(source[i*2]),int_len);
	header1 = *(int *)(temp);
	bk_size = header1;
	memcpy(&(buffer[i*2]),temp,int_len);
	i = i + 2;
	swapped_intcpy(temp,&(source[i*2]),int_len);
	header2 = *(int *)(temp);
	memcpy(&(buffer[i*2]),temp,int_len);
	bk_tag = (header2 >> 16) & (0x0000ffff);
	bk_type = (header2 >> 8) & (0x000000ff);
	bk_num = (header2) & (0x000000ff);
	// depth = head->length;  /* tree depth */
	if (bk_type >= 0x10){  /* contains children */
	  evStack_pushon((bk_size+1)*2,i-2,bk_type,bk_tag,bk_num,head);
	  lk.head_pos = i + 2;
	  head->length += 1;
	  i = i + 2;
	}
	else{  /* real data */
	  current_type = bk_type;
	  lk.head_pos = i + bk_size*2;
	  i = i+ 2;
	}
	break;
      case 0x20:
	swapped_intcpy(temp,&(source[i*2]),int_len);
	header2 = *(int *)(temp);
	memcpy(&(buffer[i*2]),temp,int_len);
	sg_size = (header2) & (0x0000ffff);
	sg_size = sg_size + 1;
	sg_tag  = (header2 >> 24) & (0x000000ff);
	sg_type = (header2 >> 16) & (0x000000ff);
	if(sg_type >= 0x20){  /* contains children */
	  evStack_pushon((sg_size)*2,i,sg_type,sg_tag,0,head);
	  lk.head_pos = i + 2;
	  head->length += 1;
	  i = i+ 2;
	}
	else{  /* real data */
	  current_type = sg_type;
	  lk.head_pos = i + sg_size*2;
	  i = i + 2;
	}
	break;
      default:  /* packet type */
	swapped_shortcpy(temp2,&(source[i*2]),short_len);
	pack = *(short *)(temp2);
	memcpy(&(buffer[i*2]),temp2,short_len);
	if(pack == 0x0000){  /* empty packet increase by 1 */
	  lk.head_pos = i + 1;
	  i++;
	}
	else{
	  // pk_tag = (pack >> 8) & (0x00ff);
	  pk_size = (pack) & (0x00ff);
	  current_type = lk.type;
	  lk.head_pos = i + pk_size + 1;
	  i = i + 1;
	}
	break;
      }
    }
    else{      /* deal with real data */
      switch(current_type){
      case 0x0:   /* unknown data type  */
      case 0x1:   /* long integer       */
      case 0x2:   /* IEEE floating point*/
      case 0x9:   /* VAX floating point */
	for(j = i; j < lk.head_pos; j=j+2){
	  swapped_intcpy(temp,&(source[j*2]),int_len);
	  memcpy(&(buffer[j*2]),temp,int_len);
	}
	i = lk.head_pos;
	break;
      case 0x4:   /* short integer     */
      case 0x5:   /* unsigned integer  */
      case 0x30:  
      case 0x34:
      case 0x35:
	for(j = i; j < lk.head_pos; j=j+1){
	  swapped_shortcpy(temp2,&(source[j*2]),short_len);
	  memcpy(&(buffer[j*2]),temp2,short_len);
	}
	i = lk.head_pos;	
	break;
      case 0x3:  /* char string        */
      case 0x6:
      case 0x7:
      case 0x36:
      case 0x37:
	memcpy(&(buffer[i*2]),&(source[i*2]),(lk.head_pos - i)*2);
	i = lk.head_pos;		
	break;
      case 0x8:  /* 64 bit */
      case 0xA:  /* 64 bit VAX floating point */
	for(j = i; j < lk.head_pos; j=j+4){
	  swapped_shortcpy(temp,&(source[j*2]),long_len);
	  memcpy(&(buffer[j*2]),temp,long_len);
	}
	i = lk.head_pos;		
	break;
      case 0xF:  /* repeating structure, for now */
	for(j = i; j < lk.head_pos; j=j+2){
	  swapped_intcpy(temp,&(source[j*2]),int_len);
	  memcpy(&(buffer[j*2]),temp,int_len);
	}
	i = lk.head_pos;
	break;
      default:
	fprintf(stderr,"Wrong datatype 0x%x\n",current_type);
	break;
      }
    }
  }
  evStack_free (head);
}


/**********************************************************
 *   evStack *init_evStack()                              *
 *   set up the head for event stack                      *
 *********************************************************/
static evStack *init_evStack()
{
  evStack *evhead;
  
  evhead = (evStack *)malloc(1*sizeof(evStack));
  if(evhead == NULL){
    fprintf(stderr,"Cannot allocate memory for evStack\n");
    exit (1);
  }
  evhead->length = 0;
  evhead->posi = 0;
  evhead->type = 0x0;
  evhead->tag = 0x0;
  evhead->num = 0x0;
  evhead->next = NULL;
  return evhead;
}

/*********************************************************
 *    evStack *evStack_top(evStack *head)                *
 *  return the top of the evStack pointed by head        *
 ********************************************************/
static evStack *evStack_top(evStack *head)
{
  evStack *p;

  p = head;
  if (p->next == NULL)
    return (NULL);
  else
    return (p->next);
}

/********************************************************
 *    void evStack_popoff(evStack *head)                *
 * pop off the top of the stack item                    *
 *******************************************************/
static void evStack_popoff(evStack *head)
{
  evStack *p,*q;

  q = head;
  if(q->next == NULL){
    fprintf(stderr,"Empty stack\n");
    return;
  }
  p = q->next;
  q->next = p->next;
  free ((char*)p);
}

/*******************************************************
 *     void evStack_pushon()                           *
 * push an item on to the stack                        *
 ******************************************************/
static void evStack_pushon(int size,
			   int posi,
			   int type,
			   int tag,
			   int num,
			   evStack *head)
{
  evStack *p, *q;

  p = (evStack *)malloc(1*sizeof(evStack));
  if (p == NULL){
    fprintf(stderr,"Not enough memory for stack item\n");
    exit(1);
  }
  q = head;
  p->length = size;
  p->posi = posi;
  p->type = type;
  p->tag = tag;
  p->num = num;
  p->next = q->next;
  q->next = p;
}

/******************************************************
 *       void evStack_free()                          *
 * Description:                                       *
 *    Free all memory allocated for the stack         *
 *****************************************************/
static void evStack_free(evStack *head)
{
  evStack *p, *q;

  p = head;
  while (p != NULL){
    q = p->next;
    free ((char*)p);
    p = q;
  }
}
