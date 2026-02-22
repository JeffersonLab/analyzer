#ifndef Podd_VariableArrayVar_h_
#define Podd_VariableArrayVar_h_

//////////////////////////////////////////////////////////////////////////
//
// VariableArrayVar
//
// Interface to variable-size arrays of analysis results
//
//////////////////////////////////////////////////////////////////////////

#include "Variable.h"

namespace Podd {
  class VariableArrayVar : virtual public Variable {

  public:
    VariableArrayVar( THaVar* pvar, const void* addr, VarType type,
                      const Int_t* count );

    VarPtr_t       clone( THaVar* pvar ) const override;

    Int_t          GetLen()  const override;
    Int_t          GetNdim() const override;
    const Int_t*   GetDim()  const override;

    Bool_t         HasSizeVar() const override;

    Bool_t         HasSameSize( const Variable& rhs ) const override;
    Bool_t         IsContiguous() const override;
    Bool_t         IsPointerArray() const override;
    Bool_t         IsVarArray() const override;

  protected:
    const Int_t*   fCount;    //Pointer to array size variable
  };

}// namespace Podd

#endif
