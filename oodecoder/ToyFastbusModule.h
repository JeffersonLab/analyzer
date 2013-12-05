#ifndef ToyFastbusModule_
#define ToyFastbusModule_

/////////////////////////////////////////////////////////////////////
//
//   ToyFastbusModule
//   Fastbus module.  Toy Class.  
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
   ToyFastbusModule(Int_t crate, Int_t slot);
   virtual ~ToyFastbusModule(); 

   Bool_t IsSlot(Int_t rdata);
   Int_t Decode(THaEvData *evdata, Int_t start);


private:

   Int_t fChanMask, fDataMask, fWdcntMask, fChanShift;

   ToyFastbusModule(const ToyFastbusModule &fh);
   ToyFastbusModule& operator=(const ToyFastbusModule &fh);

   ClassDef(ToyFastbusModule,0)  // Fastbus module

};

#endif
