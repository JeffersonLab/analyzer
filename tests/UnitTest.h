#ifndef Podd_Tests_UnitTest
#define Podd_Tests_UnitTest

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Unit test abstract base class                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"

namespace Podd {
namespace Tests {

class UnitTest : public THaAnalysisObject {

public:
  UnitTest( const char* name, const char* description );

  virtual Int_t Test() = 0;

protected:

  // By default, do not bother with the run database
  virtual Int_t  ReadRunDatabase( const TDatime& date ) { return kOK; }

  // Utility function for rounding numbers. T = Float_t or Double_t
  template< typename T > T Round( T val, Int_t n ) const;
  
  ClassDef(UnitTest,0)
};

} // namespace Tests
} // namespace Podd

////////////////////////////////////////////////////////////////////////////////

#endif
