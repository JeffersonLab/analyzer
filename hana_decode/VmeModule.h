#ifndef VmeModule_
#define VmeModule_

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

   VmeModule() {};
   VmeModule(Int_t crate, Int_t slot);
   virtual ~VmeModule();

   virtual Bool_t IsSlot(UInt_t rdata);
   virtual Int_t Slot(Int_t rdata) { return fSlot; };
   virtual Int_t Data(Int_t rdata) { return rdata; };

private:

   static TypeIter_t fgThisType;
   VmeModule(const VmeModule &fh);
   VmeModule& operator=(const VmeModule &fh);
   ClassDef(Decoder::VmeModule,0)  // A VME module (abstract)

};

}

#endif
