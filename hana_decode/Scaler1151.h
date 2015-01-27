#ifndef Scaler1151_
#define Scaler1151_

/////////////////////////////////////////////////////////////////////
//
//   Scaler1151
//   Lecroy model 1151 scaler
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "Decoder.h"
#include "GenScaler.h"

class Decoder::Scaler1151 : public GenScaler {

public:

   Scaler1151() {};  
   Scaler1151(Int_t crate, Int_t slot);  
   virtual ~Scaler1151();  

   void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Scaler1151,0)  // LeCroy scaler model 1151

};

#endif
