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

   Scaler3800() : GenScaler() {}
   Scaler3800(Int_t crate, Int_t slot);
   virtual ~Scaler3800();

   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Scaler3800,0)  // SIS model 3800 scaler

};

}

#endif
