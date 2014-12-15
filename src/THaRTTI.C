//*-- Author :    Ole Hansen   28/01/02

//////////////////////////////////////////////////////////////////////////
//
// THaRTTI
//
// RTTI information for a member variable of a ROOT class.
// This is a utility class used internally by THaVarList.
//
//////////////////////////////////////////////////////////////////////////

#include "THaRTTI.h"
#include "THaVar.h"
#include "TROOT.h"
#include "TClass.h"
#include "TClassEdit.h"
#include "TDataMember.h"
#include "TDataType.h"
#include "TRealData.h"
#include <iostream>
#include <cassert>

using namespace std;

static TObject* FindRealDataVar( TList* lrd, const TString& var );

//_____________________________________________________________________________
Int_t THaRTTI::Find( TClass* cl, const TString& var,
		     const void* const prototype )
{
  // Get RTTI info for member variable 'var' of ROOT class 'cl'
  // 'prototype' is a pointer to an object of class 'cl' and must
  // be specified if the class does not have a default constructor.

  if( !cl )
    return -1;

  // Get the object's list TRealData.  Unlike the list of TDataMembers,
  // this list handles inheritance (i.e. it includes base class data
  // members), and TRealData allows us to get any variable's offset.
  // The main limitation is that data members have to be persistent.

  TList* lrd = cl->GetListOfRealData();
  if( !lrd ) {
    // FIXME: Check if const_cast is appropriate here
    // - prototype must not be modified
    cl->BuildRealData( const_cast<void*>(prototype) );
    lrd = cl->GetListOfRealData();
    if( !lrd )
      return -1;
  }

  // Parse possible array subscript
  THaArrayString avar(var);
  if( avar.IsError() )
    return -1;
  Int_t element_requested = -1;

  // Variable names in TRealData are stored along with pointer prefixes (*)
  // and array subscripts, so we have to use a customized search function:
  TRealData* rd = static_cast<TRealData*>( FindRealDataVar( lrd, avar ) );
  if( !rd )
    return -1;

  // Get the TDataMember, which holds the really useful information
  TDataMember* m = rd->GetDataMember();
  if( !m )
    return -1;

  VarType type;
  if( m->IsBasic() || m->IsEnum() ) {
    TString typnam( m->GetTypeName() );
    if( typnam == "Double_t" )
      type = kDouble;
    else if( typnam == "Float_t" || typnam == "Real_t" )
      type = kFloat;
    else if( typnam == "Int_t" )
      type = kInt;
    else if( typnam == "UInt_t" )
      type = kUInt;
    else if( typnam == "Short_t" )
      type = kShort;
    else if( typnam == "UShort_t" )
      type = kUShort;
    else if( typnam == "Long_t" )
      type = kLong;
    else if( typnam == "ULong_t" )
      type = kULong;
    else if( typnam == "Char_t" || typnam == "Text_t" )
      type = kChar;
    else if( typnam == "Byte_t" || typnam == "UChar_t" ||
	     typnam == "Bool_t" )
      type = kByte;
    else if( m->IsEnum() )
      // Enumeration types are all treated as integers
      type = kInt;
    else
      return -1;
    // Pointers are flagged as pointer types. The way THaVar works, this means:
    // - Simple pointers will be dereferenced when reading the variable.
    //   This avoids dangling pointers.
    // - For pointers to pointers, the value of the base pointer will be
    //   memorized. Therefore, if the base pointer changes, the variable
    //   will become invalid.
    if( m->IsaPointer() )
      // Simple, but depends on the definition order in VarType.h
      type = (VarType)(type + kDoubleP);

  } else if( m->IsSTLContainer() ) {
    // STL containers: only std::vector supported
    if( m->IsSTLContainer() != TClassEdit::kVector )
      return -1;
    // Get the vector element type name
    TString typnam( m->GetTypeName() );
    Ssiz_t lbrk = typnam.Index( "<" );
    Ssiz_t rbrk = typnam.Index( ">" );
    if( lbrk == kNPOS || rbrk == kNPOS || lbrk+1 >= rbrk )
      return -1;
    typnam = typnam(lbrk+1,rbrk-lbrk-1);
    if( typnam == "Int_t" || typnam == "int" )
      type = kIntV;
    else if( typnam == "UInt_t" || typnam == "unsigned int" )
      type = kUIntV;
    else if( typnam == "Float_t" || typnam == "float" || typnam == "Real_t" )
      type = kFloatV;
    else if( typnam == "Double_t" || typnam == "double" )
      type = kDoubleV;
    else
      return -1;

  } else {
    type = (m->IsaPointer()) ? kObjectP : kObject;
  }

  // Check for arrays
  Int_t        array_dim    = m->GetArrayDim();
  const char*  array_index  = m->GetArrayIndex();
  Int_t        count_offset = -1;

  TString subscript( rd->GetName() );
  EArrayType atype = kScalar;

  // May only request an element of a matching explicitly-dimensioned array
  if( avar.IsArray() && array_dim != avar.GetNdim() )
    return -1;

  // If variable is explicitly dimensioned, ignore any "array index"
  // in the comment.
  if( array_dim > 0 ) {
    // Explicitly dimensioned arrays must not be pointers
    if( m->IsaPointer() )
      return -1;
    // Must be an array of a basic type
    if( !m->GetDataType() )
      return -1;
    if( avar.IsArray() ) {
      // If a specific element was requested, treat the variable as a scalar.
      // Calculate the linear index of the requested element.
      element_requested = avar[0];
      for( Int_t idim = 0; idim < array_dim; ++idim ) {
	if( avar[idim] >= m->GetMaxIndex(idim) ) // Out of bounds
	  return -1;
	if( idim > 0 ) {
	  element_requested *= m->GetMaxIndex(idim);
	  element_requested += avar[idim];
	}
      }
    } else {
      // Otherwise it's a fixed-size array
      Ssiz_t i = subscript.Index( "[" );
      if( i == kNPOS )
	return -1;
      subscript = subscript(i,subscript.Length()-i);
      atype = kFixed;
    }

  } else if( array_index && *array_index && m->IsaPointer() ) {
    // If no explicit dimensions given, but the variable is a pointer
    // and there is an array subscript given in the comment ( // [fN] ),
    // then we have a variable-size array.  The subscript variable in the
    // comment MUST be an Int_t. I guess that is a standard ROOT
    // convention anyway.

    // Must be an array of a basic type
    if( !m->GetDataType() )
      return -1;

    // See if the subscript variable exists and is an Int_t
    TString index(array_index);
    THaRTTI rtti;
    rtti.Find( cl, index, prototype );
    if( rtti.IsValid() && rtti.GetType() == kInt ) {
      // Looking good. Let's get the offset
      count_offset = rtti.GetOffset();
    }
    atype = kVariable;

  } else if( m->IsSTLContainer() ) {
    assert( m->IsSTLContainer() == TClassEdit::kVector );
    atype = kVector;
  }

  // Build the RTTI object

  fOffset     = rd->GetThisOffset();
  fType       = type;
  fArrayType  = atype;
  fDataMember = m;
  fRealData   = rd;

  switch( atype ) {
  case kScalar:
    if( element_requested >= 0 )
      fOffset += element_requested * m->GetDataType()->Size();
    break;
  case kFixed:
    fSubscript = subscript;
    break;
  case kVariable:
    fCountOffset = count_offset;
    break;
  case kVector:
    break;
  }

  return 0;
}

//_____________________________________________________________________________
static
TObject* FindRealDataVar( TList* lrd, const TString& var )
{
  // Search list of TRealData 'lrd' for a variable named 'var',
  // stripping pointer prefixes "*" and array subscripts.
  // Return corresponding TRealData entry if found, else 0.
  // Local function used by Find().

  TObjLink* lnk = lrd->FirstLink();
  while (lnk) {
    TObject* obj = lnk->GetObject();
    TString name( obj->GetName() );
    name = name.Strip( TString::kLeading, '*' );
    Ssiz_t i = name.Index( "[" );
    if( i != kNPOS )
      name = name(0,i);
    if( name == var )
      return obj;
    lnk = lnk->Next();
  }
  return 0;
}

//_____________________________________________________________________________
TClass* THaRTTI::GetClass() const
{
  // If this is an object, get its class

  if( IsObject() && fDataMember )
    return gROOT->GetClass( fDataMember->GetTypeName() );

  return NULL;
}

//_____________________________________________________________________________
Bool_t THaRTTI::IsPointer() const
{
  return fDataMember ? fDataMember->IsaPointer() : kFALSE;
}

//_____________________________________________________________________________
void THaRTTI::Print( Option_t* ) const
{
  // Print RTTI information

  cout << "Offset:       " << fOffset;
  if( fOffset == -1 )
    cout << " (not initialized)";
  cout << endl;
  if( fOffset == -1 )
    return;
  cout << "Type:         ";
  if( fType != kObject )
    cout << THaVar::GetTypeName( fType );
  else if( fDataMember )
    cout << fDataMember->GetFullTypeName();
  cout << endl;
  if( IsArray() ) {
    cout << "Array type:   ";
    switch( fArrayType ) {
    case kFixed:
      cout << "fixed" << endl;
      cout << "Dimension:    " << fSubscript.Data() << endl;
      break;
    case kVariable:
      cout << "variable" << endl;
      cout << "Count offset: " << fCountOffset << endl;
      break;
    default:
      cout << "(unknown)" << endl;
      break;
    }
  }
}
//_____________________________________________________________________________
ClassImp(THaRTTI)
