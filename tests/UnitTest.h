#ifndef Podd_Tests_UnitTest_h_
#define Podd_Tests_UnitTest_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Tests::UnitTest                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"

namespace Podd::Tests {

class UnitTest : public THaAnalysisObject {

public:
  UnitTest() = default;  // Needed for RTTI of derived classes
  UnitTest( const char* name, const char* description );

protected:
  // By default, let unit tests not bother with the run database
  virtual Int_t  ReadRunDatabase( const TDatime& ) { return kOK; }

  ClassDef(UnitTest,1)
};

} // namespace Podd::Tests

////////////////////////////////////////////////////////////////////////////////

#endif
