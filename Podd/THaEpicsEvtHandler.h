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
#include "Decoder.h"
#include <string>
#include <vector>
#include "TTree.h"
#include "TString.h"  

class THaEpicsEvtHandler : public THaEvtTypeHandler {

public:

   THaEpicsEvtHandler(const char* name, const char* description);
   virtual ~THaEpicsEvtHandler();

   virtual Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time);
   virtual Int_t End( THaRunBase* r=0 );
   Bool_t IsLoaded(const char* tag) const; 
   Double_t GetData(const char* tag, Int_t event=0) const;  
   Double_t GetTime(const char* tag, Int_t event=0) const; 
   TString GetString (const char* tag, int event=0) const;

private:

   Decoder::THaEpics *fEpics;
   static const Int_t MAXDATA=20000;

   THaEpicsEvtHandler(const THaEpicsEvtHandler& fh);
   THaEpicsEvtHandler& operator=(const THaEpicsEvtHandler& fh);

   ClassDef(THaEpicsEvtHandler,0)  // EPICS Event handler

};

#endif
