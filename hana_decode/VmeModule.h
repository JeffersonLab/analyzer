#ifndef Podd_VmeModule_h_
#define Podd_VmeModule_h_

/////////////////////////////////////////////////////////////////////
//
//   VmeModule
//   A VME module.
//
/////////////////////////////////////////////////////////////////////

#include "Module.h"

namespace Decoder {

class VmeModule : public Module {

public:

   VmeModule(UInt_t crate, UInt_t slot);
   VmeModule() = default;
   virtual ~VmeModule() = default;

   using Module::LoadSlot;

   virtual Bool_t IsSlot(UInt_t rdata);
   // virtual Int_t Slot(Int_t) const { return fSlot; };
   // virtual Int_t Data(Int_t rdata) const { return rdata; };

   virtual UInt_t LoadSlot( THaSlotData *sldat, const UInt_t *evbuffer,
                            const UInt_t *pstop );

protected:

private:

   ClassDef(Decoder::VmeModule,0)  // A VME module (abstract)

};

}

#endif
