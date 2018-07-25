#ifndef Podd_OldVDCTrackID_h_
#define Podd_OldVDCTrackID_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCTrackID                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "THaTrackID.h"

class OldVDCUVTrack;

class OldVDCTrackID : public THaTrackID {

public:
  OldVDCTrackID() : THaTrackID(),
    fLowerU(0), fLowerV(0), fUpperU(0), fUpperV(0) {}
  OldVDCTrackID( Int_t lowerU, Int_t lowerV,
		 Int_t upperU, Int_t upperV ) : THaTrackID(),
    fLowerU(lowerU), fLowerV(lowerV), fUpperU(upperU), fUpperV(upperV) {}
  OldVDCTrackID( const OldVDCUVTrack* lower, const OldVDCUVTrack* upper );
  OldVDCTrackID( const OldVDCTrackID& );
  OldVDCTrackID& operator=( const OldVDCTrackID& );
  
  virtual ~OldVDCTrackID() {}

  virtual Bool_t  operator==( const THaTrackID& );
  virtual Bool_t  operator!=( const THaTrackID& );
  virtual void    Print( Option_t* opt="" ) const;

protected:

  Int_t           fLowerU;         // Lower U plane pivot wire number
  Int_t           fLowerV;         // Lower V plane pivot wire number
  Int_t           fUpperU;         // Upper U plane pivot wire number
  Int_t           fUpperV;         // Upper V plane pivot wire number

  ClassDef(OldVDCTrackID,0)      // Track ID class
};

//__________________ inlines __________________________________________________
inline
Bool_t OldVDCTrackID::operator==( const THaTrackID& RHS )
{
  if( IsA() != RHS.IsA() ) return kFALSE;
  const OldVDCTrackID& rhs = static_cast<const OldVDCTrackID&>(RHS);
  return ( (fLowerU == rhs.fLowerU) && (fLowerV == rhs.fLowerV) &&
	   (fUpperU == rhs.fUpperU) && (fUpperV == rhs.fUpperV) );
}

//__________________ inlines __________________________________________________
inline
Bool_t OldVDCTrackID::operator!=( const THaTrackID& RHS )
{
  if( IsA() != RHS.IsA() ) return kTRUE;
  const OldVDCTrackID& rhs = static_cast<const OldVDCTrackID&>(RHS);
  return ( (fLowerU != rhs.fLowerU) || (fLowerV != rhs.fLowerV) ||
	   (fUpperU != rhs.fUpperU) || (fUpperV != rhs.fUpperV) );
}

//////////////////////////////////////////////////////////////////////////////

#endif
