#ifndef Podd_Tests_ArrayRTTI
#define Podd_Tests_ArrayRTTI

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ArrayRTTI unit test                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "UnitTest.h"
#include <vector>

namespace Podd {
namespace Tests {

class ArrayRTTI : public UnitTest {

public:
  ArrayRTTI( const char* name = "array_rtti",
	     const char* description = "Array RTTI unit test" );
  virtual ~ArrayRTTI();

  virtual Int_t Test();

protected:

  // Array sizes
  static const Int_t fgD1  = 8;
  static const Int_t fgD21 = 5;
  static const Int_t fgD22 = 7;
  static const Int_t fgDV  = 15;
  static const Int_t fgDVE = 13;

  // Test data
  Float_t    fArray[fgD1];      // One-dimensional fixed
  Float_t    f2D[fgD21][fgD22]; // Two-dimensional fixed
  Int_t      fN;                // Number of elements in fVarArr
  Float_t*   fVarArr;           // [fN] variable-size
  std::vector<Float_t> fVectorF;// std::vector array (1d, var size)

  virtual Int_t  DefineVariables( EMode mode );
  virtual Int_t  ReadDatabase( const TDatime& date );

  ClassDef(ArrayRTTI,0)   // Example detector
};

} // namespace Tests
} // namespace Podd

////////////////////////////////////////////////////////////////////////////////

#endif
