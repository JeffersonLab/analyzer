//*-- Author :    Ole Hansen   15/2/2016

//////////////////////////////////////////////////////////////////////////
//
// FixedArrayVar
//
// A "global variable" referencing a fixed size array of basic data.
//
//////////////////////////////////////////////////////////////////////////

#include "FixedArrayVar.h"
#include "THaVar.h"
#include "TError.h"
#include <iostream>
#include <cstring>
#include <typeinfo>

using namespace std;

namespace Podd {

//_____________________________________________________________________________
FixedArrayVar::FixedArrayVar( THaVar* pvar, const void* addr, VarType type )
  : Variable(pvar,addr,type), fParsedName(GetName())
{
  // Constructor

  if( !VerifyArrayName(fParsedName) ) {
    fValueP = 0;
    return;
  }

  fSelf->TNamed::SetName( fParsedName.GetName() );
}

//_____________________________________________________________________________
Bool_t FixedArrayVar::VerifyArrayName( const THaArrayString& parsed_name ) const
{
  // Return true if the variable string has valid array syntax

  const char* const here = "FixedArrayVar::VerifyArrayName";

  if( parsed_name.IsError() ) {
    fSelf->Error( here, "Malformed array specification for variable %s",
		  GetName() );
    return false;
  } else if( !parsed_name.IsArray() ) {
    fSelf->Error( here, "Specification \"%s\" for variable %s is not a "
		 "fixed array", parsed_name.GetName(), GetName() );
    return false;
  }

  return true;
}

//_____________________________________________________________________________
Int_t FixedArrayVar::GetLen() const
{
  // Get number of elements of the variable

  return fParsedName.GetLen();
}

//_____________________________________________________________________________
Int_t FixedArrayVar::GetNdim() const
{
  // Get number of elements of the variable

  return fParsedName.GetNdim();
}

//_____________________________________________________________________________
const Int_t* FixedArrayVar::GetDim() const
{
  // Return array of dimensions of the array. Scalers are always return a
  // pointer to 1 (as with array definition [1]).

  return fParsedName.GetDim();
}

//_____________________________________________________________________________
Bool_t FixedArrayVar::HasSameSize( const Variable& rhs ) const
{
  // Compare the size (=number of elements) of this variable to that of 'rhs'.

  if( typeid(*this) != typeid(rhs) )
    return kFALSE;

  const FixedArrayVar* other = dynamic_cast<const FixedArrayVar*>(&rhs);
  assert( other );
  if( !other )
    return kFALSE;

  return ( fParsedName.GetNdim() == other->fParsedName.GetNdim() &&
	   fParsedName.GetLen()  == other->fParsedName.GetLen() );
}

//_____________________________________________________________________________
Bool_t FixedArrayVar::IsContiguous() const
{
  // Data are contiguous in memory

  return !IsPointerArray();
}

//_____________________________________________________________________________
Bool_t FixedArrayVar::IsPointerArray() const
{
  // Data are an array of pointers to data

  return fType>=kDouble2P && fType <= kObject2P;
}

//_____________________________________________________________________________
Bool_t FixedArrayVar::IsVarArray() const
{
  // Variable is a variable-sized array

  return kFALSE;
}

//_____________________________________________________________________________
void FixedArrayVar::Print(Option_t* option) const
{
  // Print a description of this variable. If option=="FULL" (default), also
  // print current data

  fSelf->TNamed::Print(option);

  if( strcmp(option, "FULL") ) return;

  cout << "(" << GetTypeName() << ")=[";
  fParsedName.Print("dimonly");
  cout << "]";
  for( int i=0; i<GetLen(); i++ ) {
    cout << "  ";
    if( IsFloat() )
      cout << GetValue(i);
    else
      cout << GetValueInt(i);
  }
  cout << endl;
}

//_____________________________________________________________________________
void FixedArrayVar::SetName( const char* name )
{
  // Set the name of the variable

  THaArrayString new_name = name;
  if( !VerifyArrayName(new_name) )
    return;

  fSelf->TNamed::SetName( new_name );
  fParsedName = new_name;
}

//_____________________________________________________________________________
void FixedArrayVar::SetNameTitle( const char* name, const char* descript )
{
  // Set name and description of the variable

  THaArrayString new_name = name;
  if( !VerifyArrayName(new_name) )
    return;

  fSelf->TNamed::SetNameTitle( new_name, descript );
  fParsedName = new_name;
}

//_____________________________________________________________________________

} //namespace Podd
