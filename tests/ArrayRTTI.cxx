//*-- Author :    Ole Hansen   12-Nov-14
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ArrayRTTI - Test defnition of global variables of array elements via RTTI //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "ArrayRTTI.h"
#include "THaVar.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TString.h"
#include "TMath.h"

using namespace std;

static RVarDef vars[] = {
  { "oned",   "1D array",  "fArray" },
  { "elem0",  "fArray[0]", "fArray[0]" },
  { "elem1",  "fArray[1]", "fArray[1]" },
  { "twod",   "2D array",  "f2D" },
  { "elem02", "f2D[0][2]", "f2D[0][2]" },
  { "elem30", "f2D[3][0]", "f2D[3][0]" },
  { "vararr", "Var size",  "fVarArr" },
  { 0 }
};

#define NDIG 5

//_____________________________________________________________________________
ArrayRTTI::ArrayRTTI( const char* name, const char* description ) :
  THaAnalysisObject(name,description), fN(0), fVarArr(0)
{
  // Constructor. Initialize fixed variables with with kBig

  for( Int_t i = 0; i < fgD1; ++i )
    fArray[i] = kBig;

  for( Int_t i = 0; i < fgD21; ++i ) {
    for( Int_t j = 0; j < fgD22; ++j ) {
      f2D[i][j] = kBig;
    }
  }
}

//_____________________________________________________________________________
ArrayRTTI::~ArrayRTTI()
{
  // Destructor. Remove variables from global list.

  delete [] fVarArr;
  RemoveVariables();
}

//_____________________________________________________________________________
Int_t ArrayRTTI::DefineVariables( EMode mode )
{
  // Define (or delete) global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
Int_t ArrayRTTI::ReadDatabase( const TDatime& date )
{
  // Initialize test parameters

  for( Int_t i = 0; i < fgD1; ++i )
    fArray[i] = 0.25 + Float_t(i);

  for( Int_t i = 0; i < fgD21; ++i ) {
    for( Int_t j = 0; j < fgD22; ++j ) {
      f2D[i][j] = 0.33 + Float_t(fgD22*i + j);
    }
  }

  delete [] fVarArr;
  fN = fgDV;
  fVarArr = new Float_t[fN];
  for( Int_t i = 0; i < fN; ++i )
    fVarArr[i] = 0.75 + Float_t(i);

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
static Float_t Round( Double_t val, Int_t n )
{
  // Round to n+1 digits
  if( val == 0 )
    return 0;
  if( n < 0 )
    n = 0;
  Float_t sign = ((val < 0) ? -1 : 1);
  Double_t norm = TMath::Power(10,n-TMath::FloorNint(TMath::Log10(val)));
  return sign * Int_t(TMath::Abs(val)*norm+0.5)/norm;
}

//_____________________________________________________________________________
Int_t ArrayRTTI::Test() const
{
  // Test for expected behavior at run time

  const char* const here = "Test";

  if( !fIsInit || !fIsSetup || !IsOK() ) {
    Error( Here(here), "Not initialized. Call Init() first." );
    return -1;
  }

  TString prefix(GetPrefix());
  RVarDef* item = vars;
  while( item && item->name ) {
    TString name = item->name;
    TString varname = prefix+name;
    if( fDebug > 0 )
      Info( Here(here), "Testing variable %s", varname.Data() );

    // General tests
    THaVar* var = gHaVars->Find(varname.Data());
    if( var == 0 ) {
      Error( Here(here), "Variable %s not defined",
	     varname.Data() );
      return 1;
    }
    if( fDebug > 1 )
      var->Print("FULL");

    // Group & individual tests
    if( (name == "oned" || name == "twod" || name == "vararr") ) {
      if( !var->IsArray() ) {
	Error( Here(here), "Variable %s is not an array",
	       varname.Data() );
	return 2;
      }
      if( var->IsPointerArray() ) {
	Error( Here(here), "Variable %s is an array of pointers",
	       varname.Data() );
	return 3;
      }
      const Int_t* dim = var->GetDim();
      if( dim == 0 ) {
	Error( Here(here), "Variable %s does not have array size info "
	       "GetDim()", varname.Data() );
	return 4;
      }
      if( name == "oned" ) {
	if( var->GetNdim() != 1 ) {
	  Error( Here(here), "Variable %s is not a 1-d array",
		 varname.Data() );
	  return 5;
	}
	if( var->GetType() != kFloat ) {
	  Error( Here(here), "Variable %s is not a kFloat",
		 varname.Data() );
	  return 6;
	}
	if( dim[0] != fgD1 || var->GetLen() != fgD1 ) {
	  Error( Here(here), "Variable %s has wrong size/len = %d/%d, "
		 "expected %d",
		 varname.Data(), dim[0], var->GetLen(), fgD1 );
	  return 7;
	}
	for( Int_t i = 0; i < var->GetLen(); ++i ) {
	  Float_t expect = 0.25 + Float_t(i);
	  Float_t val = var->GetValue(i);
	  if( Round(val-expect,NDIG) != 0  ) {
	    Error( Here(here), "Element %s[%d] has wrong value = %f, "
		   "expected %f", varname.Data(), i, val, expect );
	    return 8;
	  }
	}
      } // end "oned"

      else if( name == "twod" ) {
	if( var->GetNdim() != 2 ) {
	  Error( Here(here), "Variable %s is not a 2-d array",
		 varname.Data() );
	  return 10;
	}
	if( var->GetType() != kFloat ) {
	  Error( Here(here), "Variable %s is not a kFloat",
		 varname.Data() );
	  return 11;
	}
	if( dim[0] != fgD21 || dim[1] != fgD22 ||
	    var->GetLen() != fgD21*fgD22 ) {
	  Error( Here(here), "Variable %s has wrong size/len = [%d][%d]/%d, "
		 "expected [%d][%d]/%d",
		 varname.Data(), dim[0], dim[1], var->GetLen(),
		 fgD21, fgD22, fgD21*fgD22 );
	  return 12;
	}
	Int_t idx = 0;
	for( Int_t i = 0; i < dim[0]; ++i ) {
	  for( Int_t j = 0; j < dim[1]; ++j ) {
	    Float_t expect = 0.33 + Float_t(dim[1]*i + j);
	    Float_t val = var->GetValue(idx);
	    if( Round(val-expect,NDIG) != 0 ) {
	      Error( Here(here), "Element %s[%d][%d] has wrong value = %f, "
		     "expected %f", varname.Data(), i, j, val, expect );
	      return 13;
	    }
	    ++idx;
	  }
	}
      } // end "twod"

      else {
	if( var->GetNdim() != 1 ) {
	  Error( Here(here), "Variable %s is not a 1-d array",
		 varname.Data() );
	  return 15;
	}
	if( var->GetType() != kFloatP ) {
	  Error( Here(here), "Variable %s is not a kFloatP",
		 varname.Data() );
	  return 16;
	}
	if( dim[0] != fgDV || var->GetLen() != fgDV ) {
	  Error( Here(here), "Variable %s has wrong size/len = %d/%d, "
		 "expected %d",
		 varname.Data(), dim[0], var->GetLen(), fgDV );
	  return 17;
	}
	for( Int_t i = 0; i < var->GetLen(); ++i ) {
	  Float_t expect = 0.75 + Float_t(i);
	  Float_t val = var->GetValue(i);
	  if( Round(val-expect,NDIG) != 0 ) {
	    Error( Here(here), "Element %s[%d] has wrong value = %f, "
		   "expected %f", varname.Data(), i, val, expect );
	    return 18;
	  }
	}
      } // end "vararr"

    } // end testing arrays

    else if( name == "elem0"  || name == "elem1" ||
	     name == "elem02" || name == "elem30" ) {
      if( var->IsArray() ) {
	Error( Here(here), "Variable %s is not a scalar",
	       varname.Data() );
	return 20;
      }
      if( var->GetType() != kFloat ) {
	Error( Here(here), "Variable %s is not a kFloatP",
	       varname.Data() );
	return 21;
      }
      Float_t val = var->GetValue(), expect = -kBig;
      if( name == "elem0" ) {
	expect = 0.25;
      } else if( name == "elem1" ) {
	expect = 1.25;
      } else if( name == "elem02" ) {
	expect = 0.33 + Float_t(fgD22*0 + 2);
      } else if( name == "elem30" ) {
	expect = 0.33 + Float_t(fgD22*3 + 0);
      }
      if( Round(val-expect,NDIG) != 0 ) {
	Error( Here(here), "Variable %s has wrong value = %f, "
	       "expected %f", varname.Data(), val, expect );
	return 22;
      }
    } // end testing scalars

    else {
      Error( Here(here), "No test defined for variable %s",
	     varname.Data() );
      return -1;
    }

    ++item;
  }
  return 0;

}

////////////////////////////////////////////////////////////////////////////////

ClassImp(ArrayRTTI)
