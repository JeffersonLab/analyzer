#ifndef Podd_CodaDecoder_h_
#define Podd_CodaDecoder_h_

/////////////////////////////////////////////////////////////////////
//
//   CodaDecoder
//
//           Object-Oriented version of decoder
//           Sept, 2014    R. Michaels
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"  // for THaEvData, Rtypes, MAX_PSFACT
#include <array>        // for array
#include <cassert>      // for assert
#include <cstdint>      // for uint32_t, uint64_t, uint16_t, uint8_t
#include <cstring>      // for size_t, memcpy, memset
#include <stdexcept>    // for runtime_error
#include <string>       // for string
#include <vector>       // for vector

namespace Decoder {

class CodaDecoder : public THaEvData {
public:
  CodaDecoder();
  ~CodaDecoder() override;

  Int_t  Init() override;

  Int_t  LoadEvent(const UInt_t* evbuffer) override;

  Int_t  GetPrescaleFactor( UInt_t trigger ) const override;
  void   SetRunTime( Long64_t tloc ) override;
  Int_t  SetDataVersion( Int_t version ) override;
  Int_t  SetCodaVersion( Int_t version )  { return SetDataVersion(version); }

  Bool_t DataCached() override { return fMultiBlockMode && !fBlockIsDone; }
  Bool_t IsMultiBlockMode() const { return fMultiBlockMode; }
  Bool_t BlockIsDone() const { return fBlockIsDone; }

  virtual Int_t LoadFromMultiBlock();
  virtual Int_t FillBankData( UInt_t* rdat, UInt_t roc, Int_t bank,
                              UInt_t offset = 0, UInt_t num = 1 ) const;

  UInt_t GetTSEvType() const { return tsEvType; }
  UInt_t GetBlockIndex() const { return blkidx; }

  // CODA file format exception, thrown by LoadEvent/LoadFromMultiBlock
  class coda_format_error : public std::runtime_error {
  public:
    explicit coda_format_error( const std::string& what_arg )
      : std::runtime_error(what_arg) {}
    explicit coda_format_error( const char* what_arg )
      : std::runtime_error(what_arg) {}
  };

  static UInt_t InterpretBankTag( UInt_t tag );

  struct BankInfo {
    enum EBankErr  : Byte_t { kOK = 0, kBadArg, kBadLen, kBadPad, kUnsupType };
    enum EDataType : Byte_t {
      kUnknown = 0, kUInt = 1, kFloat = 2, kChar = 3,
      kShort = 4, kUShort = 5, kInt8 = 6, kUInt8 = 7, kDouble = 8, kLong = 9,
      kULong = 0xA, kInt = 0xB, kTagSegment = 0xC, kAltSegment = 0xD,
      kAltBank = 0xE, kComposite = 0xF, kBank = 0x10, kSegment = 0x20
    };
    UInt_t    pos_;      // First word of payload
    UInt_t    len_;      // pos_ + len_ = first word after payload
    UShort_t  tag_;      // Bank tag
    UShort_t  otag_;     // Bank tag of "outer" bank if bank of banks
    Byte_t    dtyp_;     // Data type
    Byte_t    num_;      // Application-specific counter (e.g. block size)
    Byte_t    npad_;     // Number of padding bytes at end of data
    EBankErr  status_;   // Decoding status

    BankInfo() : pos_{0}, len_{0}, tag_{0}, otag_{0}, dtyp_{0}, num_{0},
                 npad_{0}, status_{kOK} {}
    // Decode bank header, including nested banks
    Int_t  Fill( const UInt_t* evbuf, UInt_t pos, UInt_t len, bool recurse = true );
    // Decode segment/tag segment header
    Int_t  FillSegment( const UInt_t* evbuf, UInt_t pos, UInt_t len, UInt_t dtype );

    explicit operator bool() const {
      return status_ == kOK && (IsContainer() || len_ > 0);
    }
    bool operator!() const { return !((bool)*this); }

    Bool_t IsBank()      const { return dtyp_ == kBank || dtyp_ == kAltBank; }
    Bool_t IsComposite() const { return dtyp_ == kComposite; }
    Bool_t IsSegment()   const { return dtyp_ == kSegment ||
      dtyp_ == kTagSegment || dtyp_ == kAltSegment; }
    Bool_t IsContainer() const { return IsBank() or IsSegment() or IsComposite(); }
    const char* Errtxt() const;
    const char* Typtxt() const;

    size_t GetDataSize() const {
      // Size of data in bytes/element
      switch( dtyp_ ) {
        case kLong:  case kULong: case kDouble:
          return 8;
        case kUInt:  case kFloat: case kInt: case kUnknown:
          return 4;
        case kShort: case kUShort:
          return 2;
        case kChar:  case kInt8:  case kUInt8:
          return 1;
        default:
          return 0;
      }
    }
    UInt_t GetDataType() const { return dtyp_; }
  };
  static BankInfo GetBank( const UInt_t* evbuf, UInt_t pos, UInt_t len,
                           bool recurse = true) {
    BankInfo ifo;
    ifo.Fill(evbuf, pos, len, recurse);
    return ifo;
  }
  static BankInfo GetSegment( const UInt_t* evbuf, UInt_t pos, UInt_t len,
                              UInt_t dtype ) {
    BankInfo ifo;
    ifo.FillSegment(evbuf, pos, len, dtype);
    return ifo;
  }

  class BankDat_t {            // Coordinates of bank data in raw event
  public:
    BankDat_t() : key(kMaxUInt), pos(0), len(0) {}
    BankDat_t( UInt_t key, UInt_t pos, UInt_t len ) : key(key), pos(pos), len(len) {}
    bool operator==( const BankDat_t& rhs ) const { return key == rhs.key; }
    bool operator==( UInt_t key_to_find )   const { return key == key_to_find; }
    bool operator< ( const BankDat_t& rhs ) const { return key <  rhs.key; }
    explicit operator bool() const { return key != kMaxUInt; }
    bool operator!() const { return key == kMaxUInt; }
    UInt_t key;   // bank number + (roc << 16)
    UInt_t pos;   // position in evbuffer[]
    UInt_t len;   // length of data including pos, so pos+len-1 = last word of data
  };

  // Methods to find position, length, and parameters of a CODA bank in
  // the current event buffer. Must call LoadEvent first.
  BankDat_t FindBank( UInt_t roc, Int_t bank ) const;
  BankInfo  GetBank( const BankDat_t& bank ) const {
    if(bank.pos < 2) return {};
    return GetBank( buffer, bank.pos-2, bank.len+2 );
  }
  BankInfo  GetBank( UInt_t roc, Int_t bank ) const {
    auto dat = FindBank(roc, bank);
    if( !dat ) return {};
    return GetBank(dat);
  }

  // Iterator over payload data contained in a bank or subdivisions thereof,
  // such as other banks or segments. Dereferencing the iterator returns a
  // BankInfo object that contains the parameters of the current payload chunk.
  class BankDataIterator {
  public:
    BankDataIterator( const UInt_t* evbuf, UInt_t pos, UInt_t len )
      : evbuf_(evbuf), startpos_(pos), len_(len), endpos_(0) { reset(); }
    BankDataIterator( const UInt_t* evbuf, const BankDat_t& bankdat )
      : BankDataIterator( evbuf, bankdat.pos, bankdat.len ) {}
    BankDataIterator() = delete;

    // Positioning
    BankDataIterator& operator++();
    BankDataIterator operator++(int) &
      { BankDataIterator clone(*this); ++*this; return clone; }
    void reset();

    // Status
    explicit operator bool() const { return current().pos_ < endpos_; }
    bool operator!() const { return !static_cast<bool>(*this); }

    // Current value
    const BankInfo* operator->() const { return &current(); }
    const BankInfo& operator* () const { return current(); }
    const BankInfo& header() const { return banks_.at(0); }
    const UInt_t*   ptr()    const { return evbuf_ + current().pos_; }

  private:
    const UInt_t* const    evbuf_;
    const UInt_t           startpos_;
    const UInt_t           len_;
    UInt_t                 endpos_;
    std::vector<BankInfo>  banks_;

    const BankInfo& current() const {
      assert(!banks_.empty());
      return banks_.back();
    }
    void parse_container_header( UInt_t nextpos );
  };

protected:
  virtual Int_t  LoadIfFlagData(const UInt_t* evbuffer);

  Int_t FindRocs(const UInt_t *evbuffer);  // CODA2 version
  Int_t FindRocsCoda3(const UInt_t *evbuffer); // CODA3 version
  Int_t roc_decode( UInt_t roc, const UInt_t* evbuffer, UInt_t ipt, UInt_t istop );
  Int_t bank_decode( UInt_t roc, const UInt_t* evbuffer, UInt_t ipt, UInt_t istop );
  Int_t physics_decode( const UInt_t* evbuffer );

  void CompareRocs();
  void ChkFbSlot( UInt_t roc, const UInt_t* evbuffer, UInt_t ipt, UInt_t istop );
  void ChkFbSlots();

  Int_t  init_slotdata() override;
  virtual Int_t  interpretCoda3( const UInt_t* buffer );
  virtual Int_t  trigBankDecode( const UInt_t* evbuffer );
  virtual Int_t  daqConfigDecode( const UInt_t* evbuf );
  Int_t prescale_decode_coda2( const UInt_t* evbuffer );
  Int_t prescale_decode_coda3( const UInt_t* evbuffer );
  void  dump( const UInt_t* evbuffer ) const;
  void  debug_print( const UInt_t* evbuffer ) const;
  void  PrintBankInfo() const;

  // Data
  UInt_t nroc;
  std::vector<UInt_t> irn;
  std::vector<bool>   fbfound;
  std::array<Int_t, MAX_PSFACT> psfact;
  Bool_t buffmode,synchmiss,synchextra,fdfirst;
  Int_t  chkfbstat;

  std::vector<BankDat_t> bankdat;
  BankDat_t* CheckForBank( UInt_t roc, UInt_t slot );

  // CODA3 stuff
  UInt_t blkidx;  // Event block index (0 <= blkidx < block_size)
  Bool_t fMultiBlockMode, fBlockIsDone;
  UInt_t tsEvType, bank_tag, block_size;

public:
  class TBOBJ {
  public:
     TBOBJ() : len(0), tag(0), blksize(0), nrocs(0), EBid(0), tsrocLen(0),
               evtNum(0), runInfo(0), evTS(nullptr), evType(nullptr),
               TSROC(nullptr) {}
     void     Clear() { memset(this, 0, sizeof(*this)); }
     uint32_t Fill( const uint32_t* evbuffer, uint32_t blkSize, uint32_t tsroc );
     bool     withTimeStamp()   const { return (tag & 1) != 0; }
     bool     withRunInfo()     const { return (tag & 2) != 0; }
     bool     withRunData()     const { return (tag & 4) == 0; } // = trigger bits?
     bool     withTriggerBits() const { return (tsrocLen > 2U*blksize);}
     uint64_t GetEvTS( size_t i ) const {
       uint64_t ts{0};
       if( evTS && i < blksize )
         memcpy(&ts, evTS + 2 * i, sizeof(ts));
       return ts;
     }
     uint32_t len;              /* Total Length of the Trigger Bank - including Bank header */
     uint16_t tag;              /* Trigger Bank Tag ID = 0xff2x */
     uint8_t  blksize;          /* total number of triggers in the Bank (1-255) */
     uint8_t  nrocs;            /* Number of ROC Banks in the Event Block (1-255) */
     uint16_t EBid;             /* Event builder ID (12 bits) */
     uint32_t tsrocLen;         /* Number of words in TSROC array */
     uint64_t evtNum;           /* Starting Event # of the Block */
     uint64_t runInfo;          /* Run Info Data (optional) */
     const uint32_t *evTS;      /* Pointer to array of 64-bit timestamps (optional) */
     const uint16_t *evType;    /* Pointer to array of Event Types */
     const uint32_t *TSROC;     /* Pointer to Trigger Supervisor ROC segment data */
   };

protected:
   Int_t LoadTrigBankInfo( UInt_t index_buffer );
   TBOBJ tbank;

   ClassDefOverride(CodaDecoder,0) // Decoder for CODA event buffer
};

}

#endif
