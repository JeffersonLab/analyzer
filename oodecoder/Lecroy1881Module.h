#ifndef Lecroy1881Module_
#define Lecroy1881Module_

/////////////////////////////////////////////////////////////////////
//
//   Lecroy1881Module
//   1881 Lecroy Fastbus module.  Toy Class.  
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

class Lecroy1881Module : public ToyFastbusModule {

public:

   Lecroy1881Module() {};  
   Lecroy1881Module(Int_t crate, Int_t slot);
   virtual ~Lecroy1881Module(); 
   void Init();  

private:

   static TypeIter_t fgThisType;

   Lecroy1881Module(const Lecroy1881Module &fh);
   Lecroy1881Module& operator=(const Lecroy1881Module &fh);

   ClassDef(Lecroy1881Module,0)  // Lecroy 1881 ADC module

};

#endif
