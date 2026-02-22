#ifndef Podd_THaHelicityDet_h_
#define Podd_THaHelicityDet_h_

////////////////////////////////////////////////////////////////////////
//
// THaHelicityDet
//
// Abstract base class for a beam helicity "detector".
// Typically, this type of detector will be part of a beam apparatus.
// 
// authors: V. Sulkosky and R. Feuerbach, Jan 2006
// Changed to an abstract base class. Ole Hansen, Aug 2006.
//
////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"

class THaHelicityDet : public THaDetector {
public:
  enum EHelicity { kMinus = -1, kUnknown, kPlus };

  ~THaHelicityDet() override;

  void       Clear( Option_t* opt="" ) override
  { THaDetector::Clear(opt); fHelicity = kUnknown; }
  virtual EHelicity  GetHelicity()   const { return fHelicity; }
  virtual Bool_t     HelicityValid() const { return (fHelicity != kUnknown); }

  const char* GetDBFileName() const override;

  THaHelicityDet() : fHelicity(kUnknown), fSign(1) {}  // For ROOT I/O only

protected:

  //Only derived classes may construct me
  THaHelicityDet( const char* name, const char* description,
                  THaApparatus* a = nullptr );

  EHelicity fHelicity;  // Beam helicity. fHelicity = fSign * decoded_helicity
  Int_t     fSign;      // Overall sign of beam helicity. Default 1.

  void  MakePrefix() override;
  Int_t DefineVariables( EMode mode = kDefine ) override;
  Int_t ReadDatabase( const TDatime& date ) override;

  ClassDefOverride(THaHelicityDet,1)     // ABC for a beam helicity detector
};

#endif
