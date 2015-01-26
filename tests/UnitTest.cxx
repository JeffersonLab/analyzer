//*-- Author :    Ole Hansen   13-Nov-14
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Tests::UnitTest                                                     //
//                                                                           //
// Abstract base class for a unit test                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "UnitTest.h"
#include "TMath.h"

using namespace std;

namespace Podd {
namespace Tests {

//_____________________________________________________________________________
UnitTest::UnitTest( const char* name, const char* description )
  : THaAnalysisObject(name,description)
{
  // Constructor
}

//_____________________________________________________________________________
template< typename T > T UnitTest::Round( T val, Int_t n ) const
{
  // Round to n+1 digits

  if( val == 0 )
    return 0;
  if( n < 0 )
    n = 0;
  T sign = ((val < 0) ? -1 : 1);
  T norm = TMath::Power(10,n-TMath::FloorNint(TMath::Log10(val)));
  return sign * Int_t(TMath::Abs(val)*norm+0.5) / norm;
}

template Float_t  UnitTest::Round( Float_t,  Int_t ) const;
template Double_t UnitTest::Round( Double_t, Int_t ) const;

} // namespace Tests
} // namespace Podd

////////////////////////////////////////////////////////////////////////////////

ClassImp(Podd::Tests::UnitTest)
