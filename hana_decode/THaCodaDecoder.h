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
#include "evio.h"
#include "THaEvData.h"

class THaCodaDecoder : public THaEvData {
 public:
  THaCodaDecoder();
  ~THaCodaDecoder();
// Loads CODA data evbuffer using THaCrateMap passed as 2nd arg
  virtual int LoadEvent(const int* evbuffer, THaCrateMap* usermap);    

  void PrintOut() const { dump(buffer); }

  int GetPrescaleFactor(int trigger) const; //Obtain prescale factor

  int GetScaler(const TString& spec, int slot, int chan) const;
  int GetScaler(int roc, int slot, int chan) const;

  Bool_t IsLoadedEpics(const char* tag) const;
// EPICS data which is nearest CODA event# 'event'.  Tag is EPICS variable, e.g. 'IPM1H04B.XPOS'
  double GetEpicsData(const char* tag, int event=0) const;
  double GetEpicsTime(const char* tag, int event=0) const;
  std::string GetEpicsString(const char* tag, int event=0) const;

  virtual void SetRunTime(UInt_t tloc);

 protected:
  THaFastBusWord* fb;

  THaEpics* epics; // KCR: Yes, epics is done by us, not THaEvData.

  bool first_scaler;
  TString scalerdef[MAXROC];
  int numscaler_crate;
  int scaler_crate[MAXROC];        // stored from cratemap for fast ref.

  int psfact[MAX_PSFACT];

  // Hall A Trigger Types
  Int_t synchflag,datascan;
  //Double_t dhel,dtimestamp;
  bool buffmode,synchmiss,synchextra;

  void dump(const int* evbuffer) const;
  int gendecode(const int* evbuffer, THaCrateMap* map);

  int loadFlag(const int* evbuffer);

  int epics_decode(const int* evbuffer);
  int prescale_decode(const int* evbuffer);
  int physics_decode(const int* evbuffer, THaCrateMap* map);
  int fastbus_decode(int roc, THaCrateMap* map, const int* evbuffer, 
		     int p1, int p2);
  int vme_decode(int roc, THaCrateMap* map, const int* evbuffer, 
		 int p1, int p2);
  int camac_decode(int roc, THaCrateMap* map, const int* evbuffer, 
		   int p1, int p2);
  int scaler_event_decode(const int* evbuffer, THaCrateMap* map);

  ClassDef(THaCodaDecoder,0) // Decoder for CODA event buffer
};

#endif
