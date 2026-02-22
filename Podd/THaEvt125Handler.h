#ifndef Podd_THaEvt125Handler_h_
#define Podd_THaEvt125Handler_h_

/////////////////////////////////////////////////////////////////////
//
//   THaEvt125Handler
//   For more details see the implementation file.
//   This handles hall C's event type 125.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include <string>
#include <vector>
#include <map>

class THaEvt125Handler : public THaEvtTypeHandler {

public:

   THaEvt125Handler(const char* name, const char* description);

   Int_t Analyze(THaEvData *evdata) override;
   EStatus Init( const TDatime& run_time) override;
   Float_t GetData(const std::string& tag);

private:

   std::map<std::string, Float_t> theDataMap;
   std::vector<std::string> dataKeys;
   UInt_t NVars;
   Double_t *dvars; 

   ClassDefOverride(THaEvt125Handler,0)  // Hall C event type 125

};

#endif
