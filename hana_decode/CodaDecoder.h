#ifndef CodaDecoder_
#define CodaDecoder_

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

namespace Decoder {

class CodaDecoder : public THaEvData {
public:
  CodaDecoder();
  ~CodaDecoder();

  virtual Int_t LoadEvent(const UInt_t* evbuffer);
  virtual Int_t LoadFromMultiBlock();
  virtual Int_t LoadIfFlagData(const UInt_t* evbuffer);

  virtual Int_t GetPrescaleFactor(Int_t trigger) const;
  virtual void  SetRunTime(ULong64_t tloc);

  Int_t FindRocs(const UInt_t *evbuffer);
  Int_t roc_decode( Int_t roc, const UInt_t* evbuffer, Int_t ipt, Int_t istop );
  Int_t bank_decode( Int_t roc, const UInt_t* evbuffer, Int_t ipt, Int_t istop );

protected:
  //  Int_t   synchflag; // unused
  //  Bool_t  buffmode,synchmiss,synchextra; // already defined in base class

  Int_t nroc;
  std::vector<Int_t> irn;
  std::vector<bool>  fbfound;
  std::vector<Int_t> psfact;

  void CompareRocs();
  void ChkFbSlot( Int_t roc, const UInt_t* evbuffer, Int_t ipt, Int_t istop );
  void ChkFbSlots();

  virtual Int_t init_slotdata();
  Int_t prescale_decode(const UInt_t* evbuffer);
  void dump(const UInt_t* evbuffer) const;

  ClassDef(CodaDecoder,0) // Decoder for CODA event buffer
};

}

#endif
