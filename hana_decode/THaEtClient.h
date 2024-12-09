#ifndef Podd_THaEtClient_h_
#define Podd_THaEtClient_h_

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

#include "THaCodaData.h"
#include "et.h"
#include <ctime>
#include <string>
#include <memory>

// The ET memory file will have this prefix.  The suffix is $SESSION.
static const char* const ETMEM_PREFIX = "/tmp/et_sys_";
static constexpr int32_t ET_CHUNK_SIZE = 50;

namespace Decoder {

class THaEtClient : public THaCodaData {

public:

  // By default, gets data from localhost
  explicit THaEtClient( Int_t mode = 1 );
  // find data on 'computer'.  e.g. computer="129.57.164.44"
  explicit THaEtClient( const char* computer, Int_t mode = 1 );
  THaEtClient( const THaEtClient& fn ) = delete;
  THaEtClient& operator=( const THaEtClient& fn ) = delete;
  THaEtClient( const char* computer, const char* session, Int_t mode = 1 );
  ~THaEtClient() override;

  Int_t codaOpen( const char* computer, Int_t mode = 1 ) override;
  Int_t codaOpen( const char* computer, const char* session, Int_t mode = 1 ) override;
  Int_t codaClose() override;
  Int_t codaRead() override;    // codaRead() must be called once per event
  Bool_t isOpen() const override;

private:
  Int_t nread{0};
  Int_t nused{0};
  int32_t waitflag{0};
  bool opened{false};
  std::string daqhost, session, etfile, station;
  Int_t init( const char* computer = "hana_sta" );

  // rate calculation
  Int_t firstRateCalc{1};
  Int_t evsum{0};
  Int_t xcnt{0};
  time_t daqt1{-1};
  double ratesum{0.0};

  // Support for ET data de-chunk-ifying.
  // Taken from Bryan Moffit's repo https://github.com/bmoffit/evet
  struct etChunkStat_t {
    uint32_t*     data{nullptr};
    size_t        length{0};
    int32_t       endian{0};
    int32_t       swap{0};
    int32_t       evioHandle{0};
  };

  class EvET {
  public:
    EvET() = default;
    ~EvET() { close(); }
    // These return either ET or EVIO return codes, which are distinct.
    int init( et_sys_id id, int32_t chunksz, int32_t waitmode );
    int close();
    int read_no_copy( const uint32_t** outputBuffer, uint32_t* length );

    et_sys_id     etSysId{nullptr};
    et_att_id     etAttId{};
    int32_t       etChunkSize{};       // user requested (et_events in a chunk)
    int32_t       etChunkNumRead{-1};  // actual read from et_events_get
    int32_t       currentChunkID{-1};  // j
    etChunkStat_t currentChunkStat{};  // data, len, endian, swap
    int32_t       timeout{20};         // timeout value (s)
    int16_t       mode{1};             // wait mode: 0 (indefinite), 1 (timeout)
    int16_t       verbose{1};          // 0 (none), 1 (data rate), 2+ (verbose)
    std::unique_ptr<et_event*[]> etChunk;// pointer to array of et_events (pe)

  private:
    int get_chunk();
    int get_chunks();
    void print_chunk() const;
  };

  EvET evh;

  ClassDefOverride(THaEtClient, 0)   // ET client connection for online data
};

}

#endif
