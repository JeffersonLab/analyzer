#ifndef Podd_Variable_h_
#define Podd_Variable_h_

//////////////////////////////////////////////////////////////////////////
//
// Variable
//
// Base class for Podd "global variables". Supports basic data types.
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include "VarType.h"

class THaVar;
class THaArrayString;

namespace Podd {
  class Variable {

  public:
    Variable( THaVar* pvar, const void* addr, VarType type );
    virtual ~Variable();

    virtual Int_t        GetLen()  const;
    virtual Int_t        GetNdim() const;
    virtual const Int_t* GetDim()  const;

            VarType      GetType() const { return fType; }

    virtual std::vector<Double_t>     GetValues() const;
    virtual Double_t     GetValue( Int_t i = 0 ) const;
    virtual Long64_t     GetValueInt( Int_t i = 0 ) const;

            const void*  GetValuePointer() const { return fValueP; }
    virtual const void*  GetDataPointer( Int_t i = 0 ) const;
    virtual size_t       GetData( void* buf ) const;
    virtual size_t       GetData( void* buf, Int_t i ) const;

    virtual Bool_t       HasSameSize( const Variable& rhs ) const;
    virtual Bool_t       HasSizeVar() const;
    virtual Int_t        Index( const THaArrayString& ) const;
    virtual Bool_t       IsArray() const;
    virtual Bool_t       IsBasic() const;
    virtual Bool_t       IsContiguous() const;
    virtual Bool_t       IsError() const;
    virtual Bool_t       IsFloat() const;
    virtual Bool_t       IsPointerArray() const;
    virtual Bool_t       IsStreamable() const;
    virtual Bool_t       IsTObject() const;
    virtual Bool_t       IsVarArray() const;
    virtual Bool_t       IsVector() const;

    virtual void         Print( Option_t* opt ) const;
    virtual void         SetName( const char* name );
    virtual void         SetNameTitle( const char* name, const char* descript );

  protected:
    THaVar*              fSelf;     //Back-pointer to parent (containing name & description)
    const void*          fValueP;   //Pointer to data (interpretation depends on fType)
    VarType              fType;     //Data type (see VarType.h)

    const char*          GetName() const;
    size_t               GetTypeSize() const;
    const char*          GetTypeName() const;
    Bool_t               VerifyNonArrayName( const char* name ) const;

  };

} //namespace Podd

#endif
