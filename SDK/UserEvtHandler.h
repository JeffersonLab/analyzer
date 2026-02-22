#ifndef Podd_UserEvtHandler_h_
#define Podd_UserEvtHandler_h_

/////////////////////////////////////////////////////////////////////
//
//   UserEvtHandler
//   For more details see the implementation file.
//   The idea would be to copy this and modify it for your purposes.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include <string>
#include <vector>
#include <map>

class UserEvtHandler : public THaEvtTypeHandler {

public:

   UserEvtHandler(const char* name, const char* description);

   Int_t Analyze(THaEvData *evdata) override;
   EStatus Init( const TDatime& run_time) override;
   Data_t GetData(const std::string& tag) const;

private:

   std::map<std::string, Data_t> theDataMap;
   std::vector<std::string> dataKeys;
   Data_t *dvars;

   ClassDefOverride(UserEvtHandler,0)  // Example of an Event handler
};

#endif
