#ifndef ToyModule_
#define ToyModule_

/////////////////////////////////////////////////////////////////////
//
//   ToyModule
//   Abstract interface for a module that sits in a slot of a crate.
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "ToyModule.h"

class THaEvData;

class ToyModule  {

public:

   ToyModule();  
   ToyModule(Int_t crate, Int_t slot);  
   virtual ~ToyModule();  

   virtual Bool_t IsSlot(Int_t rdata)=0;
   virtual Int_t Decode(THaEvData *evdata, Int_t start=0)=0;
   Int_t GetSlot() { return fSlot; );

protected:

   Int_t fCrate, fSlot;

private:

   ToyModule(const ToyModule& rhs); 
   ToyModule& operator=(const ToyModule &rhs);

   ClassDef(ToyModule,0)  // Module type "X"

};

#endif
