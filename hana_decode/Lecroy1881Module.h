#ifndef Podd_Lecroy1881Module_h_
#define Podd_Lecroy1881Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Lecroy1881Module
//   1881 Lecroy Fastbus module.
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "FastbusModule.h"

namespace Decoder {

class Lecroy1881Module : public FastbusModule {

public:

   Lecroy1881Module() : FastbusModule() {}
   Lecroy1881Module(Int_t crate, Int_t slot);
   virtual ~Lecroy1881Module();
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Lecroy1881Module,0)  // Lecroy 1881 ADC module

};

}

#endif
