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
  THaVar() : fValueD(NULL), fType(kDouble), fCount(NULL) {}
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

  Double_t        GetValue() const          
     { return (fType == kDouble ) ? *fValueD : GetValueAsDouble(); }
  Double_t        GetValueD() const          { return *fValueD; }
  Float_t         GetValueF() const          { return *fValueF; }
  Long_t          GetValueL() const          { return *fValueL; }
  ULong_t         GetValueX() const          { return *fValueX; }
  Int_t           GetValueI() const          { return *fValueI; }
  UInt_t          GetValueU() const          { return *fValueU; }
  Short_t         GetValueS() const          { return *fValueS; }
  UShort_t        GetValueW() const          { return *fValueW; }
  Char_t          GetValueC() const          { return *fValueC; }
  Byte_t          GetValueB() const          { return *fValueB; }
  Double_t        GetValue( Int_t i )  const 
     { return (fType == kDouble) ? fValueD[i] : GetValueAsDouble(i); }
  Double_t        GetValueD( Int_t i ) const { return fValueD[i]; }
  Float_t         GetValueF( Int_t i ) const { return fValueF[i]; }
  Long_t          GetValueL( Int_t i ) const { return fValueU[i]; }
  ULong_t         GetValueX( Int_t i ) const { return fValueX[i]; }
  Int_t           GetValueI( Int_t i ) const { return fValueI[i]; }
  UInt_t          GetValueU( Int_t i ) const { return fValueU[i]; }
  Short_t         GetValueS( Int_t i ) const { return fValueS[i]; }
  UShort_t        GetValueW( Int_t i ) const { return fValueW[i]; }
  Char_t          GetValueC( Int_t i ) const { return fValueC[i]; }
  Byte_t          GetValueB( Int_t i ) const { return fValueB[i]; }

  const Double_t* GetValuePointer()  const   { return fValueD; }
  const Double_t* GetValuePointerD() const   { return fValueD; }
  const Float_t*  GetValuePointerF() const   { return fValueF; }
  const Long_t*   GetValuePointerL() const   { return fValueL; }
  const ULong_t*  GetValuePointerX() const   { return fValueX; }
  const Int_t*    GetValuePointerI() const   { return fValueI; }
  const UInt_t*   GetValuePointerU() const   { return fValueU; }
  const Short_t*  GetValuePointerS() const   { return fValueS; }
  const UShort_t* GetValuePointerW() const   { return fValueW; }
  const Char_t*   GetValuePointerC() const   { return fValueC; }
  const Byte_t*   GetValuePointerB() const   { return fValueB; }

  virtual ULong_t Hash() const               { return fArrayData.Hash(); }
  virtual Int_t   Index( const char* ) const;
  virtual Int_t   Index( const THaArrayString& ) const;
  Bool_t          IsArray() const            
    { return fCount ? kTRUE : fArrayData.IsArray(); }
  virtual void    Print( Option_t* opt="FULL" ) const;

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

  virtual void    SetName( const char* );
  virtual void    SetNameTitle( const char* name, const char* descript );

protected:
  Double_t          GetValueAsDouble( Int_t i=0 ) const;

  THaArrayString    fArrayData; //Variable name and array dimension(s), if any
  union {
    const Double_t* fValueD;  //Pointer to data (variable or array)
    const Float_t*  fValueF;
    const Long_t*   fValueL;
    const ULong_t*  fValueX;
    const Int_t*    fValueI;
    const UInt_t*   fValueU;
    const Short_t*  fValueS;
    const UShort_t* fValueW;
    const Char_t*   fValueC;
    const Byte_t*   fValueB;
  };
  VarType           fType;    //Data type (0=double, 1=float, 2=int, etc.)
  const Int_t*      fCount;   //Pointer to actual array size for var size array

  ClassDef(THaVar,0)   //Global symbolic variable
};

//_______________ Inlines _____________________________________________________
inline
Double_t THaVar::GetValueAsDouble( Int_t i ) const
{
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
  default:
    ;
  }
  return 0.0;
}

#endif

