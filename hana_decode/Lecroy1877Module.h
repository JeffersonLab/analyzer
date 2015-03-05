#ifndef Lecroy1877Module_
#define Lecroy1877Module_

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

   Lecroy1877Module() {};
   Lecroy1877Module(Int_t crate, Int_t slot);
   virtual ~Lecroy1877Module();
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   Lecroy1877Module(const Lecroy1877Module &fh);
   Lecroy1877Module& operator=(const Lecroy1877Module &fh);

   ClassDef(Lecroy1877Module,0)  // Lecroy 1877 TDC module

};

}

#endif
