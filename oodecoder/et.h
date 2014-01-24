/*----------------------------------------------------------------------------*
 *  Copyright (c) 1998        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 * TJNAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606 *
 *      heyes@cebaf.gov   Tel: (804) 269-7030    Fax: (804) 269-5800          *
 *----------------------------------------------------------------------------*
 * Description:
 *      Header file for ET system
 *
 * Author:
 *	Carl Timmer
 *	TJNAF Data Acquisition Group
 *
 * Revision History:
 *
 *----------------------------------------------------------------------------*/

#ifndef _ET_H_
#define _ET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 

#ifdef	__cplusplus
extern "C" {
#endif

/*********************************************************************/
/*            The following default values may be changed            */
/*********************************************************************/
/* station stuff */
#define ET_STATION_CUE      10	 /* max # events cued in nonblocking stat */
#define ET_STATION_PRESCALE 1	 /* prescale of blocking stations */

/* system stuff */
#define ET_SYSTEM_EVENTS    300	 /* total # of events */
#define ET_SYSTEM_NTEMPS    300	 /* max # of temp events */
#define ET_SYSTEM_ESIZE     1000 /* size of normal events in bytes */
#define ET_SYSTEM_NSTATS    10	 /* max # of stations */

/* network stuff */
#define ET_MULTICAST_PORT  11111 /* udp multicast port */
#define ET_BROADCAST_PORT  11111 /* udp broadcast port */
#define ET_SERVER_PORT     11111 /* tcp server port */

#define ET_MULTICAST_ADDR  "239.200.0.0"    /* random multicast addr */
#define ET_BROADCAST_ADDR  "129.57.35.255"  /* daq group subnet */

/*********************************************************************/
/*                              WARNING                              */
/* Changing ET_STATION_SELECT_INTS requires a full recompile of all  */
/* ET libraries (local & remote) whose users interact in any way.    */
/*********************************************************************/
/* # of control integers associated with each station */
#define ET_STATION_SELECT_INTS 4

/*********************************************************************/
/*             DO NOT CHANGE ANYTHING BELOW THIS LINE !!!            */
/*********************************************************************/

#define ET_MULTICAST_TTL   1
#define ET_HOST_LOCAL      ".local"
#define ET_HOST_REMOTE     ".remote"
#define ET_HOST_ANYWHERE   ".anywhere"
#define ET_HOST_AS_LOCAL   1
#define ET_HOST_AS_REMOTE  0

/* name lengths */
#define ET_FILENAME_LENGTH  101 /* max length of ET system file name + 1 */
#define ET_FUNCNAME_LENGTH  51  /* max length of user's 
                                   event selection func name + 1 */
#define ET_STATNAME_LENGTH  51  /* max length of station name + 1 */
/* max length of temp event file name + 1 */
#define ET_TEMPNAME_LENGTH  (ET_FILENAME_LENGTH+10)

/* STATION STUFF */
/* et_stat_id of grandcentral station */
#define ET_GRANDCENTRAL 0

/* status */
#define ET_STATION_UNUSED   0
#define ET_STATION_CREATING 1
#define ET_STATION_IDLE     2
#define ET_STATION_ACTIVE   3

/* how many user process can attach */
#define ET_STATION_USER_MULTI  0
#define ET_STATION_USER_SINGLE 1

/* who can change settings (not implemented) */
#define ET_STATION_UNPROTECTED 0
#define ET_STATION_PROTECTED   1

/* can block for event flow or not */
#define ET_STATION_NONBLOCKING 0
#define ET_STATION_BLOCKING    1

/* criterion for event selection */
#define ET_STATION_SELECT_ALL   1
#define ET_STATION_SELECT_MATCH 2
#define ET_STATION_SELECT_USER  3

/* how to restore events in dead user process */
#define ET_STATION_RESTORE_OUT 0	/* to output list */
#define ET_STATION_RESTORE_IN  1	/* to input  list */
#define ET_STATION_RESTORE_GC  2	/* to gc station input list */

/* errors */
#define ET_OK              0
#define ET_ERROR          -1
#define ET_ERROR_TOOMANY  -2
#define ET_ERROR_EXISTS   -3
#define ET_ERROR_WAKEUP   -4
#define ET_ERROR_TIMEOUT  -5
#define ET_ERROR_EMPTY    -6
#define ET_ERROR_BUSY     -7
#define ET_ERROR_DEAD     -8
#define ET_ERROR_READ     -9
#define ET_ERROR_WRITE    -10
#define ET_ERROR_REMOTE   -11
#define ET_ERROR_NOREMOTE -12

/* debug levels */
#define ET_DEBUG_NONE   0
#define ET_DEBUG_SEVERE 1
#define ET_DEBUG_ERROR  2
#define ET_DEBUG_WARN   3
#define ET_DEBUG_INFO   4

/* event priority */
#define ET_LOW  0
#define ET_HIGH 1

/* event owner is attachment # unless owned by system */
#define ET_SYS -1

/* event is temporary or not */
#define ET_EVENT_TEMP   1
#define ET_EVENT_NORMAL 0

/* event is new (obtained thru et_event(s)_new) or not */
#define ET_EVENT_NEW  1
#define ET_EVENT_USED 0

/* event get/new routines' wait mode flags */
#define ET_SLEEP  0
#define ET_TIMED  1
#define ET_ASYNC  2
#define ET_MODIFY 4

/* et_open waiting modes */
#define ET_OPEN_NOWAIT 0
#define ET_OPEN_WAIT   1

/* are we remote or local? */
#define ET_REMOTE        0
#define ET_LOCAL         1
#define ET_LOCAL_NOSHARE 2

/* do we swap data or not? */
#define ET_NOSWAP 0
#define ET_SWAP   1

/* do we use broadcast or multicast to find server port #'s */
#define ET_MULTICAST 0
#define ET_BROADCAST 1
#define ET_DIRECT    2

/* status of data in an event */
#define ET_DATA_OK               0
#define ET_DATA_CORRUPT          1
#define ET_DATA_POSSIBLY_CORRUPT 2

/* STRUCTURES */

/*
 * et_event:
 *-----------------------------
 * next		: next event in linked list
 * priority	: HIGH or LOW
 * owner	: # of attachment that owns it, else ET_EVENT_SYS (-1)
 * length	: size of actual data in bytes
 * memsize	: total size of available data memory in bytes
 * temp		: =ET_EVENT_TEMP if temporary event, else =ET_EVENT_NORMAL
 * age		: =ET_EVENT_NEW if it's a new event, else =ET_EVENT_USED
 *		: New events always go to GrandCentral if user crashes.
 * datastatus	: =ET_DATA_OK normally, =ET_DATA_CORRUPT if data corrupt,
 *		: =ET_DATA_POSSIBLY_CORRUPT if data questionable.
 * modify	: for REMOTE events- flag to tell server
 *		: whether this event will be modified with
 *		: the intention of sending it back to the server
 *		: and into the ET system.
 * pointer	: for REMOTE events- pointer to this event in the
 *		: server's space. Used for writing of modified events.
 * filename	: filename of temp event
 * control	: array of ints to select on (see et_stat_config->select)
 * pdata	: = pointer below, or points to temp event buffer
 * data		: pointer to data if not temp event
 */
 
typedef struct et_event_t {
  struct et_event_t *next;
  int   priority;
  int   owner;
  int   length;
  int   memsize;
  int   temp;
  int   age;
  int   datastatus;
  /* for remote events */
  int   modify;
  int   pointer;
  /********************/
  char  filename[ET_TEMPNAME_LENGTH];
  int   control[ET_STATION_SELECT_INTS];
  void *pdata;
  char *data;
} et_event;

/* TYPES */
typedef void *et_sys_id;
typedef void *et_statconfig;
typedef void *et_sysconfig;
typedef void *et_openconfig;
typedef int   et_stat_id;
typedef int   et_proc_id;
typedef int   et_att_id;

/***********************/
/* Extern declarations */
/***********************/

/* event functions */
extern int   et_event_new(et_sys_id id, et_att_id att, et_event **pe,  
                          int mode, struct timespec *deltatime, int size);
extern int   et_events_new(et_sys_id id, et_att_id att, et_event *pe[], 
                           int mode, struct timespec *deltatime, int size, 
                           int num, int *nread);

extern int   et_event_get(et_sys_id id, et_att_id att, et_event **pe,  
                          int mode, struct timespec *deltatime);
extern int   et_events_get(et_sys_id id, et_att_id att, et_event *pe[], 
                           int mode, struct timespec *deltatime, 
                           int num, int *nread);

extern int   et_event_put(et_sys_id id, et_att_id att, et_event *pe);
extern int   et_events_put(et_sys_id id, et_att_id att, et_event *pe[], 
                           int num);
			   
extern int   et_event_dump(et_sys_id id, et_att_id att, et_event *pe);
extern int   et_events_dump(et_sys_id id, et_att_id att, et_event *pe[],
			    int num);

extern int   et_event_setpriority(et_event *pe, int pri);
extern int   et_event_getpriority(et_event *pe, int *pri);

extern int   et_event_setlength(et_event *pe, int len);
extern int   et_event_getlength(et_event *pe, int *len);

extern int   et_event_setcontrol(et_event *pe, int con[], int num);
extern int   et_event_getcontrol(et_event *pe, int con[]);

extern int   et_event_setdatastatus(et_event *pe, int datastatus);
extern int   et_event_getdatastatus(et_event *pe, int *datastatus);

extern int   et_event_getdata(et_event *pe, void **data);

/* system functions. */
extern int  et_open(et_sys_id* id, char *filename, et_openconfig openconfig);
extern int  et_close(et_sys_id id);
extern int  et_forcedclose(et_sys_id id);
extern int  et_system_start(et_sys_id* id, et_sysconfig sconfig);
extern int  et_system_close(et_sys_id id);
extern int  et_alive(et_sys_id id);
extern int  et_swap(et_sys_id id);
extern void et_swap_CODA(long *cbuf, int nlongs);
extern int  et_wait_for_alive(et_sys_id id);

/* user process functions */
extern int  et_wakeup_attachment(et_sys_id id, et_att_id att);
extern int  et_wakeup_all(et_sys_id id, et_stat_id stat_id);

/* station manipulation */
extern int  et_station_create(et_sys_id id, et_stat_id *stat_id,
                               char *stat_name, et_statconfig sconfig);
extern int  et_station_remove(et_sys_id id, et_stat_id stat_id);
extern int  et_station_attach(et_sys_id id, et_stat_id stat_id,
                              et_att_id *att);
extern int  et_station_detach(et_sys_id id, et_att_id att);

/***************************************************************/
/* routines to store and read station config parameters used  */
/* to define a station upon its creation.                     */
/***************************************************************/
extern int et_station_config_init(et_statconfig* sconfig);
extern int et_station_config_destroy(et_statconfig sconfig);

extern int et_station_config_setblock(et_statconfig sconfig, int val);
extern int et_station_config_getblock(et_statconfig sconfig, int *val);

extern int et_station_config_setselect(et_statconfig sconfig, int val);
extern int et_station_config_getselect(et_statconfig sconfig, int *val);

extern int et_station_config_setuser(et_statconfig sconfig, int val);
extern int et_station_config_getuser(et_statconfig sconfig, int *val);

extern int et_station_config_setrestore(et_statconfig sconfig, int val);
extern int et_station_config_getrestore(et_statconfig sconfig, int *val);

extern int et_station_config_setcue(et_statconfig sconfig, int val);
extern int et_station_config_getcue(et_statconfig sconfig, int *val);

extern int et_station_config_setprescale(et_statconfig sconfig, int val);
extern int et_station_config_getprescale(et_statconfig sconfig, int *val);

extern int et_station_config_setselectwords(et_statconfig sconfig, int val[]);
extern int et_station_config_getselectwords(et_statconfig sconfig, int val[]);

extern int et_station_config_setfunction(et_statconfig sconfig, char *val);
extern int et_station_config_getfunction(et_statconfig sconfig, char *val);

extern int et_station_config_setlib(et_statconfig sconfig, char *val);
extern int et_station_config_getlib(et_statconfig sconfig, char *val);

extern int et_event_needtoswap(et_event *p, int *val);

extern int et_event_CODAswap(et_event *x);


/**************************************************************/
/*        routines to read existing station's parameters      */
/**************************************************************/
extern int et_station_isattached(et_sys_id id, et_stat_id stat_id, et_att_id att);
extern int et_station_exists(et_sys_id id, et_stat_id *stat_id, char *stat_name);
extern int et_station_name_to_id(et_sys_id id, et_stat_id *stat_id, char *stat_name);
extern int et_station_getattachments(et_sys_id id, et_stat_id stat_id, int *numatts);
extern int et_station_getstatus(et_sys_id id, et_stat_id stat_id, int *status);
extern int et_station_getinputcount(et_sys_id id, et_stat_id stat_id, int *cnt);
extern int et_station_getoutputcount(et_sys_id id, et_stat_id stat_id, int *cnt);
extern int et_station_getblock(et_sys_id id, et_stat_id stat_id, int *block);
extern int et_station_getrestore(et_sys_id id, et_stat_id stat_id, int *restore);
extern int et_station_getuser(et_sys_id id, et_stat_id stat_id, int *user);
extern int et_station_getprescale(et_sys_id id, et_stat_id stat_id, 
                                  int *prescale);
extern int et_station_getcue(et_sys_id id, et_stat_id stat_id, int *cue);
extern int et_station_getselect(et_sys_id id, et_stat_id stat_id, int *select);
extern int et_station_getselectwords(et_sys_id id, et_stat_id stat_id, 
                                     int select[]);
extern int et_station_getlib(et_sys_id id, et_stat_id stat_id, char *lib);
extern int et_station_getfunction(et_sys_id id, et_stat_id stat_id, 
                                  char *function);

/*************************************************************/
/* routines to store and read system config parameters used  */
/* to define a system upon its creation.                     */
/*************************************************************/
extern int et_system_config_init(et_sysconfig *sconfig);
extern int et_system_config_destroy(et_sysconfig sconfig);

extern int et_system_config_setevents(et_sysconfig sconfig, int val);
extern int et_system_config_getevents(et_sysconfig sconfig, int *val);

extern int et_system_config_setsize(et_sysconfig sconfig, int val);
extern int et_system_config_getsize(et_sysconfig sconfig, int *val);

extern int et_system_config_settemps(et_sysconfig sconfig, int val);
extern int et_system_config_gettemps(et_sysconfig sconfig, int *val);

extern int et_system_config_setstations(et_sysconfig sconfig, int val);
extern int et_system_config_getstations(et_sysconfig sconfig, int *val);

extern int et_system_config_setprocs(et_sysconfig sconfig, int val);
extern int et_system_config_getprocs(et_sysconfig sconfig, int *val);

extern int et_system_config_setattachments(et_sysconfig sconfig, int val);
extern int et_system_config_getattachments(et_sysconfig sconfig, int *val);

extern int et_system_config_setfile(et_sysconfig sconfig, char *val);
extern int et_system_config_getfile(et_sysconfig sconfig, char *val);

extern int et_system_config_setcast(et_sysconfig sconfig, int val);
extern int et_system_config_getcast(et_sysconfig sconfig, int *val);

extern int et_system_config_setport(et_sysconfig sconfig, unsigned short val);
extern int et_system_config_getport(et_sysconfig sconfig, unsigned short *val);

extern int et_system_config_setserverport(et_sysconfig sconfig, unsigned short val);
extern int et_system_config_getserverport(et_sysconfig sconfig, unsigned short *val);

extern int et_system_config_setaddress(et_sysconfig sconfig, char *val);
extern int et_system_config_getaddress(et_sysconfig sconfig, char *val);

/*************************************************************/
/*          routines to read system information              */
/*************************************************************/
extern int et_system_getlocality(et_sys_id id, int *locality);
extern int et_system_setdebug(et_sys_id id, int debug);
extern int et_system_getdebug(et_sys_id id, int *debug);
extern int et_system_getnumevents(et_sys_id id, int *numevents);
extern int et_system_geteventsize(et_sys_id id, int *eventsize);
extern int et_system_gettempsmax(et_sys_id id, int *tempsmax);
extern int et_system_getstationsmax(et_sys_id id, int *stationsmax);
extern int et_system_getprocsmax(et_sys_id id, int *procsmax);
extern int et_system_getattsmax(et_sys_id id, int *attsmax);
extern int et_system_getheartbeat(et_sys_id id, int *heartbeat);
extern int et_system_getpid(et_sys_id id, pid_t *pid);
extern int et_system_getprocs(et_sys_id id, int *procs);
extern int et_system_getattachments(et_sys_id id, int *atts);
extern int et_system_getstations(et_sys_id id, int *stations);
extern int et_system_gettemps(et_sys_id id, int *temps);

/*************************************************************/
/* routines to store and read system config parameters used  */
/* to open an ET system                                      */
/*************************************************************/
extern int et_open_config_init(et_openconfig *sconfig);
extern int et_open_config_destroy(et_openconfig sconfig);

extern int et_open_config_setwait(et_openconfig sconfig, int val);
extern int et_open_config_getwait(et_openconfig sconfig, int *val);

extern int et_open_config_setcast(et_openconfig sconfig, int val);
extern int et_open_config_getcast(et_openconfig sconfig, int *val);

extern int et_open_config_setTTL(et_openconfig sconfig, int val);
extern int et_open_config_getTTL(et_openconfig sconfig, int *val);

extern int et_open_config_setmode(et_openconfig sconfig, int val);
extern int et_open_config_getmode(et_openconfig sconfig, int *val);

extern int et_open_config_setport(et_openconfig sconfig, unsigned short val);
extern int et_open_config_getport(et_openconfig sconfig, unsigned short *val);

extern int et_open_config_setserverport(et_openconfig sconfig, unsigned short val);
extern int et_open_config_getserverport(et_openconfig sconfig, unsigned short *val);

extern int et_open_config_settimeout(et_openconfig sconfig, 
                                     struct timespec val);
extern int et_open_config_gettimeout(et_openconfig sconfig, 
                                     struct timespec *val);

extern int et_open_config_setaddress(et_openconfig sconfig, char *val);
extern int et_open_config_getaddress(et_openconfig sconfig, char *val);

extern int et_open_config_sethost(et_openconfig sconfig, char *val);
extern int et_open_config_gethost(et_openconfig sconfig, char *val);


#ifdef	__cplusplus
}
#endif

#endif
