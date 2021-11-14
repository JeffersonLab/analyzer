#ifndef Podd_Scaler560_h_
#define Podd_Scaler560_h_

/////////////////////////////////////////////////////////////////////
//
//   Scaler560
//   CAEN model 560 scaler
//
/////////////////////////////////////////////////////////////////////

#include "GenScaler.h"

namespace Decoder {

class Scaler560 : public GenScaler {

public:

   Scaler560( UInt_t crate, UInt_t slot );
   Scaler560() = default;
   virtual ~Scaler560() = default;

   using GenScaler::Init;
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Scaler560,0)  // CAEN scaler model 560

};

}

#endif
