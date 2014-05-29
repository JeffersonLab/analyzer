#ifndef ToyEvtTypeHandler_
#define ToyEvtTypeHandler_

/////////////////////////////////////////////////////////////////////
//
//   NEW STUFF (= the entire class)
//   ToyEvtTypeHandler
//   Base class to handle different types of events.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "TNamed.h"
#include "Rtypes.h"
#include "DecoderGlobals.h"
#include "THaAnalysisObject.h"
#include "TDatime.h"

class THaCrateMap;
class THaEvData;

using namespace std;

class ToyEvtTypeHandler : public THaAnalysisObject {

public:

   ToyEvtTypeHandler(const char*, const char*);
   virtual ~ToyEvtTypeHandler();  

   virtual Int_t Analyze(THaEvData *evdata); 
   virtual EStatus Init( const TDatime& run_time );
   virtual void Print();
   virtual Bool_t IsMyEvent(Int_t evnum); 

protected:
 
   vector<Int_t> eventtypes;
   virtual void MakePrefix();

private:


   ToyEvtTypeHandler(const ToyEvtTypeHandler &fh);
   ToyEvtTypeHandler& operator=(const ToyEvtTypeHandler &fh);

   ClassDef(ToyEvtTypeHandler,0)  // Event type handler

};

#endif
