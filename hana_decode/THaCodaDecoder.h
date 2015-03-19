#ifndef THaCodaDecoder_
#define THaCodaDecoder_

/////////////////////////////////////////////////////////////////////
//
//   THaCodaDecoder
//
/////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TString.h"
#include "THaSlotData.h"
#include "TBits.h"
#include "THaEvData.h"
#include "Decoder.h"

namespace Decoder {

class THaCodaDecoder : public THaEvData {
 public:
  THaCodaDecoder();
  ~THaCodaDecoder();
  // Loads CODA data evbuffer
  virtual Int_t LoadEvent(const UInt_t* evbuffer );

  virtual Int_t GetPrescaleFactor(Int_t trigger) const;
  virtual Int_t GetScaler(const TString& spec, Int_t slot, Int_t chan) const;
  virtual Int_t GetScaler(Int_t roc, Int_t slot, Int_t chan) const;

  virtual Bool_t IsLoadedEpics(const char* tag) const;
  virtual Double_t GetEpicsData(const char* tag, Int_t event=0) const;
  virtual Double_t GetEpicsTime(const char* tag, Int_t event=0) const;
  virtual TString GetEpicsString(const char* tag, Int_t event=0) const;

  virtual void PrintOut() const { dump(buffer); }
  virtual void SetRunTime(ULong64_t tloc);

 protected:
  THaFastBusWord* fb;

  THaEpics* epics; // EPICS is done by us, not THaEvData.

  Bool_t  first_scaler;
  TString scalerdef[MAXROC];
  Int_t   numscaler_crate;
  Int_t   scaler_crate[MAXROC];    // stored from cratemap for fast ref.

  Int_t   psfact[MAX_PSFACT];

  // Hall A Trigger Types
  Int_t   synchflag,datascan;

  static void dump(const UInt_t* evbuffer);

  Int_t   gendecode(const UInt_t* evbuffer );

  Int_t   loadFlag(const UInt_t* evbuffer);

  Int_t   epics_decode(const UInt_t* evbuffer);
  Int_t   prescale_decode(const UInt_t* evbuffer);
  Int_t   physics_decode(const UInt_t* evbuffer);
  Int_t   fastbus_decode(Int_t roc, const UInt_t* evbuffer, Int_t p1, Int_t p2);
  Int_t   vme_decode(Int_t roc, const UInt_t* evbuffer, Int_t p1, Int_t p2);
  Int_t   camac_decode(Int_t roc, const UInt_t* evbuffer, Int_t p1, Int_t p2);
  Int_t   scaler_event_decode(const UInt_t* evbuffer );

  ClassDef(THaCodaDecoder,0) // Decoder for CODA event buffer
};

}

#endif
