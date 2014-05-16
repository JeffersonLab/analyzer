#ifndef Lecroy1877Module_
#define Lecroy1877Module_

/////////////////////////////////////////////////////////////////////
//
//   Lecroy1877Module
//   1877 Lecroy Fastbus module.  Toy Class.  
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "TNamed.h"
#include "ToyFastbusModule.h"

class THaCrateMap;
class THaEvData;

class Lecroy1877Module : public ToyFastbusModule {

public:

   Lecroy1877Module(Int_t crate, Int_t slot);
   virtual ~Lecroy1877Module(); 

   Bool_t IsSlot(Int_t rdata);
   Int_t Decode(THaEvData *evdata, Int_t start);


private:

   static TypeIter_t fgThisType;

   Lecroy1877Module(const Lecroy1877Module &fh);
   Lecroy1877Module& operator=(const Lecroy1877Module &fh);

   ClassDef(Lecroy1877Module,0)  // Lecroy 1877 TDC module

};

#endif
