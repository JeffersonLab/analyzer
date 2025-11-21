//*-- Author :    Ole Hansen   12-Nov-14
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Tests::ArrayRTTI                                                    //
//                                                                           //
// Test object for ArrayRTTI_t tests                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "ArrayRTTI.h"

using namespace std;

// Variable definitions. The setup of these through DefineVariables (below)
// is the functionality to be tested
constexpr RVarDef vars[] = {
  { "oned",   "1D array",    "fArray" },
  { "elem0",  "fArray[0]",   "fArray[0]" },
  { "elem1",  "fArray[1]",   "fArray[1]" },
  { "twod",   "2D array",    "f2D" },
  { "elem02", "f2D[0][2]",   "f2D[0][2]" },
  { "elem30", "f2D[3][0]",   "f2D[3][0]" },
  { "vararr", "Var size",    "fVarArr" },
  { "vecarr", "std::vector", "fVectorD" },
  { nullptr }
};

namespace Podd::Tests {

//_____________________________________________________________________________
// ArrayRTTI constructor. Initializes array data.
ArrayRTTI::ArrayRTTI( const char* name, const char* description )
  : UnitTest(name, description)
  , fArray{}
  , f2D{}
  , fN{kVarSize}
  , fVarArr{new Data_t[fN]}
{
  // Constructor. Initialize data. The data are completely arbitrary, but
  // obviously must match the tests in ArrayRTTI_t.

  // Fixed-size arrays
  for( Int_t i = 0; i < k1Dsize; ++i )
    fArray[i] = 0.25 + Data_t(i);

  for( Int_t i = 0; i < k2Dcols; ++i ) {
    for( Int_t j = 0; j < k2Drows; ++j ) {
      f2D[i][j] = 0.33 + Data_t(k2Drows * i + j);
    }
  }
  // Variable-sized arrays
  for( Int_t i = 0; i < fN; ++i )
    fVarArr[i] = 0.75 + Data_t(i);

  fVectorD.clear();
  fVectorD.reserve(kVecSize);
  for( Int_t i = 0; i < kVecSize; ++i )
    fVectorD.push_back(-11.652 + Data_t(2 * i));
}

//_____________________________________________________________________________
// Destructor. Free memory and remove variables from global list.
ArrayRTTI::~ArrayRTTI()
{
  RemoveVariables();
  delete [] fVarArr;
}

//_____________________________________________________________________________
// Make the variable definitions retrievable through a function so that the
// tests have access to them.
const RVarDef* ArrayRTTI::GetRVarDef()
{
  return vars;
}

//_____________________________________________________________________________
// Return number of variables that this object should define
Int_t ArrayRTTI::GetNvars()
{
  constexpr Int_t sz = sizeof(vars) / sizeof(vars[0]) - 1;
  return sz;
}

//_____________________________________________________________________________
// Define/delete global variables. This is the functionality to be tested.
Int_t ArrayRTTI::DefineVariables( EMode mode )
{
  return DefineVarsFromList(ArrayRTTI::GetRVarDef(), mode);
}

//_____________________________________________________________________________

} // namespace Podd::Tests

////////////////////////////////////////////////////////////////////////////////

ClassImp(Podd::Tests::ArrayRTTI)
