//////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCLookupTTDConv                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCLookupTTDConv.h"

ClassImp(THaVDCLookupTTDConv)


//______________________________________________________________________________
THaVDCLookupTTDConv::THaVDCLookupTTDConv()
{
  //Normal constructor
  fLongestDist = 0.01512;
  fTimeRes = 0.5e-9;
}


//______________________________________________________________________________
THaVDCLookupTTDConv::~THaVDCLookupTTDConv()
{
  // Destructor. Remove variables from global list.

}

//______________________________________________________________________________
Double_t THaVDCLookupTTDConv::ConvertTimeToDist(Double_t time, 
						Double_t tanTheta)
{
  int binned_time;
  Double_t dist;

  // initial culling
  if(time < fT0)
    return 0.0;

  // bin the given time
  binned_time = static_cast<int>((time - fT0) / fTimeRes);
  
  // make sure it's in the correct range
  if(binned_time >= fNumBins)
    return fLongestDist;

  // figure out time
  dist = fTable[binned_time];

  // do angle adjustments?

  return dist;
}


////////////////////////////////////////////////////////////////////////////////
