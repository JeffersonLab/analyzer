#ifndef Podd_THaEpicsEvtHandler_h_
#define Podd_THaEpicsEvtHandler_h_

/////////////////////////////////////////////////////////////////////
//
//   THaEpicsEvtHandler
//   Class to handle Hall A EPICS events (event type 131)
//   It makes use of the existing THaEpics and EpicsChan
//   classes from the Decoder.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include "THaEpics.h"
#include "TString.h"
#include <string>
#include <vector>
#include <memory>

class THaEpicsEvtHandler : public THaEvtTypeHandler {

public:

   THaEpicsEvtHandler(const char* name, const char* description);
   virtual ~THaEpicsEvtHandler() = default;

   virtual Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time);
   virtual Int_t End( THaRunBase* r=nullptr );
   Bool_t IsLoaded(const char* tag) const;
   Double_t GetData( const char* tag, UInt_t event = 0 ) const;
   time_t GetTime( const char* tag, UInt_t event = 0 ) const;
   TString GetString( const char* tag, UInt_t event = 0 ) const;

private:

   std::unique_ptr<Decoder::THaEpics> fEpics;
   static const Int_t MAXDATA=20000;

   THaEpicsEvtHandler(const THaEpicsEvtHandler& fh);
   THaEpicsEvtHandler& operator=(const THaEpicsEvtHandler& fh);

   ClassDef(THaEpicsEvtHandler,0)  // EPICS Event handler

};

#endif
