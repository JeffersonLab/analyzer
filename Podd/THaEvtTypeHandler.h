#ifndef Podd_THaEvtTypeHandler_h_
#define Podd_THaEvtTypeHandler_h_

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
#include <cassert>

class THaEvtTypeHandler : public THaAnalysisObject {

public:
   THaEvtTypeHandler(const char* name, const char* description);
   virtual ~THaEvtTypeHandler();

   virtual Int_t Analyze(THaEvData *evdata) = 0;
   virtual EStatus Init( const TDatime& run_time );
   virtual void EvPrint() const;
   virtual Bool_t IsMyEvent( UInt_t type ) const;
   virtual void EvDump(THaEvData *evdata) const;
   virtual void SetDebugFile(std::ofstream *file) { if (file) fDebugFile=file; };
   virtual void SetDebugFile(const char *filename);
   virtual void AddEvtType( UInt_t evtype );
   virtual void SetEvtType( UInt_t evtype );
   virtual UInt_t GetNumTypes() { return eventtypes.size(); };
   virtual UInt_t GetEvtType() {
     assert( !eventtypes.empty() );
     return eventtypes[0];
   }
   virtual std::vector<UInt_t> GetEvtTypes() { return eventtypes; };

protected:
   std::vector<UInt_t> eventtypes;
   std::ofstream *fDebugFile;

   virtual void MakePrefix();

private:
   THaEvtTypeHandler(const THaEvtTypeHandler &fh);
   THaEvtTypeHandler& operator=(const THaEvtTypeHandler &fh);

   ClassDef(THaEvtTypeHandler,0)  // Event type handler

};

#endif
