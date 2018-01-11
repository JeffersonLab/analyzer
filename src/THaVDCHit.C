///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCHit                                                                 //
//                                                                           //
// Class representing a single hit for the VDC                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCHit.h"
#include "THaVDCTimeToDistConv.h"
#include "TError.h"

const Double_t THaVDCHit::kBig = 1.e38; // Arbitrary large value

using namespace VDC;

//_____________________________________________________________________________
Double_t THaVDCHit::ConvertTimeToDist(Double_t slope)
{
  // Converts TDC time to drift distance
  // Takes the (estimated) slope of the track as an argument

  VDC::TimeToDistConv* ttdConv = (fWire) ? fWire->GetTTDConv() : NULL;

  if (ttdConv) {
    // If a time to distance algorithm exists, use it to convert the TDC time
    // to the drift distance
    fDist = ttdConv->ConvertTimeToDist(fTime, slope, &fdDist);
    return fDist;
  }

  Error("ConvertTimeToDist()", "No Time to dist algorithm available");
  return 0.0;

}

//_____________________________________________________________________________
Int_t THaVDCHit::Compare( const TObject* obj ) const
{
  // Used to sort hits
  // A hit is "less than" another hit if it occurred on a lower wire number.
  // Also, for hits on the same wire, the first hit on the wire (the one with
  // the smallest time) is "less than" one with a higher time.  If the hits
  // are sorted according to this scheme, they will be in order of increasing
  // wire number and, for each wire, will be in the order in which they hit
  // the wire

  assert( obj && IsA() == obj->IsA() );

  if( obj == this )
    return 0;

#ifndef NDEBUG
  const THaVDCHit* other = dynamic_cast<const THaVDCHit*>( obj );
  assert( other );
#else
  const THaVDCHit* other = static_cast<const THaVDCHit*>( obj );
#endif

  ByWireThenTime isless;
  if( isless( this, other ) )
    return -1;
  if( isless( other, this ) )
    return 1;
  return 0;
}

//_____________________________________________________________________________
ClassImp(THaVDCHit)
