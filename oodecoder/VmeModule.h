#ifndef VmeModule_
#define VmeModule_

/////////////////////////////////////////////////////////////////////
//
//   VmeModule
//   A VME module.  This is an abstract class.
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "ToyModule.h"

class VmeModule : public ToyModule {

public:

  VmeModule() {};  
   VmeModule(Int_t crate, Int_t slot);  
   virtual ~VmeModule();  

   virtual Bool_t IsSlot(UInt_t rdata);
   virtual Int_t Decode(const Int_t *evbuffer)=0;
   virtual Int_t Slot(Int_t rdata) { return fSlot; };
   virtual Int_t Data(Int_t rdata) { return rdata; };
         


private:

   static TypeIter_t fgThisType;
   Int_t slotmask, chanmask, datamask;
   ClassDef(VmeModule,0)  // A VME module (abstract)

};

#endif
