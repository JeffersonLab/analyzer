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

class THaVar : public TNamed {
  
public:
  THaVar() : fValueP(NULL), fType(kDouble), fCount(NULL) {}
  THaVar( const THaVar& rhs ) :
    TNamed( rhs ), fValueD(rhs.fValueD), fArrayData(rhs.fArrayData),
    fType(rhs.fType), fCount(rhs.fCount) {}
  THaVar& operator=( const THaVar& );
  virtual ~THaVar() {}

  THaVar( const char* name, const char* descript, const Double_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueD(&var), fType(kDouble),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Float_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueF(&var), fType(kFloat),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Long_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueL(&var), fType(kLong),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const ULong_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueX(&var), fType(kULong),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Int_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueI(&var), fType(kInt),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const UInt_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueU(&var), fType(kUInt),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Short_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueS(&var), fType(kShort),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const UShort_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueW(&var), fType(kUShort),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Char_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueC(&var), fType(kChar),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Byte_t& var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueB(&var), fType(kByte),
    fCount(count) {}

  THaVar( const char* name, const char* descript, const Double_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueD(var), fType(kDouble),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Float_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueF(var), fType(kFloat),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Long_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueL(var), fType(kLong),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const ULong_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueX(var), fType(kULong),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Int_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueI(var), fType(kInt),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const UInt_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueU(var), fType(kUInt),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Short_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueS(var), fType(kShort),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const UShort_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueW(var), fType(kUShort),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Char_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueC(var), fType(kChar),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Byte_t* var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueB(var), fType(kByte),
    fCount(count) {}

  THaVar( const char* name, const char* descript, const Double_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueDD(var), fType(kDoubleP),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Float_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueFF(var), fType(kFloatP),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Long_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueLL(var), fType(kLongP),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const ULong_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueXX(var), fType(kULongP),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Int_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueII(var), fType(kIntP),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const UInt_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueUU(var), fType(kUIntP),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Short_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueSS(var), fType(kShortP),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const UShort_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueWW(var), fType(kUShortP),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Char_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueCC(var), fType(kCharP),
    fCount(count) {}
  THaVar( const char* name, const char* descript, const Byte_t** var,
	  const Int_t* count = NULL ) :
    TNamed(name,descript), fArrayData(name), fValueBB(var), fType(kByteP),
    fCount(count) {}

  virtual void    Copy( TObject& );

  virtual const char*  GetName() const       { return fArrayData.GetName(); }

  Int_t           GetLen() const             
    { return fCount ? *fCount : fArrayData.GetLen(); }
  Byte_t          GetNdim() const            
    { return fCount ? 1 : fArrayData.GetNdim(); } 
  const Int_t*    GetDim() const             
    { return fCount ? fCount : fArrayData.GetDim(); }
  VarType         GetType() const            { return fType; }
  const char*     GetTypeName() const;
  size_t          GetTypeSize() const;

  Double_t        GetValue( Int_t i = 0 )  const { return GetValueAsDouble(i);}
  Double_t        GetValueD( Int_t i = 0 ) const { return fValueD[i]; }
  Float_t         GetValueF( Int_t i = 0 ) const { return fValueF[i]; }
  Long_t          GetValueL( Int_t i = 0 ) const { return fValueU[i]; }
  ULong_t         GetValueX( Int_t i = 0 ) const { return fValueX[i]; }
  Int_t           GetValueI( Int_t i = 0 ) const { return fValueI[i]; }
  UInt_t          GetValueU( Int_t i = 0 ) const { return fValueU[i]; }
  Short_t         GetValueS( Int_t i = 0 ) const { return fValueS[i]; }
  UShort_t        GetValueW( Int_t i = 0 ) const { return fValueW[i]; }
  Char_t          GetValueC( Int_t i = 0 ) const { return fValueC[i]; }
  Byte_t          GetValueB( Int_t i = 0 ) const { return fValueB[i]; }
  Double_t        GetValueDD( Int_t i = 0 ) const { return *fValueDD[i]; }
  Float_t         GetValueFF( Int_t i = 0 ) const { return *fValueFF[i]; }
  Long_t          GetValueLL( Int_t i = 0 ) const { return *fValueUU[i]; }
  ULong_t         GetValueXX( Int_t i = 0 ) const { return *fValueXX[i]; }
  Int_t           GetValueII( Int_t i = 0 ) const { return *fValueII[i]; }
  UInt_t          GetValueUU( Int_t i = 0 ) const { return *fValueUU[i]; }
  Short_t         GetValueSS( Int_t i = 0 ) const { return *fValueSS[i]; }
  UShort_t        GetValueWW( Int_t i = 0 ) const { return *fValueWW[i]; }
  Char_t          GetValueCC( Int_t i = 0 ) const { return *fValueCC[i]; }
  Byte_t          GetValueBB( Int_t i = 0 ) const { return *fValueBB[i]; }

  const void*     GetValuePointer()  const   { return fValueP; }

  virtual ULong_t Hash() const               { return fArrayData.Hash(); }
  virtual Int_t   Index( const char* ) const;
  virtual Int_t   Index( const THaArrayString& ) const;
  Bool_t          IsArray() const            
    { return fCount ? kTRUE : fArrayData.IsArray(); }
  Bool_t          IsPointerArray() const { return ( fType>=kDoubleP ); }
  // cleaner, but less efficient:
  // (fType == kDoubleP || fType == kFloatP  || fType == kLongP ||
  //  fType == kULongP  || fType == kIntP    || fType == kUIntP ||
  //  fType == kShortP  || fType == kUShortP || fType == kCharP ||
  //  fType == kByteP)
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

  void SetVar( const Double_t* var ) { fValueD = var; fType = kDouble; }
  void SetVar( const Float_t* var )  { fValueF = var; fType = kFloat; }
  void SetVar( const Long_t* var )   { fValueL = var; fType = kLong; }
  void SetVar( const ULong_t* var )  { fValueX = var; fType = kULong; }
  void SetVar( const Int_t* var )    { fValueI = var; fType = kInt; }
  void SetVar( const UInt_t* var )   { fValueU = var; fType = kUInt; }
  void SetVar( const Short_t* var )  { fValueS = var; fType = kShort; }
  void SetVar( const UShort_t* var ) { fValueW = var; fType = kUShort; }
  void SetVar( const Char_t* var )   { fValueC = var; fType = kChar; }
  void SetVar( const Byte_t* var )   { fValueB = var; fType = kByte; }

  void SetVar( const Double_t** var ) { fValueDD = var; fType = kDoubleP; }
  void SetVar( const Float_t** var )  { fValueFF = var; fType = kFloatP; }
  void SetVar( const Long_t** var )   { fValueLL = var; fType = kLongP; }
  void SetVar( const ULong_t** var )  { fValueXX = var; fType = kULongP; }
  void SetVar( const Int_t** var )    { fValueII = var; fType = kIntP; }
  void SetVar( const UInt_t** var )   { fValueUU = var; fType = kUIntP; }
  void SetVar( const Short_t** var )  { fValueSS = var; fType = kShortP; }
  void SetVar( const UShort_t** var ) { fValueWW = var; fType = kUShortP; }
  void SetVar( const Char_t** var )   { fValueCC = var; fType = kCharP; }
  void SetVar( const Byte_t** var )   { fValueBB = var; fType = kByteP; }

  virtual void    SetName( const char* );
  virtual void    SetNameTitle( const char* name, const char* descript );

protected:
  Double_t           GetValueAsDouble( Int_t i=0 ) const;

  THaArrayString     fArrayData; //Variable name and array dimension(s), if any
  union {
    const Double_t*  fValueD;  //Pointers to data (variable or array)
    const Float_t*   fValueF;
    const Long_t*    fValueL;
    const ULong_t*   fValueX;
    const Int_t*     fValueI;
    const UInt_t*    fValueU;
    const Short_t*   fValueS;
    const UShort_t*  fValueW;
    const Char_t*    fValueC;
    const Byte_t*    fValueB;
    const Double_t** fValueDD;  //Pointers to array of pointers to data
    const Float_t**  fValueFF;  // (for non-contiguous arrays)
    const Long_t**   fValueLL;
    const ULong_t**  fValueXX;
    const Int_t**    fValueII;
    const UInt_t**   fValueUU;
    const Short_t**  fValueSS;
    const UShort_t** fValueWW;
    const Char_t**   fValueCC;
    const Byte_t**   fValueBB;
    const void*      fValueP;
  };
  VarType            fType;    //Data type (see VarType.h)
  const Int_t*       fCount;   //Pointer to actual array size for var size array

  ClassDef(THaVar,0)   //Global symbolic variable
};

//_______________ Inlines _____________________________________________________
inline
Double_t THaVar::GetValueAsDouble( Int_t i ) const
{
  if( IsPointerArray() && (!fValueDD || !fValueDD[i] ))
      return 0.0;
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
    return *fValueDD[i];
  case kFloatP:
    return static_cast<Double_t>( *fValueFF[i] );
  case kLongP:
    return static_cast<Double_t>( *fValueLL[i] );
  case kULongP:
    return static_cast<Double_t>( *fValueXX[i] );
  case kIntP:
    return static_cast<Double_t>( *fValueII[i] );
  case kUIntP:
    return static_cast<Double_t>( *fValueUU[i] );
  case kShortP:
    return static_cast<Double_t>( *fValueSS[i] );
  case kUShortP:
    return static_cast<Double_t>( *fValueWW[i] );
  case kCharP:
    return static_cast<Double_t>( *fValueCC[i] );
  case kByteP:
    return static_cast<Double_t>( *fValueBB[i] );
  default:
    ;
  }
  return 0.0;
}

#endif

