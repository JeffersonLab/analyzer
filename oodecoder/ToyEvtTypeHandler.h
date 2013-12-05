#ifndef ToyEvtTypeHandler_
#define ToyEvtTypeHandler_

/////////////////////////////////////////////////////////////////////
//
//   NEW STUFF (= the entire class)
//   ToyEvtTypeHandler
//   Abstract class to handle different types of events.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"

class THaCrateMap;
class THaEvData;

class ToyEvtTypeHandler {

public:

   ToyEvtTypeHandler();
   virtual ~ToyEvtTypeHandler();  

   virtual Int_t Decode(THaEvData *evdata)=0; // abstract
   virtual void Init(THaCrateMap *map)=0;

protected:

   

private:

   ToyEvtTypeHandler(const ToyEvtTypeHandler &fh);
   ToyEvtTypeHandler& operator=(const ToyEvtTypeHandler &fh);

   ClassDef(ToyEvtTypeHandler,0)  // Event type handler

};

#endif
