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
   virtual ~ToyModule();  

   Bool_t Found(Int_t i) { return kTRUE; };

   Bool_t IsSlot(Int_t slot, Int_t rdata);
   Int_t Decode(THaEvData *evdata, Int_t start);
   ToyModule(const ToyModule& rhs); 
   ToyModule& operator=(const ToyModule &rhs);

protected:

   void Create(const ToyModule&);
   void Uncreate();

private:

   Int_t slotmask, chanmask, datamask;
   ClassDef(ToyModule,0)  // Module type "X"

};

#endif
