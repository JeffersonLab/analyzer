#ifndef ToyCodaDecoder_
#define ToyCodaDecoder_

/////////////////////////////////////////////////////////////////////
//
//   ToyCodaDecoder
//           to become the new THaCodaDecoder
//
/////////////////////////////////////////////////////////////////////

#define MAX_EVTYPES 200
#define MAX_PHYS_EVTYPES 14

#include "TObject.h"
#include "TString.h"
#include "THaSlotData.h"1
#include "TBits.h"
#include "evio.h"
#include "THaEvData.h"
#include <vector>

class ToyEvtTypeHandler;
class ToyCrateDecoder;
class ToyModule;

class ToyCodaDecoder : public THaEvData {
  // public interface is SAME as before
 public:
  ToyCodaDecoder();
  ~ToyCodaDecoder();
 
  virtual Int_t LoadEvent(const Int_t* evbuffer, THaCrateMap* usermap);    

  virtual Int_t GetNslots() { return fNSlotUsed; };
  virtual Int_t FillCrateSlot(Int_t crate, Int_t ipt);

  virtual Int_t GetPrescaleFactor(Int_t trigger) const;
  virtual Int_t GetScaler(const TString& spec, Int_t slot, Int_t chan) const;
  virtual Int_t GetScaler(Int_t roc, Int_t slot, Int_t chan) const;
  //  virtual Int_t GetScaler(Int_t evtype, Int_t roc, Int_t slot, Int_t chan) const;
  
  virtual Bool_t IsLoadedEpics(const char* tag) const;
  virtual Double_t GetEpicsData(const char* tag, Int_t event=0) const;
  virtual Double_t GetEpicsTime(const char* tag, Int_t event=0) const;
  virtual std::string GetEpicsString(const char* tag, Int_t event=0) const;

  virtual void PrintOut() const { dump(buffer); }
  virtual void SetRunTime(ULong64_t tloc);

 protected:

  // NEW STUFF

  int init_slotdata(const THaCrateMap *map);

  void InitHandlers();

  std::vector<ToyEvtTypeHandler *> event_handler;

  std::vector<Int_t> evidx;
  // end, NEW STUFF


  Int_t   psfact[MAX_PSFACT];

  static void dump(const Int_t* evbuffer);

  ClassDef(ToyCodaDecoder,0) // Decoder for CODA event buffer
};

#endif
