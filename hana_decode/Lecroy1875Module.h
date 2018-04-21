#ifndef Podd_Lecroy1875Module_h_
#define Podd_Lecroy1875Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Lecroy1875Module
//   1875 Lecroy Fastbus module.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "FastbusModule.h"

namespace Decoder {

class Lecroy1875Module : public FastbusModule {

public:

   Lecroy1875Module() : FastbusModule() {}
   Lecroy1875Module(Int_t crate, Int_t slot);
   virtual ~Lecroy1875Module();
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Lecroy1875Module,0)  // Lecroy 1875 TDC module

};

}

#endif
