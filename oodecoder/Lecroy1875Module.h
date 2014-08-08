#ifndef Lecroy1875Module_
#define Lecroy1875Module_

/////////////////////////////////////////////////////////////////////
//
//   Lecroy1875Module
//   1875 Lecroy Fastbus module.  Toy Class.  
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

class Lecroy1875Module : public ToyFastbusModule {

public:

   Lecroy1875Module() {};  
   Lecroy1875Module(Int_t crate, Int_t slot);
   virtual ~Lecroy1875Module(); 
   void Init();  

private:

   static TypeIter_t fgThisType;

   Lecroy1875Module(const Lecroy1875Module &fh);
   Lecroy1875Module& operator=(const Lecroy1875Module &fh);

   ClassDef(Lecroy1875Module,0)  // Lecroy 1875 TDC module

};

#endif
