#ifndef Podd_CodaDecoder_h_
#define Podd_CodaDecoder_h_

/////////////////////////////////////////////////////////////////////
//
//   CodaDecoder
//
//           Object Oriented version of decoder
//           Sept, 2014    R. Michaels
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"
#include <vector>
#include <cstdint>

namespace Decoder {

class CodaDecoder : public THaEvData {
public:
  CodaDecoder();
  virtual ~CodaDecoder();

  virtual Int_t  LoadEvent(const UInt_t* evbuffer);
  virtual Int_t  LoadFromMultiBlock();

  virtual UInt_t GetPrescaleFactor( UInt_t trigger ) const;
  virtual void   SetRunTime( ULong64_t tloc );
  virtual Int_t  SetDataVersion( Int_t version ) { return SetCodaVersion(version); }
          Int_t  SetCodaVersion( Int_t version );

  enum { MAX_PSFACT = 12 };

protected:
  virtual Int_t  LoadIfFlagData(const UInt_t* evbuffer);

  void FillBankData( UInt_t* rdat, UInt_t roc, Int_t bank, UInt_t offset = 0,
                     UInt_t num = 1 ) const;

  Int_t FindRocs(const UInt_t *evbuffer);  // CODA2 version
  Int_t FindRocsCoda3(const UInt_t *evbuffer); // CODA3 version
  Int_t roc_decode( UInt_t roc, const UInt_t* evbuffer, UInt_t ipt, UInt_t istop );
  Int_t bank_decode( UInt_t roc, const UInt_t* evbuffer, UInt_t ipt, UInt_t istop );

  void CompareRocs();
  void ChkFbSlot( UInt_t roc, const UInt_t* evbuffer, UInt_t ipt, UInt_t istop );
  void ChkFbSlots();

  virtual Int_t  init_slotdata();
  virtual Int_t  interpretCoda3( const UInt_t* buffer );
  virtual UInt_t trigBankDecode( const UInt_t* evbuffer, UInt_t blkSize );
  Int_t prescale_decode( const UInt_t* evbuffer );
  void  dump( const UInt_t* evbuffer ) const;

  // Data
  //  Int_t   synchflag; // unused
  //  Bool_t  buffmode,synchmiss,synchextra; // already defined in base class

  UInt_t nroc;
  std::vector<UInt_t> irn;
  std::vector<bool>   fbfound;
  std::vector<UInt_t> psfact;
  Bool_t fdfirst;
  Int_t  chkfbstat;

  // CODA3 stuff
  UInt_t evcnt_coda3;

  class TBOBJ {
  public:
     TBOBJ() : blksize(0), tag(0), nrocs(0), len(0), withTimeStamp(0),
         withRunInfo(0), evtNum(0), runInfo(0), start(nullptr), evTS(nullptr), evType(nullptr) {}
     uint32_t blksize;              /* total number of triggers in the Bank */
     uint16_t tag;                  /* Trigger Bank Tag ID = 0xff2x */
     uint16_t nrocs;                /* Number of ROC Banks in the Event Block (val = 1-256) */
     uint32_t len;                  /* Total Length of the Trigger Bank - including Bank header */
     int      withTimeStamp;        /* =1 if Time Stamps are available */
     int      withRunInfo;          /* =1 if Run information is available - Run # and Run Type */
     uint64_t evtNum;               /* Starting Event # of the Block */
     uint64_t runInfo;              /* Run Info Data */
     uint32_t *start;               /* Pointer to start of the Trigger Bank */
     uint64_t *evTS;                /* Pointer to the array of Time Stamps */
     uint16_t *evType;              /* Pointer to the array of Event Types */
   };

  TBOBJ tbank;

  ClassDef(CodaDecoder,0) // Decoder for CODA event buffer
};

}

#endif
