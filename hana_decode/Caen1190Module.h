#ifndef Podd_Caen1190Module_h_
#define Podd_Caen1190Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Caen1190Module
//
/////////////////////////////////////////////////////////////////////


#include "PipeliningModule.h"
#include <cstring>  // for memset
#include <vector>
#include <string>

namespace Decoder {

class Caen1190Module : public PipeliningModule {

public:

  Caen1190Module() : fSlotData(nullptr), fEvBuf(nullptr), fNfill(0) {}
  Caen1190Module( Int_t crate, Int_t slot );
  virtual ~Caen1190Module() = default;

  using VmeModule::GetData;
  using VmeModule::Init;

  virtual void Init();
  virtual void Clear( Option_t* opt = "" );
  virtual Int_t Decode( const UInt_t* p );
  virtual UInt_t GetData( UInt_t chan, UInt_t hit ) const;
  virtual UInt_t GetOpt( UInt_t chan, UInt_t hit ) const;

  // Loads slot data
  virtual UInt_t LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                           const UInt_t* pstop );
  // Loads slot data for bank structures
  virtual UInt_t LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                           UInt_t pos, UInt_t len );
  // Loads slot data for bank structures with support for event blocking
  virtual UInt_t LoadBank( THaSlotData* sldat, const UInt_t* evbuffer,
                           UInt_t pos, UInt_t len );

private:
  virtual UInt_t LoadNextEvBuffer( THaSlotData* sldat );
  std::string Here( const char* function );

  enum EDataType {
    kTDCData = 0, kTDCHeader = 1, kTDCTrailer = 3, kTDCError = 4,
    kGlobalHeader = 8, kGlobalTrailer = 16, kTriggerTime = 17, kFiller = 24
  };
  static Long64_t Find1190Word( const uint32_t* buf, size_t start, size_t len,
                                EDataType type, uint32_t slot );

  std::vector<UInt_t> fNumHits;
  std::vector<UInt_t> fTdcData; // Raw data
  std::vector<UInt_t> fTdcOpt;  // Edge flag =0 Leading edge, = 1 Trailing edge

  THaSlotData*  fSlotData; // Need to fix if multi-threading becomes available
  const UInt_t* fEvBuf;    // Pointer to current event buffer (for multi-block)
  UInt_t        fNfill;    // Number of filler words at end of current bank

  class tdcData {
  public:
    tdcData()
      : glb_hdr_evno(0), glb_hdr_slno(0), glb_trl_wrd_cnt(0), glb_trl_slno(0), glb_trl_status(0), chan(0), raw(0),
        opt(0), flags(0), trig_time(0), chip_nr_hd(0), hdr_chip_id(0), hdr_event_id(0), hdr_bunch_id(0), trl_chip_id(0),
        trl_event_id(0), trl_word_cnt(0), status(0) {}
    void clear() { memset(this, 0, sizeof(tdcData)); }
    UInt_t glb_hdr_evno;
    UInt_t glb_hdr_slno;
    UInt_t glb_trl_wrd_cnt;
    UInt_t glb_trl_slno;
    UInt_t glb_trl_status;
    UInt_t chan;
    UInt_t raw;
    UInt_t opt;
    UInt_t flags;
    UInt_t trig_time;
    UInt_t chip_nr_hd;
    UInt_t hdr_chip_id;
    UInt_t hdr_event_id;
    UInt_t hdr_bunch_id;
    UInt_t trl_chip_id;
    UInt_t trl_event_id;
    UInt_t trl_word_cnt;
    Int_t status;
  } tdc_data;

  static TypeIter_t fgThisType;

  ClassDef(Caen1190Module, 0)  //  CAEN 1190 multi-hit TDC module
};

//_____________________________________________________________________________
inline
Long64_t Caen1190Module::Find1190Word(
  const uint32_t* buf, size_t start, size_t len, EDataType type, uint32_t slot )
{
  // Search 'buf' for CAEN 1190 global header/trailer word for 'slot'.
  // The format is (T=type, S=slot)
  //    TTTT TXXX XXXX XXXX XXXX XXXX XXXS SSSS
  // bit  28   24   20   16   12    8    4    0
  //
  // Supported types:
  //   8 (0x08) Global header
  //  16 (0x10) Global trailer
  //   
  // The buffer is searched between [start,start+len)
  // Returns the offset into 'buf' containing the word, or -1 if not found.

  assert(type == kGlobalHeader or type == kGlobalTrailer);
  const uint32_t ID = (type & 0x1F) << 27 | (slot & 0x1F);
  const auto* p = buf + start;
  const auto* q = p + len;
  while( p != q && (*p & 0xF800001F) != ID )
    ++p;
  return (p != q) ? p - buf : -1;
}

} // namespace Decoder

#endif
