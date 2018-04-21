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

    virtual Int_t        GetLen()  const;
    virtual Int_t        GetNdim() const;
    virtual const Int_t* GetDim()  const;
    virtual const void*  GetDataPointer( Int_t i = 0 ) const;
    virtual Bool_t       HasSameSize( const Variable& rhs ) const;
    virtual Bool_t       IsBasic() const;
    virtual Bool_t       IsContiguous() const;
    virtual Bool_t       IsPointerArray() const;
    virtual Bool_t       IsStreamable() const;
    virtual Bool_t       IsVarArray() const;

  protected:
    mutable Int_t        fDim;    //Current array dimension
  };

}//namespace Podd

#endif
