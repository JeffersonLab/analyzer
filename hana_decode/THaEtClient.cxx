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
#include <cerrno>
#include <cstring>
#include <ctime>        // for timespec
#include <stdexcept>
#include "evio.h"       // for evioswap
#include "et_private.h" // for ET_VERSION
#include <byteswap.h>

using namespace std;

static const int FAST          = 25;
static const int SMALL_TIMEOUT = 10;
static const int BIG_TIMEOUT   = 20;

namespace Decoder {

// Common member initialization for our constructors
#define initflags                                       \
nread(0), nused(0), timeout(BIG_TIMEOUT),               \
id(0), my_att(0),                                       \
daqhost(nullptr), session(nullptr), etfile(nullptr),    \
waitflag(0), didclose(0), notopened(0), firstread(1),   \
firstRateCalc(1), evsum(0), xcnt(0), daqt1(-1), ratesum(0)

#define EVETCHECKINIT(x)					\
  if(x.etSysId == 0) {						\
    printf("%s: ERROR: evet not initiallized\n", __func__);	\
    return -1;}

THaEtClient::THaEtClient(Int_t smode)
  : initflags
{
  // uses default server (where CODA runs)
  const char* defaultcomputer = ADAQS2;
  THaEtClient::codaOpen(defaultcomputer,smode);
}

THaEtClient::THaEtClient(const char* computer,Int_t smode)
  : initflags
{
  THaEtClient::codaOpen(computer,smode);
}

THaEtClient::THaEtClient(const char* computer, const char* mysession, Int_t smode)
  : initflags
{
  THaEtClient::codaOpen(computer, mysession, smode);
}

THaEtClient::~THaEtClient() {
  delete [] daqhost;
  delete [] session;
  delete [] etfile;
  Int_t status = THaEtClient::codaClose();
  if (status == CODA_ERROR) cout << "ERROR: closing THaEtClient"<<endl;
}

Int_t THaEtClient::init(const char* mystation)
{
  static char station[ET_STATNAME_LENGTH];
  if(!mystation||strlen(mystation)>=ET_STATNAME_LENGTH){
    cout << "THaEtClient: bad station name\n";
    return CODA_FATAL;
  }
  strcpy(station,mystation);

  et_openconfig openconfig{};
  et_open_config_init(&openconfig);

  et_open_config_sethost(openconfig, daqhost);
  et_open_config_setcast(openconfig, ET_DIRECT);
	et_sys_id id = 0;
  if (et_open(&id, etfile, openconfig) != ET_OK) {
    notopened = 1;
    cout << "THaEtClient: cannot open ET system"<<endl;
    cout << "Likely causes:  "<<endl;
    cout << "  1. Incorrect SESSION environment variable (it can also be passed to codaOpen)"<<endl;
    cout << "  2. ET not running (CODA not running) on specified computer"<<endl;
    return CODA_FATAL;
  }

	evetOpen(id, ET_CHUNK_SIZE, evh);
  et_open_config_destroy(openconfig);


	/* set level of debug output (everything) */
	et_system_setdebug(evh.etSysId, ET_DEBUG_ERROR);


	/* define station to create */
  et_statconfig sconfig{};
  et_station_config_init(&sconfig);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setuser(sconfig, ET_STATION_USER_MULTI);
  et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
  et_station_config_setcue(sconfig, 100);
  et_station_config_setprescale(sconfig, 1);
  et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
  et_stat_id my_stat{};
  Int_t status = et_station_create(evh.etSysId, &my_stat, station, sconfig);
  if (status < ET_OK) {
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
  if (et_station_attach(evh.etSysId, my_stat, &evh.etAttId) < 0) {
    cout << "THaEtClient: error in station attach"<<endl;
    return CODA_ERROR;
  }
  return CODA_OK;
}

Int_t THaEtClient::codaClose() {
  if (didclose || firstread) return CODA_OK;
  didclose = 1;
  if (notopened) return CODA_ERROR;
  if (et_station_detach(evh.etSysId, evh.etAttId) != ET_OK) {
    cout << "ERROR: codaClose: detaching from ET"<<endl;
    return CODA_ERROR;
  }
	if (evetClose(evh) ) {
    cout << "ERROR: evetClose: error closing EVIO handle"<<endl;
    return CODA_ERROR;
	}
  if (et_close(evh.etSysId) != ET_OK) {
    cout << "ERROR: codaClose: error closing ET"<<endl;
    return CODA_ERROR;
  }
  return CODA_OK;
}

Int_t THaEtClient::codaRead()
{
  if (firstread) {
    Int_t status = init();
    if (status != CODA_OK) {
      cout << "THaEtClient: ERROR: codaRead, cannot connect to CODA"<<endl;
      return CODA_FATAL;
    }
    firstread = 0;
  }

  //  Read a chunk of data, return read status (0 = ok, else not).
  //  To try to use network efficiently, it actually gets
  //  the events in chunks, and passes them to the user.
  const size_t bpi = sizeof(uint32_t);
	int status;
	const uint32_t *readBuffer;
	uint32_t len;
	status = evetReadNoCopy(evh, &readBuffer, &len);
	if(status == 0){
		if( !evbuffer.grow(len/bpi+1) )
			throw runtime_error("THaEtClient: Maximum event buffer size reached");
		assert(bpi * evbuffer.size() >= (size_t)len);
		memcpy(evbuffer.get(), readBuffer, sizeof(uint32_t)*len);
	}
	
  if (firstRateCalc) {
  	firstRateCalc = 0;
  	daqt1 = time(nullptr);
  }
  else {
  	time_t daqt2 = time(nullptr);
  	double tdiff = difftime(daqt2, daqt1);
		evsum += evh.etChunkNumRead;
    if ((tdiff > 4) && (evsum > 30))
	 	{
			double daqrate  = static_cast<double>(evsum)/tdiff;
      evsum    = 0;
      ratesum += daqrate;
      double avgrate  = ratesum/++xcnt;
				
      if (evh.verbose > 0) {
      	printf("ET rate %4.1f Hz in %2.0f sec, avg %4.1f Hz\n", daqrate, tdiff, avgrate);
      }
      if (waitflag != 0) {
      	timeout = (avgrate > FAST) ? SMALL_TIMEOUT : BIG_TIMEOUT;
      }
      daqt1 = time(nullptr);
    }
  }
	return status;
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
  if (s == nullptr)
    return CODA_ERROR;
  TString mysession(s);
  return codaOpen( computer, mysession, smode );
}

bool THaEtClient::isOpen() const {
  return (notopened==1&&didclose==0);
}


int32_t
THaEtClient::evetOpen(et_sys_id etSysId, int32_t chunk, evetHandle_t &evh)
{
  evh.etSysId = etSysId;
  evh.etChunkSize = chunk;

  evh.etAttId = 0;
  evh.currentChunkID = -1;
  evh.etChunkNumRead = -1;

  evh.currentChunkStat.evioHandle = 0;
  evh.currentChunkStat.length = 0;
  evh.currentChunkStat.endian = 0;
  evh.currentChunkStat.swap = 0;

  /* allocate some memory */
  evh.etChunk = (et_event **) calloc((size_t)chunk, sizeof(et_event *));
  if (evh.etChunk == NULL) {
    printf("%s: out of memory\n", __func__);
    evh.etSysId = 0;
    return CODA_FATAL;
  }

  return CODA_OK;
}

int32_t
THaEtClient::evetClose(evetHandle_t &evh)
{

	// Close up any currently opened evBufferOpen's.
	if(evh.currentChunkStat.evioHandle)
	{
		int32_t stat = evClose(evh.currentChunkStat.evioHandle);
		if(stat != S_SUCCESS)
		{
			printf("%s: ERROR: evClose returned %s\n",
					__func__, et_perror(stat));
			return CODA_ERROR;
		}
	}

	// put any events we may still have
	if(evh.etChunkNumRead != -1)
	{
		/* putting array of events */
		int32_t status = et_events_put(evh.etSysId, evh.etAttId, evh.etChunk, evh.etChunkNumRead);
		if (status != ET_OK)
		{
			printf("%s: ERROR: et_events_put returned %s\n",
					__func__, et_perror(status));
			return CODA_FATAL;
		}
	}

	// free up the etChunk memory
	if(evh.etChunk)
		free(evh.etChunk);

	return CODA_OK;
}


int32_t
THaEtClient::evetGetEtChunks(evetHandle_t &evh)
{
	if(evh.verbose > 1)
		printf("%s: enter\n", __func__);

	EVETCHECKINIT(evh);

	int32_t status;
	if (waitflag == 0) {
		status = et_events_get(evh.etSysId, evh.etAttId, evh.etChunk, ET_SLEEP, NULL, evh.etChunkSize, &evh.etChunkNumRead);
	}
	else {
		struct timespec twait{};
		twait.tv_sec  = timeout;
		twait.tv_nsec = 0;
		status = et_events_get(evh.etSysId, evh.etAttId, evh.etChunk, ET_TIMED, &twait, evh.etChunkSize, &evh.etChunkNumRead);
	}
	if(status != ET_OK)
	{
		printf("%s: ERROR: et_events_get returned (%d) %s\n",
				__func__, status, et_perror(status));
		if (status == ET_ERROR_TIMEOUT) {
			printf("et_netclient: timeout calling et_events_get\n");
			printf("Probably means CODA is not running...\n");
		}
		return CODA_FATAL;
	}

	evh.currentChunkID = -1;

	return CODA_OK;
}

int32_t
THaEtClient::evetGetChunk(evetHandle_t &evh)
{
	if(evh.verbose > 1)
		printf("%s: enter\n", __func__);

	EVETCHECKINIT(evh);

	evh.currentChunkID++;

	if((evh.currentChunkID >= evh.etChunkNumRead) || (evh.etChunkNumRead == -1))
	{
		if(evh.etChunkNumRead != -1)
		{
			/* putting array of events */
			int32_t status = et_events_put(evh.etSysId, evh.etAttId, evh.etChunk, evh.etChunkNumRead);
			if (status != ET_OK)
			{
				printf("%s: ERROR: et_events_put returned %s\n",
						__func__, et_perror(status));
				return CODA_FATAL;
			}
		}

		// out of chunks.  get some more
		int32_t stat = evetGetEtChunks(evh);
		if(stat != 0)
		{
			printf("%s: ERROR: evetGetEtChunks(evh) returned %d\n",
					__func__, stat);
			return CODA_FATAL;
		}
		evh.currentChunkID++;

	}

	// Close previous handle
	if(evh.currentChunkStat.evioHandle)
	{
		int32_t stat = evClose(evh.currentChunkStat.evioHandle);
		if(stat != ET_OK)
		{
			printf("%s: ERROR: evClose returned %s\n",
					__func__, et_perror(stat));
			return CODA_FATAL;
		}
	}

	et_event *currentChunk = evh.etChunk[evh.currentChunkID];
	et_event_getdata(currentChunk, (void **) &evh.currentChunkStat.data);
	et_event_getlength(currentChunk, &evh.currentChunkStat.length);
	et_event_getendian(currentChunk, &evh.currentChunkStat.endian);
	et_event_needtoswap(currentChunk, &evh.currentChunkStat.swap);

	if(evh.verbose > 1)
	{
		uint32_t *data = evh.currentChunkStat.data;
		uint32_t idata = 0, len = evh.currentChunkStat.length;

		printf("data byte order = %s\n",
				(evh.currentChunkStat.endian == ET_ENDIAN_BIG) ? "BIG" : "LITTLE");
		printf(" %2d/%2d: data (len = %d) %s  int = %d\n",
				evh.currentChunkID, evh.etChunkNumRead,
				(int) len,
				evh.currentChunkStat.swap ? "needs swapping" : "does not need swapping",
				evh.currentChunkStat.swap ? bswap_32(data[0]) : data[0]);

		for(idata = 0; idata < ((32 < (len>>2)) ? 32 : (len>>2)); idata++)
		{
			printf("0x%08x ", evh.currentChunkStat.swap ? bswap_32(data[idata]) : data[idata]);
			if(((idata+1) % 8) == 0)
				printf("\n");
		}
		printf("\n");
	}

	int32_t evstat = evOpenBuffer((char *) evh.currentChunkStat.data, evh.currentChunkStat.length,
			(char *)"r",  &evh.currentChunkStat.evioHandle);

	if(evstat != 0)
	{
		printf("%s: ERROR: evOpenBuffer returned %s\n",
				__func__, et_perror(evstat));
		return CODA_FATAL;
	}

	return CODA_OK;
}

int32_t
THaEtClient::evetReadNoCopy(evetHandle_t &evh, const uint32_t **outputBuffer, uint32_t *length)
{
	if(evh.verbose > 1)
		printf("%s: enter\n", __func__);

	EVETCHECKINIT(evh);

	int32_t status = evReadNoCopy(evh.currentChunkStat.evioHandle,
																outputBuffer, length);
	if(status != S_SUCCESS)
	{
		// Get a new chunk from et_get_event
		status = evetGetChunk(evh);
		if(status == 0)
		{
			status = evReadNoCopy(evh.currentChunkStat.evioHandle,
														outputBuffer, length);
		}
		else
		{
			printf("%s: ERROR: evetGetChunk failed %d\n",
					__func__, status);
		}
	}

	return status;
}

} //Namespace
ClassImp(Decoder::THaEtClient)
