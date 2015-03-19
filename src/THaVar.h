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
    fValueP(0), fType(kDouble), fCount(0), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const THaVar& rhs );
  THaVar& operator=( const THaVar& );
  virtual ~THaVar();

  THaVar( const char* name, const char* descript, const Double_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueD(&var), fType(kDouble),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Float_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueF(&var), fType(kFloat),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Long64_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueL(&var), fType(kLong),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const ULong64_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueX(&var), fType(kULong),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Int_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueI(&var), fType(kInt),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const UInt_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueU(&var), fType(kUInt),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Short_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueS(&var), fType(kShort),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const UShort_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueW(&var), fType(kUShort),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Char_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueC(&var), fType(kChar),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Byte_t& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueB(&var), fType(kByte),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}

  THaVar( const char* name, const char* descript, const Double_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueDD(&var), fType(kDoubleP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}

  THaVar( const char* name, const char* descript, const Float_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueFF(&var), fType(kFloatP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Long64_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueLL(&var), fType(kLongP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const ULong64_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueXX(&var), fType(kULongP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Int_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueII(&var), fType(kIntP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const UInt_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueUU(&var), fType(kUIntP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Short_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueSS(&var), fType(kShortP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const UShort_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueWW(&var), fType(kUShortP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Char_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueCC(&var), fType(kCharP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}
  THaVar( const char* name, const char* descript, const Byte_t*& var,
	  const Int_t* count = 0 ) :
    TNamed(name,descript), fParsedName(name), fValueBB(&var), fType(kByteP),
    fCount(count), fOffset(-1), fMethod(0), fDim(0) {}

  THaVar( const char* name, const char* desc, const void* obj,
	  VarType type, Int_t offset, TMethodCall* method=0,
	  const Int_t* count=0 );

  virtual const char*  GetName() const { return fParsedName.GetName(); }

  Int_t           GetLen()       const;
  Int_t           GetNdim()      const
    { return IsVarArray() ? 1 : fParsedName.GetNdim(); }
  const Int_t*    GetDim()       const;
  VarType         GetType()      const { return fType; }
  size_t          GetTypeSize()  const { return GetTypeSize( fType ); }
  const char*     GetTypeName()  const { return GetTypeName( fType ); }

  Double_t        GetValue( Int_t i = 0 )  const { return GetValueAsDouble(i); }
  const void*     GetValuePointer()        const { return fValueP; }

  virtual ULong_t Hash() const { return fParsedName.Hash(); }
  virtual Bool_t  HasSameSize( const THaVar& rhs ) const;
  virtual Bool_t  HasSameSize( const THaVar* rhs ) const;
  virtual Int_t   Index( const char* ) const;
  virtual Int_t   Index( const THaArrayString& ) const;
  Bool_t          IsVarArray() const
    { return ( fCount != 0 || fOffset != -1 || IsVector() ); }
  Bool_t          IsArray() const
    { return ( IsVarArray() || fParsedName.IsArray() ); }
  Bool_t          IsBasic() const
    { return ( fOffset == -1 && fMethod == 0 ); }
  Bool_t          IsPointerArray() const
    { return ( IsArray() && fType>=kDouble2P && fType <= kObject2P ); }
  Bool_t          IsVector() const
    { return ( fType >= kIntV && fType <= kDoubleV ); }
  virtual void    Print( Option_t* opt="FULL" ) const;

  // The following are necessary to initialize empty THaVars such as those in arrays,
  // where each element was constructed with the default constructor
  void SetVar( const Double_t& var ) { fValueD = &var; fType = kDouble; }
  void SetVar( const Float_t& var )  { fValueF = &var; fType = kFloat; }
  void SetVar( const Long64_t& var ) { fValueL = &var; fType = kLong; }
  void SetVar( const ULong64_t& var ){ fValueX = &var; fType = kULong; }
  void SetVar( const Int_t& var )    { fValueI = &var; fType = kInt; }
  void SetVar( const UInt_t& var )   { fValueU = &var; fType = kUInt; }
  void SetVar( const Short_t& var )  { fValueS = &var; fType = kShort; }
  void SetVar( const UShort_t& var ) { fValueW = &var; fType = kUShort; }
  void SetVar( const Char_t& var )   { fValueC = &var; fType = kChar; }
  void SetVar( const Byte_t& var )   { fValueB = &var; fType = kByte; }

  void SetVar( const Double_t*& var ) { fValueDD = &var; fType = kDoubleP; }
  void SetVar( const Float_t*& var )  { fValueFF = &var; fType = kFloatP; }
  void SetVar( const Long64_t*& var ) { fValueLL = &var; fType = kLongP; }
  void SetVar( const ULong64_t*& var ){ fValueXX = &var; fType = kULongP; }
  void SetVar( const Int_t*& var )    { fValueII = &var; fType = kIntP; }
  void SetVar( const UInt_t*& var )   { fValueUU = &var; fType = kUIntP; }
  void SetVar( const Short_t*& var )  { fValueSS = &var; fType = kShortP; }
  void SetVar( const UShort_t*& var ) { fValueWW = &var; fType = kUShortP; }
  void SetVar( const Char_t*& var )   { fValueCC = &var; fType = kCharP; }
  void SetVar( const Byte_t*& var )   { fValueBB = &var; fType = kByteP; }

  void SetVar( const Double_t**& var ) { fValue3D = &var; fType = kDouble2P; }
  void SetVar( const Float_t**& var )  { fValue3F = &var; fType = kFloat2P; }
  void SetVar( const Long64_t**& var ) { fValue3L = &var; fType = kLong2P; }
  void SetVar( const ULong64_t**& var ){ fValue3X = &var; fType = kULong2P; }
  void SetVar( const Int_t**& var )    { fValue3I = &var; fType = kInt2P; }
  void SetVar( const UInt_t**& var )   { fValue3U = &var; fType = kUInt2P; }
  void SetVar( const Short_t**& var )  { fValue3S = &var; fType = kShort2P; }
  void SetVar( const UShort_t**& var ) { fValue3W = &var; fType = kUShort2P; }
  void SetVar( const Char_t**& var )   { fValue3C = &var; fType = kChar2P; }
  void SetVar( const Byte_t**& var )   { fValue3B = &var; fType = kByte2P; }

  virtual void    SetName( const char* );
  virtual void    SetNameTitle( const char* name, const char* descript );

  // Access to detailed information about types defined in VarType.h
  static const char* GetEnumName( VarType type );
  static const char* GetTypeName( VarType type );
  static size_t      GetTypeSize( VarType type );

protected:
  Double_t            GetValueAsDouble( Int_t i=0 ) const;
  Double_t            GetValueFromObject( Int_t i=0 ) const;
  Int_t               GetObjArrayLen() const;

  THaArrayString      fParsedName; //Variable name and array dimension(s), if any
  union {
    const Double_t*   fValueD;  //Pointers to data (scalar or stack array)
    const Float_t*    fValueF;
    const Long64_t*   fValueL;
    const ULong64_t*  fValueX;
    const Int_t*      fValueI;
    const UInt_t*     fValueU;
    const Short_t*    fValueS;
    const UShort_t*   fValueW;
    const Char_t*     fValueC;
    const Byte_t*     fValueB;
    const Double_t**  fValueDD;  //Pointers to pointers to array of data
    const Float_t**   fValueFF;  //(fixed or variable heap arrays)
    const Long64_t**  fValueLL;
    const ULong64_t** fValueXX;
    const Int_t**     fValueII;
    const UInt_t**    fValueUU;
    const Short_t**   fValueSS;
    const UShort_t**  fValueWW;
    const Char_t**    fValueCC;
    const Byte_t**    fValueBB;
    const Double_t*** fValue3D;  //Pointers to pointers to array of pointers
    const Float_t***  fValue3F;  //to data (non-contiguous arrays)
    const Long64_t*** fValue3L;
    const ULong64_t*** fValue3X;
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
  mutable Int_t       fDim;      //Current size of object array

  ClassDef(THaVar,0)   //Global symbolic variable
};

#endif

