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
#include "VarDef.h"  // for RVarDef
#include "Helper.h"
#include <vector>    // for vector, ssize

using namespace std;

namespace Podd::Tests {

//_____________________________________________________________________________
// Variable definitions. The setup of these through DefineVariables (below)
// is the functionality to be tested
//
// Make the variable definitions retrievable through a function so that the
// tests have access to them.
const vector<RVarDef>& ArrayRTTI::GetRVarDef()
{
  static const vector<RVarDef> vars = {
    { .name = "oned",   .desc = "1D array",    .def = "fArray"     },
    { .name = "elem0",  .desc = "fArray[0]",   .def = "fArray[0]"  },
    { .name = "elem1",  .desc = "fArray[1]",   .def = "fArray[1]"  },
    { .name = "twod",   .desc = "2D array",    .def = "f2D"        },
    { .name = "elem02", .desc = "f2D[0][2]",   .def = "f2D[0][2]"  },
    { .name = "elem30", .desc = "f2D[3][0]",   .def = "f2D[3][0]"  },
    { .name = "vararr", .desc = "Var size",    .def = "fVarArr"    },
    { .name = "vecarr", .desc = "std::vector", .def = "fVectorD"   },
    { .name = "vec.x",  .desc = "vector[n].x", .def = "fVectorS.x" },
    { .name = "vec.i",  .desc = "vector[n].i", .def = "fVectorS.i" },
  };
  return vars;
}

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

  fVectorS.reserve(kVecObjSize);
  for( Int_t i = 0; i < kVecObjSize; ++i )
    fVectorS.push_back({
      .x = 11.1 + 3*i,
      .i = 2*i + 1
    });
}

//_____________________________________________________________________________
// Destructor. Free memory and remove variables from global list.
ArrayRTTI::~ArrayRTTI()
{
  RemoveVariables();
  delete [] fVarArr;
}

//_____________________________________________________________________________
// Return number of variables that this object should define
Int_t ArrayRTTI::GetNvars()
{
  return ToInt(GetRVarDef().size());
}

//_____________________________________________________________________________
// Define/delete global variables. This is the functionality to be tested.
Int_t ArrayRTTI::DefineVariables( EMode mode )
{
  return DefineVarsFromList( GetRVarDef(), mode );
}

//_____________________________________________________________________________

} // namespace Podd::Tests

////////////////////////////////////////////////////////////////////////////////

ClassImp(Podd::Tests::ArrayRTTI)
