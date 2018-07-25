///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCTimeToDistConv                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCTimeToDistConv.h"

using namespace std;

ClassImp(VDC::TimeToDistConv)

namespace VDC {

//_____________________________________________________________________________
TimeToDistConv::TimeToDistConv( UInt_t npar )
  : fNparam(npar), fDriftVel(kBig), fIsSet(false)
{
  // Constructor
}

//_____________________________________________________________________________
void TimeToDistConv::SetDriftVel( Double_t v )
{
  fDriftVel = v;
  if( fNparam == 0 )
    fIsSet = true;
}

//_____________________________________________________________________________
Int_t TimeToDistConv::SetParameters( const vector<double>& )
{
  if( fNparam == 0 )
    fIsSet = true;
  return 0;
}

} // namespace VDC

////////////////////////////////////////////////////////////////////////////////
