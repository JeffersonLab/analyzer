#ifndef Scaler3801_
#define Scaler3801_

/////////////////////////////////////////////////////////////////////
//
//   Scaler3801
//   SIS (Struck) model 3801 scaler
//
/////////////////////////////////////////////////////////////////////

#include "GenScaler.h"

namespace Decoder {

class Scaler3801 : public GenScaler {

public:

   Scaler3801() {};
   Scaler3801(Int_t crate, Int_t slot);
   virtual ~Scaler3801();

   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Scaler3801,0)  // SIS model 3801 scaler

};

}

#endif
