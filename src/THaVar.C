//*-- Author :    Ole Hansen   26/04/2000


//////////////////////////////////////////////////////////////////////////
//
// THaVar
//
// A "global variable".  This is essentially a read-only pointer to the 
// actual data, along with support for data types and arrays.  It can
// be used to retrieve data from anywhere in memory, e.g. analysis results 
// of some detector object.
//
// THaVar objects are managed by the THaVarList.
// 
// The standard way to define a variable is via the constructor, e.g.
//
//    Double_t x = 1.5;
//    THaVar v("x","variable x",x);
//
// Constructors and Set() determine the data type automatically.
// Supported types are 
//    Double_t, Float_t, Long_t, ULong_t, Int_t, UInt_t, 
//    Short_t, UShort_t, Char_t, Byte_t
//
// Arrays can be defined as follows:
//
//    Double_t* a = new Double_t[10];
//    THaVar* va = new THaVar("a[10]","array of doubles",a);
//
// One-dimensional variable-size arrays are also supported. The actual
// size must be contained in a variable of type Int_t. 
// NOTE: This feature may still be buggy. Use with caution.
// Example:
//
//    Double_t* a = new Double_t[20];
//    Int_t size;
//    GetValues( a, size );  //Fills a and sets size to number of data.
//    THaVar* va = new THaVar("a","var size array",a,&size);
//
// Any array size given in the name string is ignored (e.g. "a[20]"
// would have the same effect as "a"). The data are always interpreted
// as a one-dimensional array.
//
// Data should normally be retrieved using GetValue(), which always
// returns Double_t.  
// If no type conversion is desired, or for maximum
// efficiency, one can use the typed GetValue methods.  However, this
// should be done with extreme care; a type mismatch could return
// just garbage without warning.
//
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
#include "THaVar.h"

ClassImp(THaVar)

//_____________________________________________________________________________
THaVar& THaVar::operator=( const THaVar& rhs )
{
  // Assignment operator. Must be explicit due to limitations of CINT.

  if( this != &rhs ) {
    TNamed::operator=(rhs);
    fValueD    = rhs.fValueD;  // assignment of pointer ok in this case
    fArrayData = rhs.fArrayData;
    fType      = rhs.fType;
    fCount     = rhs.fCount;
  }
  return *this;
}

//_____________________________________________________________________________
void THaVar::Copy( TObject& rhs )
{
  // Copy this object to rhs

  TNamed::Copy(rhs);
  ((THaVar&)rhs).fValueD    = fValueD;
  ((THaVar&)rhs).fArrayData = fArrayData;
  ((THaVar&)rhs).fType      = fType;
  ((THaVar&)rhs).fCount     = fCount;
}

//_____________________________________________________________________________
const char* THaVar::GetTypeName() const
{
  static const char* const type[] = { 
    "Double_t", "Float_t", "Long_t", "ULong_t", "Int_t", "UInt_t", "Short_t", 
    "UShort_t", "Char_t", "Byte_t", 
    "Double_t*", "Float_t*", "Long_t*", "ULong_t*", "Int_t*", "UInt_t*", 
    "Short_t*", "UShort_t*", "Char_t*", "Byte_t*" };

  return type[fType];
}

//_____________________________________________________________________________
size_t THaVar::GetTypeSize() const
{
  static const size_t size[] = { 
    sizeof(Double_t), sizeof(Float_t), sizeof(Long_t), sizeof(ULong_t), 
    sizeof(Int_t), sizeof(UInt_t), sizeof(Short_t), sizeof(UShort_t), 
    sizeof(Char_t), sizeof(Byte_t),
    sizeof(Double_t), sizeof(Float_t), sizeof(Long_t), sizeof(ULong_t), 
    sizeof(Int_t), sizeof(UInt_t), sizeof(Short_t), sizeof(UShort_t), 
    sizeof(Char_t), sizeof(Byte_t) };

  return size[fType];
}

//_____________________________________________________________________________
Int_t THaVar::Index( const THaArrayString& elem ) const
{
  // Return linear index into this array variable corresponding 
  // to the array element described by 'elem'.  
  // Return -1 if subscript(s) out of bound(s) or -2 if incompatible arrays.

  if( elem.IsError() ) return -1;
  if( !elem.IsArray() ) return 0;

  if( fCount ) {
    if( elem.GetNdim() != 1 ) return -2;
    return *elem.GetDim();
  }

  Byte_t ndim = fArrayData.GetNdim();
  if( ndim != elem.GetNdim() ) return -2;

  const Int_t *subs = elem.GetDim(), *adim = fArrayData.GetDim();

  Int_t index = subs[0];
  for( Byte_t i = 0; i<ndim; i++ ) {
    if( subs[i]+1 > adim[i] ) return -1;
    if( i>0 )
      index = index*adim[i] + subs[i];
  }
  if( index >= fArrayData.GetLen() || index > kMaxInt ) return -1;
  return index;
}

//_____________________________________________________________________________
Int_t THaVar::Index( const char* s ) const
{
  // Return linear index into this array variable corresponding 
  // to the array element described by the string 's'.
  // 's' must be either a single integer subscript (for a 1-d array) 
  // or a comma-separated list of subscripts (for multi-dimensional arrays).
  //
  // NOTE: This method is vastly less efficient than THaVar::Index( THaArraySring& )
  // above because the string has to be parsed first.
  //
  // Return -1 if subscript(s) out of bound(s) or -2 if incompatible arrays.

  size_t len = strlen(s);
  if( len == 0 ) return 0;

  char* str = new char[ len+4 ];
  str[0] = 't';
  str[1] = '[';
  str[len+2] = ']';
  str[len+3] = '\0';
  strncpy( str+2, s, len );
  THaArrayString elem(str);
  delete [] str;

  return Index( elem );
}

//_____________________________________________________________________________
void THaVar::Print(Option_t* option) const
{
  //Print a description of this variable

  TNamed::Print(option);

  if( strcmp( option, "FULL" )) return;

  cout << "(" << GetTypeName() << ")[";
  if( fCount ) cout << "*";
  cout << GetLen() << "]";
  for( int i=0; i<GetLen(); i++ ) {
    cout << "  ";
    if( IsPointerArray() && (!fValueDD || !fValueDD[i]) ) {
      cout << "???";
      continue;
    }
    switch( fType ) {
    case kDouble:
      cout << fValueD[i]; break;
    case kFloat:
      cout << fValueF[i]; break;
    case kLong:
      cout << fValueL[i]; break;
    case kULong:
      cout << fValueX[i]; break;
    case kInt:
      cout << fValueI[i]; break;
    case kUInt:
      cout << fValueU[i]; break;
    case kShort:
      cout << fValueS[i]; break;
    case kUShort:
      cout << fValueW[i]; break;
    case kChar:
      cout << static_cast<Short_t>(fValueC[i]);  // cast to force numeric output
      break;
    case kByte:
      cout << static_cast<UShort_t>(fValueB[i]); 
      break;
    case kDoubleP:
      cout << *fValueDD[i]; break;
    case kFloatP:
      cout << *fValueFF[i]; break;
    case kLongP:
      cout << *fValueLL[i]; break;
    case kULongP:
      cout << *fValueXX[i]; break;
    case kIntP:
      cout << *fValueII[i]; break;
    case kUIntP:
      cout << *fValueUU[i]; break;
    case kShortP:
      cout << *fValueSS[i]; break;
    case kUShortP:
      cout << *fValueWW[i]; break;
    case kCharP:
      cout << static_cast<Short_t>(*fValueCC[i]);  // cast to force numeric output
      break;
    case kByteP:
      cout << static_cast<UShort_t>(*fValueBB[i]); 
      break;
    default:
      break;
    }
  }
  cout << endl;
}

//_____________________________________________________________________________
void THaVar::SetName( const char* name )
{
  // Set the name of the variable

  TNamed::SetName( name );
  fArrayData = name;
}

//_____________________________________________________________________________
void THaVar::SetNameTitle( const char* name, const char* descript )
{
  // Set name and description of the variable

  TNamed::SetNameTitle( name, descript );
  fArrayData = name;
}
