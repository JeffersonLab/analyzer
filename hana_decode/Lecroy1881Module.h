#ifndef Lecroy1881Module_
#define Lecroy1881Module_

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

   Lecroy1881Module() {};
   Lecroy1881Module(Int_t crate, Int_t slot);
   virtual ~Lecroy1881Module();
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   Lecroy1881Module(const Lecroy1881Module &fh);
   Lecroy1881Module& operator=(const Lecroy1881Module &fh);

   ClassDef(Lecroy1881Module,0)  // Lecroy 1881 ADC module

};

}

#endif
