#ifndef Podd_VectorVar_h_
#define Podd_VectorVar_h_

//////////////////////////////////////////////////////////////////////////
//
// VectorVar
//
// A "global variable" referencing a std::vector of basic data
//
//////////////////////////////////////////////////////////////////////////

#include "Variable.h"

namespace Podd {

  class VectorVar : virtual public Variable {

  public:
    VectorVar( THaVar* pvar, const void* addr, VarType type );

    VarPtr_t       clone( THaVar* pvar ) const override;

    Int_t          GetLen()  const override;
    Int_t          GetNdim() const override;
    const Int_t*   GetDim()  const override;
    const void*    GetDataPointer( Int_t i = 0 ) const override;
    Bool_t         HasSameSize( const Variable& rhs ) const override;
    Bool_t         IsBasic() const override;
    Bool_t         IsContiguous() const override;
    Bool_t         IsPointerArray() const override;
    Bool_t         IsStreamable() const override;
    Bool_t         IsVarArray() const override;

  protected:
    mutable Int_t  fDim;    //Current array dimension
  };

}//namespace Podd

#endif
