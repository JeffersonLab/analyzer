#ifndef THaEvtTypeHandler_
#define THaEvtTypeHandler_

/////////////////////////////////////////////////////////////////////
//
//   THaEvtTypeHandler
//   Base class to handle different types of events.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include <vector>
#include <fstream>

class THaEvtTypeHandler : public THaAnalysisObject {

public:

   THaEvtTypeHandler(const char*, const char*);
   virtual ~THaEvtTypeHandler();

   virtual Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time );
   virtual void EvPrint() const;
   virtual Bool_t IsMyEvent(Int_t evnum) const;
   virtual void EvDump(THaEvData *evdata) const;
   virtual void SetDebugFile(std::ofstream *file) { if (file!=0) fDebugFile=file; };

protected:

   std::vector<Int_t> eventtypes;
   virtual void MakePrefix();
   std::ofstream *fDebugFile;

private:

   THaEvtTypeHandler(const THaEvtTypeHandler &fh);
   THaEvtTypeHandler& operator=(const THaEvtTypeHandler &fh);

   ClassDef(THaEvtTypeHandler,0)  // Event type handler

};

#endif
