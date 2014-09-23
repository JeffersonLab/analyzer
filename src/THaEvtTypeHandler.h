#ifndef THaEvtTypeHandler_
#define THaEvtTypeHandler_

/////////////////////////////////////////////////////////////////////
//
//   THaEvtTypeHandler
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
#include "Decoder.h"
#include "THaEvData.h"

using namespace Decoder;
using namespace std;

class THaEvtTypeHandler : public THaAnalysisObject {

public:

   THaEvtTypeHandler(const char*, const char*);
   virtual ~THaEvtTypeHandler();  

   virtual Int_t Analyze(THaEvData *evdata); 
   virtual EStatus Init( const TDatime& run_time );
   virtual void Print();
   virtual Bool_t IsMyEvent(Int_t evnum); 
   virtual void Dump(THaEvData *evdata);

protected:
 
   vector<Int_t> eventtypes;
   virtual void MakePrefix();
   ofstream *fDebugFile;

private:


   THaEvtTypeHandler(const THaEvtTypeHandler &fh);
   THaEvtTypeHandler& operator=(const THaEvtTypeHandler &fh);

   ClassDef(THaEvtTypeHandler,0)  // Event type handler

};

#endif
