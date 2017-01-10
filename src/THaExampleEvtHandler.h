#ifndef THaExampleEvtHandler_
#define THaExampleEvtHandler_

/////////////////////////////////////////////////////////////////////
//
//   THaExampleEvtHandler
//   For more details see the implementation file.
//   The idea would be to copy this and modify it for your purposes.
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

class THaExampleEvtHandler : public THaEvtTypeHandler {

public:

   THaExampleEvtHandler(const char* name, const char* description);
   virtual ~THaExampleEvtHandler();

   virtual Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time);
   virtual Int_t End( THaRunBase* r=0 );
   Float_t GetData(std::string tag);  

private:

   static const Int_t MAXDATA=20000;

   std::map<std::string, Float_t> theDataMap;
   std::vector<std::string> dataKeys;
   Double_t *dvars; 

   THaExampleEvtHandler(const THaExampleEvtHandler& fh);
   THaExampleEvtHandler& operator=(const THaExampleEvtHandler& fh);

   ClassDef(THaExampleEvtHandler,0)  // Example of an Event handler

};

#endif
