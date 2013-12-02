#ifndef ToyFastbusModule_
#define ToyFastbusModule_

/////////////////////////////////////////////////////////////////////
//
//   ToyFastbusModule
//   Abstract class for the devices (ADCs, TDCs, scales) which
//   occur in events.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "TNamed.h"
#include "ToyModule.h"

class THaCrateMap;
class THaEvData;

class ToyFastbusModule : public ToyModule {

public:

   ToyFastbusModule();
   virtual ~ToyFastbusModule(); 
   Int_t Decode(THaEvData *evdata, Int_t start);

// Why do I need to define this if it's in the F'ing base class ?
   Bool_t IsSlot(Int_t slot, Int_t rdata);
      


private:

   ToyFastbusModule(const ToyFastbusModule &fh);
   ToyFastbusModule& operator=(const ToyFastbusModule &fh);

   ClassDef(ToyFastbusModule,0)  // Fastbus module

};

#endif
