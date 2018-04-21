#ifndef Podd_Lecroy1877Module_h_
#define Podd_Lecroy1877Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Lecroy1877Module
//   1877 Lecroy Fastbus module.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "FastbusModule.h"

namespace Decoder {

class Lecroy1877Module : public FastbusModule {

public:

   Lecroy1877Module() : FastbusModule() {}
   Lecroy1877Module(Int_t crate, Int_t slot);
   virtual ~Lecroy1877Module();
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Lecroy1877Module,0)  // Lecroy 1877 TDC module

};

}

#endif
