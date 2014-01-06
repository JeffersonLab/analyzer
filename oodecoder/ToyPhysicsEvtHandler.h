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
   void Init(THaCrateMap *map);
  

private:

   ToyPhysicsEvtHandler(const ToyPhysicsEvtHandler &fh);
   ToyPhysicsEvtHandler& operator=(const ToyPhysicsEvtHandler &fh);

   ClassDef(ToyPhysicsEvtHandler,0)  // Physics Event handler

};

#endif
