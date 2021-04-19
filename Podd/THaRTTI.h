#ifndef Podd_THaRTTI_h_
#define Podd_THaRTTI_h_

//////////////////////////////////////////////////////////////////////////
//
// THaRTTI
//
//////////////////////////////////////////////////////////////////////////

#include "TString.h"
#include "VarType.h"

class TDataMember;
class TRealData;
class TList;
class TObject;

class THaRTTI {

public:
  enum  EArrayType { kScalar, kFixed, kVariable, kVector };

  THaRTTI() :
    fOffset(-1), fType(kDouble), fArrayType(kScalar), fCountOffset(-1),
    fDataMember(nullptr), fRealData(nullptr), fElemClass(nullptr) {}
  virtual ~THaRTTI() = default;

  Int_t        Find( TClass* cl, const TString& var,
		     const void* p = nullptr );

  EArrayType   GetArrayType()   const { return fArrayType; }
  TClass*      GetClass()       const;
  Long_t       GetCountOffset() const { return fCountOffset; }
  TDataMember* GetDataMember()  const { return fDataMember; }
  Long_t       GetOffset()      const { return fOffset; }
  TRealData*   GetRealData()    const { return fRealData; }
  const char*  GetSubscript()   const { return fSubscript.Data(); }
  VarType      GetType()        const { return fType; }
  Bool_t       IsArray()        const { return (fArrayType != kScalar); }
  Bool_t       IsObject()       const { return (fType == kObject ||
						fType == kObjectP ||
						fType == kObject2P); }
  Bool_t       IsObjVector()    const { return (fType == kObjectV ||
						fType == kObjectPV); }
  Bool_t       IsPointer()      const;
  Bool_t       IsValid()        const { return (fOffset != -1); }
  void         Print( Option_t* opt="" ) const;
  void         Reset() {
    fOffset = -1; fType = kDouble; fArrayType = kScalar; fCountOffset = -1;
    fDataMember = nullptr; fRealData = nullptr; fElemClass = nullptr;
  }

protected:

  Long_t       fOffset;       // Offset with respect to THIS pointer
  VarType      fType;         // Variable type (kObject if object)
  EArrayType   fArrayType;    // Array type (see EArrayType)
  TString      fSubscript;    // For fixed array: Description of dimension(s)
  Long_t       fCountOffset;  // For var array: Offset of length specifier
  TDataMember* fDataMember;   // Associated ROOT TDataMember
  TRealData*   fRealData;     // Associated ROOT TRealData
  TClass*      fElemClass;    // Class of object vector element type

  ClassDef(THaRTTI,0)   //Parsed type information for a ROOT class member variable
};

#endif
