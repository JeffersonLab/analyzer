#ifndef ToyFastbusModule_
#define ToyFastbusModule_

/////////////////////////////////////////////////////////////////////
//
//   ToyFastbusModule
//   Fastbus module.  Toy Class.  
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#define MAXSLOTS_FASTBUS 26

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

   Int_t Slot(Int_t rdata) { return (rdata&fSlotMask)>>fSlotShift; };
   Int_t Chan(Int_t rdata) { return (rdata&fChanMask)>>fChanShift; };
   Int_t WndCnt(Int_t rdata) { 
          if (fNoWdCnt) return -1; 
          return (rdata&fWdcntMask)>>fWdcntShift; 
   };
   Int_t Data(Int_t rdata);

protected:

   Int_t fSlotMask, fSlotShift;
   Int_t fChanMask, fChanShift;
   Int_t fDatamask,  fDataShift;
   Int_t fWdcnt, fWdcntMask;
   Int_t fNoWdCnt;

private:

   ToyFastbusModule(const ToyFastbusModule &fh);
   ToyFastbusModule& operator=(const ToyFastbusModule &fh);

   ClassDef(ToyFastbusModule,0)  // Fastbus module

};

#endif
