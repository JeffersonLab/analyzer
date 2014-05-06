#ifndef ToyPhysicsEvtHandler_
#define ToyPhysicsEvtHandler_

/////////////////////////////////////////////////////////////////////
//
//   NEW STUFF (the entire class)
//   ToyPhysicsEvtHandler
//   Abstract class to handle different types of events.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"

class ToyEvtTypeHandler;
class THaEvData;

class ToyPhysicsEvtHandler : public ToyEvtTypeHandler {

public:

   ToyPhysicsEvtHandler();
   virtual ~ToyPhysicsEvtHandler();  

   Int_t Decode(THaEvData *evdata);
   Int_t Init(THaCrateMap *map);
  

private:

   ToyPhysicsEvtHandler(const ToyPhysicsEvtHandler &fh);
   ToyPhysicsEvtHandler& operator=(const ToyPhysicsEvtHandler &fh);

   Int_t FindRocs(const Int_t *evbuffer);
   Int_t irn[MAXROC];
   struct RocDat_t {           // ROC raw data descriptor
        Int_t pos;             // position in evbuffer[]
        Int_t len;             // length of data
   } rocdat[MAXROC];

   ClassDef(ToyPhysicsEvtHandler,0)  // Physics Event handler

};

#endif
