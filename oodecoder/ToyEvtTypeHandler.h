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
#include "Rtypes.h"
#include "DecoderGlobals.h"

class THaCrateMap;
class THaEvData;

class ToyEvtTypeHandler {

public:

   ToyEvtTypeHandler();
   virtual ~ToyEvtTypeHandler();  

   virtual Int_t Decode(THaEvData *evdata); 
   virtual Int_t Init(THaCrateMap *map);

protected:

   THaCrateMap *fMap;
   Int_t FindRocs(const Int_t *evbuffer);
   Int_t irn[MAXROC];
   struct RocDat_t {           // ROC raw data descriptor
        Int_t pos;                // position in evbuffer[]
        Int_t len;                // length of data
    } rocdat[MAXROC];

private:

   ToyEvtTypeHandler(const ToyEvtTypeHandler &fh);
   ToyEvtTypeHandler& operator=(const ToyEvtTypeHandler &fh);

   ClassDef(ToyEvtTypeHandler,0)  // Event type handler

};

#endif
