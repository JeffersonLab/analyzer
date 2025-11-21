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
#include <cstdio>
#include <cstdint>
#include <ctime>        // for timespec
#include <stdexcept>
#include <algorithm>    // std::min
#include "evio.h"
#include "et.h"

#ifdef __cpp_lib_byteswap
#include <bit>
#define bswap_32(x) std::byteswap<uint32_t>(x)
#else
#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#elif HAVE_BUILTIN_BSWAP
#define bswap_32 __builtin_bswap32
#else
#define bswap_32(x) ET_SWAP32(x)
#endif
#endif

using namespace std;

static const int FAST = 25;
static const int SMALL_TIMEOUT = 10;
static const int BIG_TIMEOUT = 20;

namespace Decoder {

//______________________________________________________________________________
THaEtClient::THaEtClient( Int_t smode )
{
  // uses default server (where CODA runs)
  const char* defaultcomputer = "127.0.0.1";
  const char* defaultsession = "halla";
  THaEtClient::codaOpen(defaultcomputer, defaultsession, smode);
}

//______________________________________________________________________________
THaEtClient::THaEtClient( const char* computer, Int_t smode )
{
  if( THaEtClient::codaOpen(computer, smode) != CODA_OK )
    throw std::invalid_argument(
      "THaEtClient: invalid computer or session name");
}

//______________________________________________________________________________
THaEtClient::THaEtClient( const char* computer, const char* mysession, Int_t smode )
{
  if( THaEtClient::codaOpen(computer, mysession, smode) != CODA_OK )
    throw std::invalid_argument(
      "THaEtClient: invalid computer or session name");
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
  if( THaEtClient::codaClose() != CODA_OK )
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

  // Initialize eventHandle evh
  if( evh.init(id, ET_CHUNK_SIZE, waitflag) != ET_OK )
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
  auto* id = evh.etSysId;  // this gets zeroed out in evh.close();
  int status = et_station_detach(id, evh.etAttId);
  if( status != ET_OK ) {
    cerr << "WARNING: codaClose: error detaching from ET: "
         << et_perror(status) << endl;
//    return CODA_ERROR;
  }

  evh.close(); // always succeeds, even if error

  status = et_close(id);
  if( status != ET_OK ) {
    cerr << "WARNING: codaClose: error closing ET: " << et_perror(status)
         << endl;
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
  int status = evh.read_no_copy(&readBuffer, &len);
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
  if( !computer || !*computer ) {
    cerr << "THaEtClient: ERROR: no computer name given" << endl;
    return CODA_ERROR;
  }
  if( !mysession || !*mysession ) {
    cerr << "THaEtClient: ERROR: no session name given" << endl;
    return CODA_ERROR;
  }
  daqhost = computer;
  session = mysession;
  etfile = ETMEM_PREFIX + session;
  waitflag = smode;
  return CODA_OK;
}

//______________________________________________________________________________
Int_t THaEtClient::codaOpen( const char* computer, Int_t smode )
{
  // See comment in the above version of codaOpen()
  const auto* s = gSystem->Getenv("SESSION");
  if( s == nullptr ) {
    cerr << "THaEtClient: ERROR: $SESSION not set" << endl;
    return CODA_ERROR;
  }
  return codaOpen(computer, s, smode);
}

//______________________________________________________________________________
bool THaEtClient::isOpen() const
{
  return opened;
}

//______________________________________________________________________________
#define EVETCHECKINIT                                             \
  if( verbose > 1 )                                               \
    printf("%s: enter\n", __func__);                              \
                                                                  \
  if( etSysId == nullptr || !etChunk ) {                          \
    printf("%s: ERROR: evet not initialized\n", __func__);        \
    return ET_ERROR;                                              \
  }

//______________________________________________________________________________
int THaEtClient::EvET::init( et_sys_id id, int32_t chunksz, int32_t waitmode )
{
  close();

  etSysId = nullptr;
  etAttId = 0;

  etChunkSize = chunksz;
  etChunkNumRead = -1;
  currentChunkID = -1;

  currentChunkStat.data = nullptr;
  currentChunkStat.length = 0;
  currentChunkStat.endian = 0;
  currentChunkStat.swap = 0;
  currentChunkStat.evioHandle = 0;

  timeout = BIG_TIMEOUT;
  mode = static_cast<int16_t>(waitmode);

  /* allocate some memory */
  try {
    etChunk = make_unique<et_event*[]>(chunksz);
  } catch ( const std::bad_alloc& ) {
    printf("%s: out of memory\n", __func__);
    etSysId = nullptr;
    return ET_ERROR_NOMEM;
  }

  etSysId = id;   // indicates successful initialization

  return ET_OK;
}

//______________________________________________________________________________
int THaEtClient::EvET::close()
{
  // This function always succeeds. Any errors print a message and continue.

  // Close up any currently opened evBufferOpen's.
  int status = ET_OK;
  if( currentChunkStat.evioHandle ) {
    status = evClose(currentChunkStat.evioHandle);
    if( status != S_SUCCESS ) {
      printf("%s: WARNING: evClose returned %s\n", __func__, evPerror(status));
      //return status;  // EVIO return code
    }
  }

  // put any events we may still have
  if( etSysId && etChunkNumRead != -1 ) {
    /* putting array of events */
    status = et_events_put(etSysId, etAttId, etChunk.get(), etChunkNumRead);
    if( status != ET_OK ) {
      printf("%s: WARNING: et_events_put returned %s\n",
             __func__, et_perror(status));
      //return status;
    }
  }

  // free up the etChunk memory
  etChunk.reset();

  currentChunkStat.evioHandle = 0;
  etSysId = nullptr;  // this object now considered uninitialized

  return ET_OK;
}

//______________________________________________________________________________
int THaEtClient::EvET::get_chunks()
{
  EVETCHECKINIT

  int status = ET_OK;
  if( mode == 0 ) {
    status = et_events_get(etSysId, etAttId, etChunk.get(), ET_SLEEP,
                           nullptr, etChunkSize, &etChunkNumRead);
  } else {
    struct timespec twait{};
    twait.tv_sec = timeout;
    twait.tv_nsec = 0;
    status = et_events_get(etSysId, etAttId, etChunk.get(), ET_TIMED,
                           &twait, etChunkSize, &etChunkNumRead);
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

  currentChunkID = -1;

  return status;
}

//______________________________________________________________________________
void THaEtClient::EvET::print_chunk() const
{
  uint32_t* data = currentChunkStat.data;
  size_t len = currentChunkStat.length;

  printf("data byte order = %s\n",
         (currentChunkStat.endian == ET_ENDIAN_BIG) ? "BIG" : "LITTLE");
  printf(" %2d/%2d: data (len = %u) %s  int = ",
         currentChunkID, etChunkNumRead, static_cast<unsigned>(len),
         currentChunkStat.swap ? "needs swapping" : "does not need swapping");
  if( data ) {
    printf("%d\n", currentChunkStat.swap ? bswap_32(data[0]) : data[0]);
    for( size_t idata = 0; idata < std::min((size_t)32, len >> 2); idata++ ) {
      printf("0x%08x ", currentChunkStat.swap ? bswap_32(data[idata])
                                              : data[idata]);
      if( ((idata + 1) % 8) == 0 )
        printf("\n");
    }
    printf("\n");
  } else {
    printf("(null)\n%s: ERROR: data = NULL\n", __func__);
  }
}

//______________________________________________________________________________
int THaEtClient::EvET::get_chunk()
{
  EVETCHECKINIT

  currentChunkID++;

  if( currentChunkID >= etChunkNumRead || etChunkNumRead == -1 ) {
    if( etChunkNumRead != -1 ) {
      /* putting array of events */
      int status = et_events_put(etSysId, etAttId, etChunk.get(),
                                 etChunkNumRead);
      if( status != ET_OK ) {
        printf("%s: ERROR: et_events_put returned %s\n",
               __func__, et_perror(status));
        return status;
      }
    }

    // out of chunks.  get some more
    int status = get_chunks();
    if( status != ET_OK ) {
      printf("%s: ERROR: get_chunks returned %d\n", __func__, status);
      return status;
    }
    currentChunkID++;

  }

  // Close previous handle
  if( currentChunkStat.evioHandle ) {
    int status = evClose(currentChunkStat.evioHandle);
    if( status != S_SUCCESS ) {
      printf("%s: ERROR: evClose returned %s\n", __func__, evPerror(status));
      return status;  // EVIO return code
    }
  }

  auto* currentChunk = etChunk[currentChunkID];
  et_event_getdata(currentChunk, (void**)&currentChunkStat.data);
  et_event_getlength(currentChunk, &currentChunkStat.length);
  et_event_getendian(currentChunk, &currentChunkStat.endian);
  et_event_needtoswap(currentChunk, &currentChunkStat.swap);

  if( verbose > 1 )
    print_chunk();

  int status = evOpenBuffer((char*)currentChunkStat.data,
                            currentChunkStat.length,
                            (char*)"r", &currentChunkStat.evioHandle);

  if( status != S_SUCCESS ) {
    printf("%s: ERROR: evOpenBuffer returned %s\n",
           __func__, evPerror(status));
    return status;  // EVIO return code
  }

  return status;
}

//______________________________________________________________________________
int THaEtClient::EvET::read_no_copy( const uint32_t** outputBuffer,
                                     uint32_t* length )
{
  EVETCHECKINIT

  int status = evReadNoCopy(currentChunkStat.evioHandle,
                            outputBuffer, length);
  if( status != S_SUCCESS ) {
    // Get a new chunk from et_get_event
    status = get_chunk();
    if( status == ET_OK ) {
      status = evReadNoCopy(currentChunkStat.evioHandle,
                            outputBuffer, length);
    } else {
      printf("%s: ERROR: get_chunk failed %d\n", __func__, status);
    }
  }

  return status; // EVIO return code
}

//______________________________________________________________________________
} // end namespace Decoder

ClassImp(Decoder::THaEtClient)
