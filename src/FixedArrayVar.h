#ifndef Podd_FixedArrayVar_h_
#define Podd_FixedArrayVar_h_

//////////////////////////////////////////////////////////////////////////
//
// FixedArrayVar
//
// Interface to fixed-size arrays of analysis results
//
//////////////////////////////////////////////////////////////////////////

#include "Variable.h"
#include "THaArrayString.h"

namespace Podd {
  class FixedArrayVar : virtual public Variable {

  public:
    FixedArrayVar( THaVar* pvar, const void* addr, VarType type );

    virtual Int_t        GetLen()  const;
    virtual Int_t        GetNdim() const;
    virtual const Int_t* GetDim()  const;

    virtual Bool_t       HasSameSize( const Variable& rhs ) const;
    virtual Bool_t       IsContiguous() const;
    virtual Bool_t       IsPointerArray() const;
    virtual Bool_t       IsVarArray() const;
    virtual void         Print( Option_t* opt="FULL" ) const;

    virtual void         SetName( const char* );
    virtual void         SetNameTitle( const char* name, const char* desc );

  protected:
    THaArrayString       fParsedName;
  
    Bool_t               VerifyArrayName( const THaArrayString& astr ) const;
  };

}// namespace Podd

#endif
