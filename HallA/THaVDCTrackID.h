#ifndef Podd_THaVDCTrackID_h_
#define Podd_THaVDCTrackID_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCTrackID                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "THaTrackID.h"

class THaVDCPoint;

class THaVDCTrackID : public THaTrackID {

public:
  THaVDCTrackID()
    : fLowerU(-1), fLowerV(-1), fUpperU(-1), fUpperV(-1) {}
  THaVDCTrackID( Int_t lowerU, Int_t lowerV, Int_t upperU, Int_t upperV )
    : fLowerU(lowerU), fLowerV(lowerV), fUpperU(upperU), fUpperV(upperV) {}
  THaVDCTrackID( const THaVDCPoint* lower, const THaVDCPoint* upper );

  Bool_t  operator==( const THaTrackID& ) const override;
  Bool_t  operator==( const THaVDCTrackID& ) const;
  Bool_t  operator!=( const THaVDCTrackID& rhs ) const
  { return !THaVDCTrackID::operator==(rhs); }

  void     Print( Option_t* opt="" ) const override;
  TObject* Clone(const char* ="") const override;

protected:

  Int_t         fLowerU;         // Lower U plane pivot wire number
  Int_t         fLowerV;         // Lower V plane pivot wire number
  Int_t         fUpperU;         // Upper U plane pivot wire number
  Int_t         fUpperV;         // Upper V plane pivot wire number

  ClassDefOverride(THaVDCTrackID,0)  // VDC Track ID class
};

//__________________ inlines __________________________________________________
inline
Bool_t THaVDCTrackID::operator==( const THaVDCTrackID& rhs ) const
{
  return ( (fLowerU == rhs.fLowerU) && (fLowerV == rhs.fLowerV) &&
	   (fUpperU == rhs.fUpperU) && (fUpperV == rhs.fUpperV) );
}

//_____________________________________________________________________________
inline
Bool_t THaVDCTrackID::operator==( const THaTrackID& RHS ) const
{
  if( IsA() != RHS.IsA() ) return false;
  const auto& rhs = static_cast<const THaVDCTrackID&>(RHS);
  return THaVDCTrackID::operator==(rhs);
}

//////////////////////////////////////////////////////////////////////////////

#endif
