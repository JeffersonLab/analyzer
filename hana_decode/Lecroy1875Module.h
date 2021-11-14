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

   Lecroy1875Module( UInt_t crate, UInt_t slot );
   Lecroy1875Module() = default;
   virtual ~Lecroy1875Module() = default;

   using FastbusModule::Init;
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Lecroy1875Module,0)  // Lecroy 1875 TDC module

};

}

#endif
