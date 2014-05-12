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

class THaEvData;

class ToyModule  {

public:

   ToyModule();  
   ToyModule(Int_t crate, Int_t slot);  
   virtual ~ToyModule();  

   virtual Bool_t IsSlot(Int_t rdata)=0;
   virtual Int_t GetNumWords() { return fNumWords; };
   virtual Int_t Decode(THaEvData *evdata, Int_t start=0)=0;
   Int_t GetCrate() { return fCrate; };
   Int_t GetSlot() { return fSlot; };
   ToyModule& operator=(const ToyModule &rhs);

protected:

  Int_t fCrate, fSlot;
  Int_t fNumWord;
  ToyModule(const ToyModule& rhs); 
  void Create(const ToyModule& rhs);

private:

   ClassDef(ToyModule,0)  // A module in a crate and slot

};

#endif
