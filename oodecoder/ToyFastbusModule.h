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


class ToyFastbusModule : public ToyModule {

public:

   ToyFastbusModule() {};
   ToyFastbusModule(Int_t crate, Int_t slot);
   virtual ~ToyFastbusModule(); 

   Int_t Slot(Int_t rdata) { return (rdata&fSlotMask)>>fSlotShift; };
   Int_t Chan(Int_t rdata) { return (rdata&fChanMask)>>fChanShift; };
   Int_t Data(Int_t rdata) { return (rdata&fDataMask)>>fDataShift; };
         
   Int_t Decode(const Int_t *evbuffer);

   Bool_t IsSlot(Int_t rdata) { return (Slot(rdata)==fSlot); };

protected:

   Bool_t fHasHeader;
   Int_t fHeader;
   Int_t fWdcntMask, fWdcntShift;
   Int_t fSlotMask, fSlotShift;
   Int_t fChanMask, fChanShift;
   Int_t fDataMask,  fDataShift;
   Int_t fOptMask, fOptShift;

private:

   static TypeIter_t fgThisType;
   ToyFastbusModule(const ToyFastbusModule &fh);
   ToyFastbusModule& operator=(const ToyFastbusModule &fh);
   void Create(const ToyFastbusModule& rhs);

   ClassDef(ToyFastbusModule,0)  // Fastbus module

};

#endif
