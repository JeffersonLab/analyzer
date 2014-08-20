#ifndef Scaler560_
#define Scaler560_

/////////////////////////////////////////////////////////////////////
//
//   Scaler560
//   CAEN model 560 scaler
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "THaGenScaler.h"

class Scaler560 : public THaGenScaler {

public:

   Scaler560() {};  
   Scaler560(Int_t crate, Int_t slot);  
   virtual ~Scaler560();  

   void Init();

private:

   static TypeIter_t fgThisType;

   ClassDef(Scaler560,0)  // CAEN scaler model 560

};

#endif
