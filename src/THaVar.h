#ifndef ROOT_THaVar
#define ROOT_THaVar

//////////////////////////////////////////////////////////////////////////
//
// THaVar
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "THaArrayString.h"
#include "VarType.h"
#include <cstddef>

class TMethodCall;

class THaVar : public TNamed {
  
public:
  static const Int_t    kInvalidInt;
  static const Double_t kInvalid;

  THaVar() : 
    fValueP(NULL), fType(kDouble), fCount(NULL), fOffset(-1), 
    fMethod(NULL) {}
  THaVar( const THaVar& rhs );
  THaVar& operator=( const THaVar& );
  virtual ~THaVar();

  THaVar( const char* name, const char* descript, const Double_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueD(&var), fType(kDouble),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Float_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueF(&var), fType(kFloat),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Long_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueL(&var), fType(kLong),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const ULong_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueX(&var), fType(kULong),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Int_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueI(&var), fType(kInt),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const UInt_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueU(&var), fType(kUInt),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Short_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueS(&var), fType(kShort),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const UShort_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueW(&var), fType(kUShort),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Char_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueC(&var), fType(kChar),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Byte_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueB(&var), fType(kByte),
    fCount(count), fOffset(-1), fMethod(NULL) {}

  THaVar( const char* name, const char* descript, const Double_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueDD(&var), fType(kDoubleP),
    fCount(count), fOffset(-1), fMethod(NULL) {}

  THaVar( const char* name, const char* descript, const Float_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueFF(&var), fType(kFloatP),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Long_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueLL(&var), fType(kLongP),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const ULong_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueXX(&var), fType(kULongP),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Int_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueII(&var), fType(kIntP),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const UInt_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueUU(&var), fType(kUIntP),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Short_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueSS(&var), fType(kShortP),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const UShort_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueWW(&var), fType(kUShortP),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Char_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueCC(&var), fType(kCharP),
    fCount(count), fOffset(-1), fMethod(NULL) {}
  THaVar( const char* name, const char* descript, const Byte_t*& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueBB(&var), fType(kByteP),
    fCount(count), fOffset(-1), fMethod(NULL) {}

  THaVar( const char* name, const char* desc, const void* obj,
	  VarType type, Int_t offset, TMethodCall* method=NULL, 
	  const Int_t* count=NULL ) :
    TNamed(name,desc), fArrayData(name), fObject(obj), fType(type),
    fCount(count), fOffset(offset), fMethod(method) {}

  virtual const char*  GetName() const { return fArrayData.GetName(); }

  Int_t           GetLen()       const;
  Byte_t          GetNdim()      const            
    { return ( fCount != NULL || fOffset != -1 ) ? 1 : fArrayData.GetNdim(); } 
  const Int_t*    GetDim()       const;
  VarType         GetType()      const { return fType; }
  size_t          GetTypeSize()  const { return GetTypeSize( fType ); }
  static size_t   GetTypeSize( VarType type );
  const char*     GetTypeName()  const { return GetTypeName( fType ); }
  static const char* GetTypeName( VarType type );

  Double_t        GetValue( Int_t i = 0 )  const { return GetValueAsDouble(i); }
  const void*     GetValuePointer()        const { return fValueP; }

  virtual ULong_t Hash() const { return fArrayData.Hash(); }
  virtual Bool_t  HasSameSize( const THaVar& rhs ) const;
  virtual Bool_t  HasSameSize( const THaVar* rhs ) const;
  virtual Int_t   Index( const char* ) const;
  virtual Int_t   Index( const THaArrayString& ) const;
  Bool_t          IsArray() const            
    { return ( fCount != NULL || fOffset != -1 ) ? kTRUE : fArrayData.IsArray(); }
  Bool_t          IsBasic() const
    { return ( fOffset == -1 && fMethod == NULL ); }
  Bool_t          IsPointerArray() const 
    { return ( IsArray() && fType>=kDouble2P ); }
  virtual void    Print( Option_t* opt="FULL" ) const;

  // The following are necessary to initialize empty THaVars such as those in arrays, 
  // where each element was constructed with the default constructor
  void SetVar( const Double_t& var ) { fValueD = &var; fType = kDouble; }
  void SetVar( const Float_t& var )  { fValueF = &var; fType = kFloat; }
  void SetVar( const Long_t& var )   { fValueL = &var; fType = kLong; }
  void SetVar( const ULong_t& var )  { fValueX = &var; fType = kULong; }
  void SetVar( const Int_t& var )    { fValueI = &var; fType = kInt; }
  void SetVar( const UInt_t& var )   { fValueU = &var; fType = kUInt; }
  void SetVar( const Short_t& var )  { fValueS = &var; fType = kShort; }
  void SetVar( const UShort_t& var ) { fValueW = &var; fType = kUShort; }
  void SetVar( const Char_t& var )   { fValueC = &var; fType = kChar; }
  void SetVar( const Byte_t& var )   { fValueB = &var; fType = kByte; }

  void SetVar( const Double_t*& var ) { fValueDD = &var; fType = kDoubleP; }
  void SetVar( const Float_t*& var )  { fValueFF = &var; fType = kFloatP; }
  void SetVar( const Long_t*& var )   { fValueLL = &var; fType = kLongP; }
  void SetVar( const ULong_t*& var )  { fValueXX = &var; fType = kULongP; }
  void SetVar( const Int_t*& var )    { fValueII = &var; fType = kIntP; }
  void SetVar( const UInt_t*& var )   { fValueUU = &var; fType = kUIntP; }
  void SetVar( const Short_t*& var )  { fValueSS = &var; fType = kShortP; }
  void SetVar( const UShort_t*& var ) { fValueWW = &var; fType = kUShortP; }
  void SetVar( const Char_t*& var )   { fValueCC = &var; fType = kCharP; }
  void SetVar( const Byte_t*& var )   { fValueBB = &var; fType = kByteP; }

  void SetVar( const Double_t**& var ) { fValue3D = &var; fType = kDouble2P; }
  void SetVar( const Float_t**& var )  { fValue3F = &var; fType = kFloat2P; }
  void SetVar( const Long_t**& var )   { fValue3L = &var; fType = kLong2P; }
  void SetVar( const ULong_t**& var )  { fValue3X = &var; fType = kULong2P; }
  void SetVar( const Int_t**& var )    { fValue3I = &var; fType = kInt2P; }
  void SetVar( const UInt_t**& var )   { fValue3U = &var; fType = kUInt2P; }
  void SetVar( const Short_t**& var )  { fValue3S = &var; fType = kShort2P; }
  void SetVar( const UShort_t**& var ) { fValue3W = &var; fType = kUShort2P; }
  void SetVar( const Char_t**& var )   { fValue3C = &var; fType = kChar2P; }
  void SetVar( const Byte_t**& var )   { fValue3B = &var; fType = kByte2P; }

  virtual void    SetName( const char* );
  virtual void    SetNameTitle( const char* name, const char* descript );

protected:
  Double_t            GetValueAsDouble( Int_t i=0 ) const;
  Double_t            GetValueFromObject( Int_t i=0 ) const;
  const Int_t*        GetObjArrayLenPtr() const;

  THaArrayString      fArrayData; //Variable name and array dimension(s), if any
  union {
    const Double_t*   fValueD;  //Pointers to data (scalar or stack array)
    const Float_t*    fValueF;
    const Long_t*     fValueL;
    const ULong_t*    fValueX;
    const Int_t*      fValueI;
    const UInt_t*     fValueU;
    const Short_t*    fValueS;
    const UShort_t*   fValueW;
    const Char_t*     fValueC;
    const Byte_t*     fValueB;
    const Double_t**  fValueDD;  //Pointers to pointers to array of data
    const Float_t**   fValueFF;  //(fixed or variable heap arrays)
    const Long_t**    fValueLL;
    const ULong_t**   fValueXX;
    const Int_t**     fValueII;
    const UInt_t**    fValueUU;
    const Short_t**   fValueSS;
    const UShort_t**  fValueWW;
    const Char_t**    fValueCC;
    const Byte_t**    fValueBB;
    const Double_t*** fValue3D;  //Pointers to pointers to array of pointers
    const Float_t***  fValue3F;  //to data (non-contiguous arrays)
    const Long_t***   fValue3L;
    const ULong_t***  fValue3X;
    const Int_t***    fValue3I;
    const UInt_t***   fValue3U;
    const Short_t***  fValue3S;
    const UShort_t*** fValue3W;
    const Char_t***   fValue3C;
    const Byte_t***   fValue3B;
    const void*       fValueP;
    const void*       fObject;   //Pointer to ROOT object containing data
    const void**      fObjectP;  //Indirect pointer to ROOT object containing the data
  };
  VarType             fType;     //Data type (see VarType.h)
  const Int_t*        fCount;    //Pointer to array size for var size array

  Int_t               fOffset;   //Offset of data w.r.t. object pointer
  TMethodCall*        fMethod;   //Member function to access data in object

  ClassDef(THaVar,0)   //Global symbolic variable
};

//_______________ Inlines _____________________________________________________
inline
Int_t THaVar::GetLen() const
{ 
  if( fCount )
    return *fCount;
  if( fOffset != -1 ) {
    const Int_t* pi = GetObjArrayLenPtr();
    return pi ? *pi : kInvalidInt;
  }
  return fArrayData.GetLen();
}

//_____________________________________________________________________________
inline
const Int_t* THaVar::GetDim() const
{
  if( fCount )
    return fCount;
  if( fOffset != - 1 )
    return GetObjArrayLenPtr();
  return fArrayData.GetDim();
}

//_____________________________________________________________________________
inline
Double_t THaVar::GetValueAsDouble( Int_t i ) const
{
  //  if( ( fValueP == NULL ) || 
  //    ( fType>=kDoubleP  && *fDoubleDD == NULL ) ||
  //    ( fType>=kDouble2P && (*fDouble3D)[i] == NULL ))
  //  return 0.0;
  if( !IsBasic() )
    return GetValueFromObject( i );

#ifdef WITH_DEBUG
  if( i<0 || i>=GetLen() ) {
    Warning("GetValue()", "Whoa! Index out of range, variable %s, index %d",
	    GetName(), i );
    return kInvalid;
  }
#endif

  switch( fType ) {
  case kDouble: 
    return fValueD[i];
  case kFloat:
    return static_cast<Double_t>( fValueF[i] );
  case kLong:
    return static_cast<Double_t>( fValueL[i] );
  case kULong:
    return static_cast<Double_t>( fValueX[i] );
  case kInt:
    return static_cast<Double_t>( fValueI[i] );
  case kUInt:
    return static_cast<Double_t>( fValueU[i] );
  case kShort:
    return static_cast<Double_t>( fValueS[i] );
  case kUShort:
    return static_cast<Double_t>( fValueW[i] );
  case kChar:
    return static_cast<Double_t>( fValueC[i] );
  case kByte:
    return static_cast<Double_t>( fValueB[i] );

  case kDoubleP: 
    return (*fValueDD)[i];
  case kFloatP:
    return static_cast<Double_t>( (*fValueFF)[i] );
  case kLongP:
    return static_cast<Double_t>( (*fValueLL)[i] );
  case kULongP:
    return static_cast<Double_t>( (*fValueXX)[i] );
  case kIntP:
    return static_cast<Double_t>( (*fValueII)[i] );
  case kUIntP:
    return static_cast<Double_t>( (*fValueUU)[i] );
  case kShortP:
    return static_cast<Double_t>( (*fValueSS)[i] );
  case kUShortP:
    return static_cast<Double_t>( (*fValueWW)[i] );
  case kCharP:
    return static_cast<Double_t>( (*fValueCC)[i] );
  case kByteP:
    return static_cast<Double_t>( (*fValueBB)[i] );

  case kDouble2P: 
    return *((*fValue3D)[i]);
  case kFloat2P:
    return static_cast<Double_t>( *((*fValue3F)[i]) );
  case kLong2P:
    return static_cast<Double_t>( *((*fValue3L)[i]) );
  case kULong2P:
    return static_cast<Double_t>( *((*fValue3X)[i]) );
  case kInt2P:
    return static_cast<Double_t>( *((*fValue3I)[i]) );
  case kUInt2P:
    return static_cast<Double_t>( *((*fValue3U)[i]) );
  case kShort2P:
    return static_cast<Double_t>( *((*fValue3S)[i]) );
  case kUShort2P:
    return static_cast<Double_t>( *((*fValue3W)[i]) );
  case kChar2P:
    return static_cast<Double_t>( *((*fValue3C)[i]) );
  case kByte2P:
    return static_cast<Double_t>( *((*fValue3B)[i]) );
  default:
    ;
  }
  return kInvalid;
}

#endif

