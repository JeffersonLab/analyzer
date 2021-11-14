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

   Lecroy1881Module( UInt_t crate, UInt_t slot );
   Lecroy1881Module() = default;
   virtual ~Lecroy1881Module() = default;

   using FastbusModule::Init;
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Lecroy1881Module,0)  // Lecroy 1881 ADC module

};

}

#endif
