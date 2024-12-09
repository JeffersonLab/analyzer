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
#include "TSystem.h"
#include <iostream>
#include <cstdlib>
#include <ctime>        // for timespec
#include <stdexcept>
#include "evio.h"
#include "et_private.h" // for ET_VERSION

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#elif HAVE_BUILTIN_BSWAP
#define bswap_32 __builtin_bswap32
#else
#define bswap_32(x) ET_SWAP32(x)
#endif

using namespace std;

static const int FAST = 25;
static const int SMALL_TIMEOUT = 10;
static const int BIG_TIMEOUT = 20;

namespace Decoder {

#define EVETCHECKINIT( x )                                        \
  if( x.etSysId == nullptr || x.etChunk == nullptr ) {            \
    printf("%s: ERROR: evet not initialized\n", __func__);        \
    return ET_ERROR;}

//______________________________________________________________________________
THaEtClient::THaEtClient( Int_t smode )
{
  // uses default server (where CODA runs)
  const char* defaultcomputer = "127.0.0.1";
  THaEtClient::codaOpen(defaultcomputer, smode);
}

//______________________________________________________________________________
THaEtClient::THaEtClient( const char* computer, Int_t smode )
{
  THaEtClient::codaOpen(computer, smode);
}

//______________________________________________________________________________
THaEtClient::THaEtClient( const char* computer, const char* mysession, Int_t smode )
{
  THaEtClient::codaOpen(computer, mysession, smode);
}

//______________________________________________________________________________
THaEtClient::~THaEtClient()
{
  THaEtClient::codaClose();
  // If error, codaClose already printed a message
}

//______________________________________________________________________________
Int_t THaEtClient::init( const char* mystation )
{
  if( opened && THaEtClient::codaClose() != CODA_OK )
    return CODA_ERROR;

  if( !mystation || !*mystation ) {
    cout << "THaEtClient: bad station name\n";
    return CODA_FATAL;
  }
  station = mystation;

  et_openconfig openconfig{};
  et_open_config_init(&openconfig);

  et_open_config_sethost(openconfig, daqhost.c_str());
  et_open_config_setcast(openconfig, ET_DIRECT);
  et_sys_id id{};
  if( et_open(&id, etfile.c_str(), openconfig) != ET_OK ) {
    cout << "THaEtClient: cannot open ET system" << endl;
    cout << "Likely causes:  " << endl;
    cout << "  1. Incorrect SESSION environment variable (it can also be passed to codaOpen)" << endl;
    cout << "  2. ET not running (CODA not running) on specified computer" << endl;
    return CODA_FATAL;
  }

  // Initialize evetHandle evh
  if( evetOpen(id, ET_CHUNK_SIZE) != ET_OK )
    return CODA_FATAL; // Error message already printed
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
  int status = et_station_create(id, &my_stat, station.c_str(), sconfig);
  if( status != ET_OK ) {
    if( status == ET_ERROR_EXISTS ) {
      // ok
    } else if( status == ET_ERROR_TOOMANY ) {
      cout << "THaEtClient: too many stations created" << endl;
      return CODA_ERROR;
    } else if( status == ET_ERROR_REMOTE ) {
      cout << "THaEtClient: memory or improper arg problems" << endl;
      return CODA_ERROR;
    } else if( status == ET_ERROR_READ ) {
      cout << "THaEtClient: network reading problem" << endl;
      return CODA_ERROR;
    } else if( status == ET_ERROR_WRITE ) {
      cout << "THaEtClient: network writing problem" << endl;
      return CODA_ERROR;
    } else {
      cout << "THaEtClient: error in station creation" << endl;
      return CODA_ERROR;
    }
  }
  et_station_config_destroy(sconfig);
  if( et_station_attach(id, my_stat, &evh.etAttId) != ET_OK ) {
    cout << "THaEtClient: error in station attach" << endl;
    return CODA_ERROR;
  }
  opened = true;
  return CODA_OK;
}

//______________________________________________________________________________
Int_t THaEtClient::codaClose()
{
  if( !opened )
    return CODA_OK;  // If not successfully opened, close() is a no-op
  if( et_station_detach(evh.etSysId, evh.etAttId) != ET_OK ) {
    cout << "WARNING: codaClose: detaching from ET" << endl;
//    return CODA_ERROR;
  }
  auto* id = evh.etSysId;
  evetClose(evh); // always succeeds, even if error
  if( et_close(id) != ET_OK ) {
    cout << "WARNING: codaClose: error closing ET" << endl;
//    return CODA_ERROR;
  }
  opened = false;
  return CODA_OK;
}

//______________________________________________________________________________
Int_t THaEtClient::codaRead()
{
  if( !opened ) {
    Int_t status = init();
    if( status != CODA_OK ) {
      cout << "THaEtClient: ERROR: codaRead, cannot connect to CODA" << endl;
      return CODA_FATAL;
    }
  }

  //  Read a chunk of data, return read status (0 = ok, else not).
  //  To try to use network efficiently, it actually gets
  //  the events in chunks, and passes them to the user.
  constexpr size_t bpi = sizeof(uint32_t);
  const uint32_t* readBuffer{};
  uint32_t len{};
  int status = evetReadNoCopy(evh, &readBuffer, &len);
  if( status == ET_OK ) {
    if( !evbuffer.grow(len / bpi + 1) )
      throw runtime_error("THaEtClient: Maximum event buffer size reached");
    assert(bpi * evbuffer.size() >= (size_t)len);
    memcpy(evbuffer.get(), readBuffer, bpi * len);
  }

  if( firstRateCalc ) {
    firstRateCalc = 0;
    daqt1 = time(nullptr);
  } else {
    time_t daqt2 = time(nullptr);
    double tdiff = difftime(daqt2, daqt1);
    evsum += evh.etChunkNumRead;
    if( tdiff > 4 && evsum > 30 ) {
      double daqrate = evsum / tdiff;
      evsum = 0;
      ratesum += daqrate;
      double avgrate = ratesum / ++xcnt;

      if( evh.verbose > 0 ) {
        printf("ET rate %4.1f Hz in %2.0f sec, avg %4.1f Hz\n",
               daqrate, tdiff, avgrate);
      }
      if( waitflag != 0 ) {
        evh.timeout = (avgrate > FAST) ? SMALL_TIMEOUT : BIG_TIMEOUT;
      }
      daqt1 = time(nullptr);
    }
  }
  return status;
}

//______________________________________________________________________________
Int_t THaEtClient::codaOpen( const char* computer,
                             const char* mysession,
                             Int_t smode )
{
  // To run codaOpen, you need to know:
  // 1) What computer is ET running on ? (e.g. computer='adaql2')
  // 2) What session ? (usually env. variable $SESSION, e.g. 'onla')
  // 3) mode (0 = wait forever for data,  1 = time-out in a few seconds)
  daqhost = computer;
  etfile = ETMEM_PREFIX;
  etfile.append(mysession);
  session = mysession;
  waitflag = smode;
  return CODA_OK;
}

//______________________________________________________________________________
Int_t THaEtClient::codaOpen( const char* computer, Int_t smode )
{
  // See comment in the above version of codaOpen()
  const auto* s = gSystem->Getenv("SESSION");
  if( s == nullptr )
    return CODA_ERROR;
  session = s;
  return codaOpen(computer, session.c_str(), smode);
}

//______________________________________________________________________________
bool THaEtClient::isOpen() const
{
  return opened;
}

//______________________________________________________________________________
int THaEtClient::evetOpen( et_sys_id etSysId, int32_t chunksz )
{
  evh.etSysId = nullptr;
  evh.etChunkSize = chunksz;

  evh.etAttId = 0;
  evh.currentChunkID = -1;
  evh.etChunkNumRead = -1;

  evh.currentChunkStat.evioHandle = 0;
  evh.currentChunkStat.length = 0;
  evh.currentChunkStat.endian = 0;
  evh.currentChunkStat.swap = 0;
  evh.timeout = BIG_TIMEOUT;
  evh.mode = waitflag;

  /* allocate some memory */
  free(evh.etChunk); // NOLINT(*-multi-level-implicit-pointer-conversion)
  evh.etChunk = (et_event**)calloc(chunksz, sizeof(et_event*));
  if( evh.etChunk == nullptr ) {
    printf("%s: out of memory\n", __func__);
    evh.etSysId = nullptr;
    return ET_ERROR_NOMEM;
  }

  evh.etSysId = etSysId; // indicates successful initialization

  return ET_OK;
}

//______________________________________________________________________________
int THaEtClient::evetClose( evetHandle_t& evh )
{
  // This function always succeeds. Any errors print a message and continue.

  // Close up any currently opened evBufferOpen's.
  int status = ET_OK;
  if( evh.currentChunkStat.evioHandle ) {
    status = evClose(evh.currentChunkStat.evioHandle);
    if( status != S_SUCCESS ) {
      printf("%s: WARNING: evClose returned %s\n", __func__, evPerror(status));
      //return status;  // EVIO return code
    }
  }

  // put any events we may still have
  if( evh.etSysId && evh.etChunkNumRead != -1 ) {
    /* putting array of events */
    status = et_events_put(evh.etSysId, evh.etAttId, evh.etChunk,
                           evh.etChunkNumRead);
    if( status != ET_OK ) {
      printf("%s: WARNING: et_events_put returned %s\n",
             __func__, et_perror(status));
      //return status;
    }
  }

  // free up the etChunk memory
  free(evh.etChunk); // NOLINT(*-multi-level-implicit-pointer-conversion)
  evh.etChunk = nullptr;

  evh.etSysId = nullptr;  // evetHandle_t now uninitialized

  return ET_OK;
}

//______________________________________________________________________________
int THaEtClient::evetGetEtChunks( evetHandle_t& evh )
{
  if( evh.verbose > 1 )
    printf("%s: enter\n", __func__);

  EVETCHECKINIT(evh)

  int status = ET_OK;
  if( evh.mode == 0 ) {
    status = et_events_get(evh.etSysId, evh.etAttId, evh.etChunk, ET_SLEEP,
                           nullptr, evh.etChunkSize, &evh.etChunkNumRead);
  } else {
    struct timespec twait{};
    twait.tv_sec = evh.timeout;
    twait.tv_nsec = 0;
    status = et_events_get(evh.etSysId, evh.etAttId, evh.etChunk, ET_TIMED,
                           &twait, evh.etChunkSize, &evh.etChunkNumRead);
  }
  if( status != ET_OK ) {
    printf("%s: ERROR: et_events_get returned (%d) %s\n",
           __func__, status, et_perror(status));
    if( status == ET_ERROR_TIMEOUT ) {
      printf("et_netclient: timeout calling et_events_get\n");
      printf("Probably means CODA is not running...\n");
    }
    return status;
  }

  evh.currentChunkID = -1;

  return status;
}

//______________________________________________________________________________
int THaEtClient::evetGetChunk( evetHandle_t& evh )
{
  if( evh.verbose > 1 )
    printf("%s: enter\n", __func__);

  EVETCHECKINIT(evh)

  evh.currentChunkID++;

  if( (evh.currentChunkID >= evh.etChunkNumRead) || (evh.etChunkNumRead == -1) ) {
    if( evh.etChunkNumRead != -1 ) {
      /* putting array of events */
      int status = et_events_put(evh.etSysId, evh.etAttId, evh.etChunk,
                                 evh.etChunkNumRead);
      if( status != ET_OK ) {
        printf("%s: ERROR: et_events_put returned %s\n",
               __func__, et_perror(status));
        return status;
      }
    }

    // out of chunks.  get some more
    int status = evetGetEtChunks(evh);
    if( status != ET_OK ) {
      printf("%s: ERROR: evetGetEtChunks(evh) returned %d\n", __func__, status);
      return status;
    }
    evh.currentChunkID++;

  }

  // Close previous handle
  if( evh.currentChunkStat.evioHandle ) {
    int status = evClose(evh.currentChunkStat.evioHandle);
    if( status != S_SUCCESS ) {
      printf("%s: ERROR: evClose returned %s\n", __func__, evPerror(status));
      return status;  // EVIO return code
    }
  }

  auto* currentChunk = evh.etChunk[evh.currentChunkID];
  et_event_getdata(currentChunk, (void**)&evh.currentChunkStat.data);
  et_event_getlength(currentChunk, &evh.currentChunkStat.length);
  et_event_getendian(currentChunk, &evh.currentChunkStat.endian);
  et_event_needtoswap(currentChunk, &evh.currentChunkStat.swap);

  if( evh.verbose > 1 ) {
    auto* data = evh.currentChunkStat.data;
    auto len = evh.currentChunkStat.length;

    printf("data byte order = %s\n",
           (evh.currentChunkStat.endian == ET_ENDIAN_BIG) ? "BIG" : "LITTLE");
    printf(" %2d/%2d: data (len = %d) %s  int = %d\n",
           evh.currentChunkID, evh.etChunkNumRead,
           (int)len,
           evh.currentChunkStat.swap ? "needs swapping" : "does not need swapping",
           evh.currentChunkStat.swap ? bswap_32(data[0]) : data[0]);

    for( uint32_t idata = 0; idata < ((32 < (len >> 2)) ? 32 : (len >> 2)); idata++ ) {
      printf("0x%08x ", evh.currentChunkStat.swap ? bswap_32(data[idata]) : data[idata]);
      if( ((idata + 1) % 8) == 0 )
        printf("\n");
    }
    printf("\n");
  }

  int status = evOpenBuffer((char*)evh.currentChunkStat.data,
                            evh.currentChunkStat.length,
                            (char*)"r", &evh.currentChunkStat.evioHandle);

  if( status != S_SUCCESS ) {
    printf("%s: ERROR: evOpenBuffer returned %s\n",
           __func__, evPerror(status));
    return status;  // EVIO return code
  }

  return status;
}

//______________________________________________________________________________
int THaEtClient::evetReadNoCopy( evetHandle_t& evh,
                                 const uint32_t** outputBuffer,
                                 uint32_t* length )
{
  if( evh.verbose > 1 )
    printf("%s: enter\n", __func__);

  EVETCHECKINIT(evh)

  int status = evReadNoCopy(evh.currentChunkStat.evioHandle,
                            outputBuffer, length);
  if( status != S_SUCCESS ) {
    // Get a new chunk from et_get_event
    status = evetGetChunk(evh);
    if( status == ET_OK ) {
      status = evReadNoCopy(evh.currentChunkStat.evioHandle,
                            outputBuffer, length);
    } else {
      printf("%s: ERROR: evetGetChunk failed %d\n", __func__, status);
    }
  }

  return status; // EVIO return code
}

//______________________________________________________________________________
} // end namespace Decoder

ClassImp(Decoder::THaEtClient)
