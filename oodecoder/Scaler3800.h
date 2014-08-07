#ifndef Scaler3800_
#define Scaler3800_

/////////////////////////////////////////////////////////////////////
//
//   Scaler3800
//   SIS (Struck) model 3800 scaler
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "THaGenScaler.h"

class Scaler3800 : public THaGenScaler {

public:

   Scaler3800() {};  
   Scaler3800(Int_t crate, Int_t slot);  
   virtual ~Scaler3800();  


private:

   static TypeIter_t fgThisType;

   ClassDef(Scaler3800,0)  // SIS model 3800 scaler

};

#endif