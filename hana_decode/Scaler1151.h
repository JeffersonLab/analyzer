#ifndef Podd_Scaler1151_h_
#define Podd_Scaler1151_h_

/////////////////////////////////////////////////////////////////////
//
//   Scaler1151
//   Lecroy model 1151 scaler
//
/////////////////////////////////////////////////////////////////////

#include "GenScaler.h"

namespace Decoder {

class Scaler1151 : public GenScaler {

public:

   Scaler1151( UInt_t crate, UInt_t slot );
   Scaler1151() = default;
   virtual ~Scaler1151() = default;

   using GenScaler::Init;
   virtual void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Scaler1151,0)  // LeCroy scaler model 1151

};

}

#endif
