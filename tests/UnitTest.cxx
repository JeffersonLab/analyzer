//*-- Author :    Ole Hansen   13-Nov-14
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Tests::UnitTest                                                     //
//                                                                           //
// Base class for unit test "test objects" (derived from THaAnalysisObject)  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "UnitTest.h"

namespace Podd::Tests {

//_____________________________________________________________________________
UnitTest::UnitTest( const char* name, const char* description )
  : THaAnalysisObject(name,description)
{
  // Constructor
}

} // namespace Podd::Tests

////////////////////////////////////////////////////////////////////////////////

ClassImp(Podd::Tests::UnitTest)
