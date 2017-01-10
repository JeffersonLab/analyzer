#ifndef THaEvt125Handler_
#define THaEvt125Handler_

/////////////////////////////////////////////////////////////////////
//
//   THaEvt125Handler
//   For more details see the implementation file.
//   This handles hall C's event type 125.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include "Decoder.h"
#include <string>
#include <vector>
#include <map>
#include <iterator>
#include "TTree.h"
#include "TString.h"  

class THaEvt125Handler : public THaEvtTypeHandler {

public:

   THaEvt125Handler(const char* name, const char* description);
   virtual ~THaEvt125Handler();

   virtual Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time);
   virtual Int_t End( THaRunBase* r=0 );
   Float_t GetData(std::string tag);  

private:

   static const Int_t MAXDATA=20000;

   std::map<std::string, Float_t> theDataMap;
   std::vector<std::string> dataKeys;
   Int_t NVars;
   Double_t *dvars; 

   THaEvt125Handler(const THaEvt125Handler& fh);
   THaEvt125Handler& operator=(const THaEvt125Handler& fh);

   ClassDef(THaEvt125Handler,0)  // Hall C event type 125

};

#endif
