////////////////////////////////////////////////////////////////////////
//
// THaHelicityDet
//
// Abstract base class for a beam helicity "detector".
// Typically, this type of detector will be part of a beam apparatus.
// 
// author: V. Sulkosky and R. Feuerbach, Jan 2006
// Changed to an abstract base class. Ole Hansen, Aug 2006.
//
////////////////////////////////////////////////////////////////////////

#ifndef ROOT_THaHelicityDet
#define ROOT_THaHelicityDet

#include "THaDetector.h"

class THaHelicityDet :  public THaDetector {
  
public:

  virtual ~THaHelicityDet();

  virtual Int_t  GetHelicity()   const { return fHelicity; }
  virtual Bool_t HelicityValid() const { return (fHelicity != 0); }

  THaHelicityDet();  // For ROOT I/O only

protected:

  //Only derived classes may construct me

  THaHelicityDet(const char* name, const char* description,
		 THaApparatus* a = NULL);

  Int_t fHelicity;               // Beam helicity

  ClassDef(THaHelicityDet,1)     // ABC for a beam helicity detector
};

#endif
