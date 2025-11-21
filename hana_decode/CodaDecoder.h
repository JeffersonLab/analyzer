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

#include "THaEvData.h"
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <cstring> // for memcpy

namespace Decoder {

class CodaDecoder : public THaEvData {
public:
  CodaDecoder();
  virtual ~CodaDecoder();

  virtual Int_t  Init();

  virtual Int_t  LoadEvent(const UInt_t* evbuffer);

  virtual Int_t  GetPrescaleFactor( UInt_t trigger ) const;
  virtual void   SetRunTime( Long64_t tloc );
  virtual Int_t  SetDataVersion( Int_t version );
          Int_t  SetCodaVersion( Int_t version )  { return SetDataVersion(version); }

  virtual Bool_t DataCached() { return fMultiBlockMode && !fBlockIsDone; }
  virtual Int_t  LoadFromMultiBlock();
          Bool_t IsMultiBlockMode() const { return fMultiBlockMode; };
          Bool_t BlockIsDone() const { return fBlockIsDone; };

  virtual Int_t  FillBankData( UInt_t* rdat, UInt_t roc, Int_t bank,
                               UInt_t offset = 0, UInt_t num = 1 ) const;

  UInt_t         GetTSEvType() const { return tsEvType; }
  UInt_t         GetBlockIndex() const { return blkidx; }

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
    BankInfo() : pos_{0}, len_{0}, tag_{0}, otag_{0}, dtyp_{0}, npad_{0},
                 blksz_{0}, status_{kOK} {}
    enum EBankErr  { kOK = 0, kBadArg, kBadLen, kBadPad, kUnsupType };
    enum EDataSize { kUndef = 0, k8bit = 1, k16bit = 2, k32bit = 4, k64bit = 8 };
    enum EIntFloat { kInteger = 0, kFloat, kDouble };
    enum ESigned   { kUnknown = 0, kUnsigned, kSigned };
    explicit operator bool() const { return status_ == kOK && len_ > 0; }
    bool operator!() const { return !((bool)*this); }
    Int_t Fill( const UInt_t* evbuf, UInt_t pos, UInt_t len );
    EDataSize GetDataSize() const; // Size of data in bytes/element
    EIntFloat GetFloat()    const;
    ESigned   GetSigned()   const;
    const char* Errtxt()    const;
    const char* Typtxt()    const;
    UInt_t    pos_;      // First word of payload
    UInt_t    len_;      // pos_ + len_ = first word after payload
    UInt_t    tag_;      // Bank tag
    UInt_t    otag_;     // Bank tag of "outer" bank if bank of banks
    UInt_t    dtyp_;     // Data type
    UInt_t    npad_;     // Number of padding bytes at end of data
    UInt_t    blksz_;    // Block size (multiple events per buffer)
    EBankErr  status_;   // Decoding status
  };
  static BankInfo GetBank( const UInt_t* evbuf, UInt_t pos, UInt_t len ) {
    BankInfo ifo;
    ifo.Fill(evbuf, pos, len);
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
  BankInfo  GetBank( const BankDat_t& bank ) {
    return GetBank( buffer, bank.pos, bank.len );
  }
  BankInfo  GetBank( UInt_t roc, Int_t bank ) {
    auto dat = FindBank(roc, bank);
    if( !dat ) return {};
    return GetBank(dat);
  }

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

  virtual Int_t  init_slotdata();
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
     TBOBJ() : blksize(0), tag(0), nrocs(0), len(0), tsrocLen(0), evtNum(0),
               runInfo(0), start(nullptr), evTS(nullptr), evType(nullptr),
               TSROC(nullptr) {}
     void     Clear() { memset(this, 0, sizeof(*this)); }
     uint32_t Fill( const uint32_t* evbuffer, uint32_t blkSize, uint32_t tsroc );
     bool     withTimeStamp()   const { return (tag & 1) != 0; }
     bool     withRunInfo()     const { return (tag & 2) != 0; }
     bool     withTriggerBits() const { return (tsrocLen > 2*blksize);}
     uint64_t GetEvTS( size_t i ) {
       uint64_t ts{0};
       if( evTS && i < blksize )
         memcpy(&ts, evTS + 2 * i, sizeof(ts));
       return ts;
     }
     uint32_t blksize;          /* total number of triggers in the Bank */
     uint16_t tag;              /* Trigger Bank Tag ID = 0xff2x */
     uint16_t nrocs;            /* Number of ROC Banks in the Event Block (val = 1-256) */
     uint32_t len;              /* Total Length of the Trigger Bank - including Bank header */
     uint32_t tsrocLen;         /* Number of words in TSROC array */
     uint64_t evtNum;           /* Starting Event # of the Block */
     uint64_t runInfo;          /* Run Info Data (optional) */
     const uint32_t *start;     /* Pointer to start of the Trigger Bank */
     const uint32_t *evTS;      /* Pointer to array of 64-bit timestamps (optional) */
     const uint16_t *evType;    /* Pointer to array of Event Types */
     const uint32_t *TSROC;     /* Pointer to Trigger Supervisor ROC segment data */
   };

protected:
   Int_t LoadTrigBankInfo( UInt_t index_buffer );
   TBOBJ tbank;

   ClassDef(CodaDecoder,0) // Decoder for CODA event buffer
};

}

#endif
