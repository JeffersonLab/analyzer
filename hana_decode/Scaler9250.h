#ifndef Podd_Scaler9250_
#define Podd_Scaler9250_

/////////////////////////////////////////////////////////////////////
//
//   Scaler9250
//   FADC250 scalers bank data (bank 9250)
//
//   Ole Hansen Apr 2026
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

   ClassDefOverride(Scaler9250,0)  // FADC250 scaler bank data
};

}

#endif
