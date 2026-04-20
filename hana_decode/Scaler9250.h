#ifndef Scaler9250_
#define Scaler9250_

/////////////////////////////////////////////////////////////////////
//
//   Scaler9250
//   FADC250 scalers
//
//   Apr 2026
//   TODO this is a very temporary class that will disappear shortly
//
/////////////////////////////////////////////////////////////////////

#include "GenScaler.h"

namespace Decoder {

class Scaler9250 : public GenScaler {

public:

   Scaler9250() = default;
   Scaler9250( UInt_t crate, UInt_t slot );

   void Init() override;

private:

   static TypeIter_t fgThisType;

   ClassDefOverride(Scaler9250,0)  // FADC250 scalers

};

}

#endif
