#ifndef Podd_Tests_ArrayRTTI_h_
#define Podd_Tests_ArrayRTTI_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Tests::ArrayRTTI "test object" for ArrayRTTI_t tests                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "UnitTest.h"
#include <vector>

namespace Podd::Tests {

class ArrayRTTI : public UnitTest {

public:
  ArrayRTTI() : fArray{}, f2D{}, fN{0}, fVarArr{nullptr} {}
  explicit ArrayRTTI( const char* name,
                      const char* description = "Array RTTI test" );
  virtual ~ArrayRTTI();

  virtual Int_t DefineVariables( EMode mode );
  static const RVarDef* GetRVarDef();
  static Int_t GetNvars();

  // Array sizes
  static constexpr Int_t k1Dsize  = 8;
  static constexpr Int_t k2Dcols  = 5;
  static constexpr Int_t k2Drows  = 7;
  static constexpr Int_t kVarSize = 15;
  static constexpr Int_t kVecSize = 13;

  // Test data
  Data_t              fArray[k1Dsize];       // One-dimensional fixed
  Data_t              f2D[k2Dcols][k2Drows]; // Two-dimensional fixed
  Int_t               fN;                    // Number of elements in fVarArr
  Data_t*             fVarArr;               // [fN] variable-size
  std::vector<Data_t> fVectorD;              // std::vector array (1d, var size)

  ArrayRTTI( const ArrayRTTI& rhs ) = delete;
  ArrayRTTI( ArrayRTTI&& rhs ) = delete;
  ArrayRTTI& operator=( const ArrayRTTI& rhs ) = delete;
  ArrayRTTI& operator=( ArrayRTTI&& rhs ) = delete;

  ClassDef(ArrayRTTI,1)   // Structure for testing variables defined on arrays
};

} // namespace Podd::Tests

////////////////////////////////////////////////////////////////////////////////

#endif
