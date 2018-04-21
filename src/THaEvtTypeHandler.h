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

class THaEvtTypeHandler : public THaAnalysisObject {

public:

   THaEvtTypeHandler(const char* name, const char* description);
   virtual ~THaEvtTypeHandler();

   virtual Int_t Analyze(THaEvData *evdata) = 0;
   virtual EStatus Init( const TDatime& run_time );
   virtual void EvPrint() const;
   virtual Bool_t IsMyEvent(Int_t evnum) const;
   virtual void EvDump(THaEvData *evdata) const;
   virtual void SetDebugFile(std::ofstream *file) { if (file!=0) fDebugFile=file; };
   virtual void SetDebugFile(const char *filename);
   virtual void AddEvtType(int evtype);
   virtual void SetEvtType(int evtype);
   virtual Int_t GetNumTypes() { return eventtypes.size(); };
   virtual Int_t GetEvtType() {
     if (eventtypes.size()==0) return -1;
     return eventtypes[0];
   }
   virtual std::vector<Int_t> GetEvtTypes() { return eventtypes; };

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
