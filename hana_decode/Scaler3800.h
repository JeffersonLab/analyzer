#ifndef Podd_Scaler3800_h_
#define Podd_Scaler3800_h_

/////////////////////////////////////////////////////////////////////
//
//   Scaler3800
//   SIS (Struck) model 3800 scaler
//
/////////////////////////////////////////////////////////////////////

#include "GenScaler.h"

namespace Decoder {

class Scaler3800 : public GenScaler {

public:

   Scaler3800( UInt_t crate, UInt_t slot);
   Scaler3800() = default;
   virtual ~Scaler3800() = default;

   using GenScaler::Init;
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Scaler3800,0)  // SIS model 3800 scaler

};

}

#endif
