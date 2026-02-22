#ifndef Podd_Scaler3801_h_
#define Podd_Scaler3801_h_

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

   Scaler3801( UInt_t crate, UInt_t slot );
   Scaler3801() = default;

   using GenScaler::Init;
   void Init() override;

private:

   static TypeIter_t fgThisType;

   ClassDefOverride(Scaler3801,0)  // SIS model 3801 scaler

};

}

#endif
