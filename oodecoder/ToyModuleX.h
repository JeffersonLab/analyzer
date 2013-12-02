#ifndef ToyModuleX_
#define ToyModuleX_

/////////////////////////////////////////////////////////////////////
//
//   ToyModuleX
//   A module type "X".  (presumably a VME module)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "ToyModule.h"

class ToyModuleX : public ToyModule {

public:

   ToyModuleX();  
   virtual ~ToyModuleX();  

   Int_t Decode(THaEvData *evdata, Int_t start);

   Bool_t IsSlot(Int_t slot, Int_t rdata);


private:

   Int_t slotmask, chanmask, datamask;
   ClassDef(ToyModuleX,0)  // Module type "X"

};

#endif
