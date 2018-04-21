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

    virtual Int_t        GetLen()  const;
    virtual Int_t        GetNdim() const;
    virtual const Int_t* GetDim()  const;

    virtual Bool_t       HasSizeVar() const;

    virtual Bool_t       HasSameSize( const Variable& rhs ) const;
    virtual Bool_t       IsContiguous() const;
    virtual Bool_t       IsPointerArray() const;
    virtual Bool_t       IsVarArray() const;

  protected:
    const Int_t*         fCount;    //Pointer to array size variable
  };

}// namespace Podd

#endif
