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

    VarPtr_t       clone( THaVar* pvar ) const override;

    Int_t          GetLen()  const override;
    Int_t          GetNdim() const override;
    const Int_t*   GetDim()  const override;

    Bool_t         HasSameSize( const Variable& rhs ) const override;
    Bool_t         IsContiguous() const override;
    Bool_t         IsPointerArray() const override;
    Bool_t         IsVarArray() const override;
    void           Print( Option_t* opt="FULL" ) const override;

    void           SetName( const char* ) override;
    void           SetNameTitle( const char* name, const char* desc ) override;

  protected:
    THaArrayString fParsedName;
  
    Bool_t       VerifyArrayName( const THaArrayString& astr ) const;
  };

}// namespace Podd

#endif
