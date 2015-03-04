//////////////////////////////////////////////////////////////////////
//
//   THaEtClient
//   Data from ET Online System
//
//   THaEtClient contains normal CODA data obtained via
//   the ET (Event Transfer) online system invented
//   by the JLab DAQ group.
//   This code works locally or remotely and uses the
//   ET system in a particular mode favored by  hall A.
//
//   Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEtClient.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <cstring>
#include <strings.h>
#include <ctime>
#include <sys/time.h>
#include <netdb.h>

using namespace std;

namespace Decoder {

THaEtClient::THaEtClient(Int_t smode)
{
  // uses default server (where CODA runs)
  initflags();
  const char* defaultcomputer = ADAQS2;
  codaOpen(defaultcomputer,smode);
}

THaEtClient::THaEtClient(const char* computer,Int_t smode)
{
  initflags();
  codaOpen(computer,smode);
}

THaEtClient::THaEtClient(const char* computer, const char* mysession, Int_t smode)
{
  initflags();
  codaOpen(computer, mysession, smode);
}

THaEtClient::~THaEtClient() {
  delete [] daqhost;
  delete [] session;
  delete [] etfile;
  Int_t status = codaClose();
  if (status == CODA_ERROR) cout << "ERROR: closing THaEtClient"<<endl;
};

void THaEtClient::initflags()
{
  daqhost = NULL;
  session = NULL;
  etfile  = NULL;
  DEBUG = 0;
  FAST = 25;
  SMALL_TIMEOUT = 10;
  BIG_TIMEOUT = 20;
  didclose = 0;
  notopened = 0;
  firstread = 1;
  nread = 0;
  nused = 0;
  timeout = BIG_TIMEOUT;
  firstRateCalc = 1;
  evsum = 0;
  xcnt = 0;
  ratesum = 0;
};

Int_t THaEtClient::init(const char* mystation)
{
  static char station[ET_STATNAME_LENGTH];
  if(!mystation||strlen(mystation)>=ET_STATNAME_LENGTH){
    cout << "THaEtClient: bad station name\n";
    return CODA_ERROR;
  }
  strcpy(station,mystation);
  et_open_config_init(&openconfig);
  et_open_config_sethost(openconfig, daqhost);
  et_open_config_setcast(openconfig, ET_DIRECT);
  if (et_open(&id, etfile, openconfig) != ET_OK) {
    notopened = 1;
    cout << "THaEtClient: cannot open ET system"<<endl;
    cout << "Likely causes:  "<<endl;
    cout << "  1. Incorrect SESSION environment variable (it can also be passed to codaOpen)"<<endl;
    cout << "  2. ET not running (CODA not running) on specified computer"<<endl;
    return CODA_ERROR;
  }
  et_open_config_destroy(openconfig);
  et_station_config_init(&sconfig);
  et_station_config_setuser(sconfig, ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(sconfig, 1);
  et_station_config_setcue(sconfig, 100);
  et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  Int_t status;
  if ((status = et_station_create(id, &my_stat, station, sconfig)) < ET_OK) {
    if (status == ET_ERROR_EXISTS) {
      // ok
    }
    else if (status == ET_ERROR_TOOMANY) {
      cout << "THaEtClient: too many stations created"<<endl;
      return CODA_ERROR;
    }
    else if (status == ET_ERROR_REMOTE) {
      cout << "THaEtClient: memory or improper arg problems"<<endl;
      return CODA_ERROR;
    }
    else if (status == ET_ERROR_READ) {
      cout << "THaEtClient: network reading problem"<<endl;
      return CODA_ERROR;
    }
    else if (status == ET_ERROR_WRITE) {
      cout << "THaEtClient: network writing problem"<<endl;
      return CODA_ERROR;
    }
    else {
      cout << "THaEtClient: error in station creation"<<endl;
      return CODA_ERROR;
    }
  }
  et_station_config_destroy(sconfig);
  if (et_station_attach(id, my_stat, &my_att) < 0) {
    cout << "THaEtClient: error in station attach"<<endl;
    return CODA_ERROR;
  }
  return CODA_OK;
};

Int_t THaEtClient::codaClose() {
  if (didclose || firstread) return CODA_OK;
  didclose = 1;
  if (notopened) return CODA_ERROR;
  if (et_station_detach(id, my_att) != ET_OK) {
    cout << "ERROR: codaClose: detaching from ET"<<endl;
    return CODA_ERROR;
  }
  if (et_close(id) != ET_OK) {
    cout << "ERROR: codaClose: error closing ET"<<endl;
    return CODA_ERROR;
  }
  return CODA_OK;
};

Int_t THaEtClient::codaRead()
{
  //  Read a chunk of data, return read status (0 = ok, else not).
  //  To try to use network efficiently, it actually gets
  //  the events in chunks, and passes them to the user.

  et_event *evs[ET_CHUNK_SIZE];
  struct timespec twait;
  Int_t *data;
  Int_t err;
  Int_t lencpy, nbytes;
  Int_t swapflg;
  const Int_t bpi = sizeof(Int_t);

  if (firstread) {
    firstread = 0;
    Int_t status = init();
    if (status == CODA_ERROR) {
      cout << "THaEtClient: ERROR: codaRead, cannot connect to CODA"<<endl;
      return CODA_ERROR;
    }
  }

// pull out a ET_CHUNK_SIZE of events from ET
  if (nused >= nread) {
    if (waitflag == 0) {
      err = et_events_get(id, my_att, evs, ET_SLEEP, NULL, ET_CHUNK_SIZE, &nread);
    } else {
      twait.tv_sec  = timeout;
      twait.tv_nsec = 0;
      err = et_events_get(id, my_att, evs, ET_TIMED, &twait, ET_CHUNK_SIZE, &nread);
    }
    if (err < ET_OK) {
      if (err == ET_ERROR_TIMEOUT) {
	 printf("et_netclient: timeout calling et_events_get\n");
	 printf("Probably means CODA is not running...\n");
      }
      else {
	 printf("et_netclient: error calling et_events_get, %d\n", err);
      }
      nread = nused = 0;
      return CODA_ERROR;
    }

// reset
    nused = 0;

    for (Int_t j=0; j < nread; j++) {

      et_event_getdata(evs[j], (void **) &data);
      et_event_needtoswap(evs[j], &swapflg);
      if (swapflg == ET_SWAP) {
	et_event_CODAswap(evs[j]);
      }
      Int_t* pdata = data;
      Int_t event_size = *pdata + 1;
      if ( event_size > MAXEVLEN ) {
         printf("\nET:codaRead:ERROR:  Event from ET truncated\n");
         printf("-> Need a larger value than MAXEVLEN = %d \n",MAXEVLEN);
         return CODA_ERROR;
      }
      if (CODA_DEBUG) {
  	 cout<<"\n\n===== Event "<<j<<"  length "<<event_size<<endl;
	 pdata = data;
         for (Int_t i=0; i < event_size; i++, pdata++) {
           cout<<"evbuff["<<dec<<i<<"] = "<<*pdata<<" = 0x"<<hex<<*pdata<<endl;
	 }
      }
    }

    if (firstRateCalc) {
      firstRateCalc = 0;
      daqt1 = time(0);
    }
    else {
      time_t daqt2 = time(0);
      double tdiff = difftime(daqt2, daqt1);
      evsum += nread;
      if ((tdiff > 4) && (evsum > 30)) {
	 double daqrate  = static_cast<double>(evsum)/tdiff;
         evsum    = 0;
         ratesum += daqrate;
         double avgrate  = ratesum/++xcnt;

         if (CODA_VERBOSE)
           printf("ET rate %4.1f Hz in %2.0f sec, avg %4.1f Hz\n",
          	      daqrate, tdiff, avgrate);
         if (waitflag != 0) {
           timeout = (avgrate > FAST) ? SMALL_TIMEOUT : BIG_TIMEOUT;
         }
         daqt1 = time(0);
      }
    }
  }

// return an event
  et_event_getdata(evs[nused], (void **) &data);
  et_event_getlength(evs[nused], &nbytes);
  lencpy = (nbytes < bpi*MAXEVLEN) ? nbytes : bpi*MAXEVLEN;
  memcpy(evbuffer,data,lencpy);
  nused++;
  if (nbytes > bpi*MAXEVLEN) {
      cout<<"\nET:codaRead:ERROR:  CODA event truncated"<<endl;
      cout<<"-> Byte size exceeds bytes "<<bpi*MAXEVLEN<<endl;
      return CODA_ERROR;
  }

// if we've used all our events, put them back
  if (nused >= nread) {
    err = et_events_put(id, my_att, evs, nread);
    if (err < ET_OK) {
      cout<<"THaEtClient::codaRead: ERROR: calling et_events_put"<<endl;
      cout<<"This is potentially very bad !!\n"<<endl;
      cout<<"best not continue.... exiting... \n"<<endl;
      exit(1);
    }
  }
  return CODA_OK;
}

Int_t THaEtClient::codaOpen(const char* computer,
			    const char* mysession,
			    Int_t smode)
{
  // To run codaOpen, you need to know:
  // 1) What computer is ET running on ? (e.g. computer='adaql2')
  // 2) What session ? (usually env. variable $SESSION, e.g. 'onla')
  // 3) mode (0 = wait forever for data,  1 = time-out in a few seconds)
  delete [] daqhost;
  delete [] session;
  delete [] etfile;
  daqhost = new char[strlen(computer)+1];
  strcpy(daqhost,computer);
  etfile = new char[strlen(ETMEM_PREFIX)+strlen(mysession)+1];
  strcpy(etfile,ETMEM_PREFIX);
  strcat(etfile,mysession);
  session = new char[strlen(mysession)+1];
  strcpy(session,mysession);
  waitflag = smode;
  return CODA_OK;
}

Int_t THaEtClient::codaOpen(const char* computer, Int_t smode)
{
  // See comment in the above version of codaOpen()
  char* s = getenv("SESSION");
  if (s == NULL)
    return CODA_ERROR;
  TString mysession(s);
  return codaOpen( computer, mysession, smode );
}

bool THaEtClient::isOpen() const {
  return (notopened==1&&didclose==0);
}

}

ClassImp(Decoder::THaEtClient)
