#ifndef Podd_VectorObjMethodVar_h_
#define Podd_VectorObjMethodVar_h_

//////////////////////////////////////////////////////////////////////////
//
// VectorObjMethodVar
//
// A "global variable" referencing a class member function call
// on objects held in a std::vector.
//
//////////////////////////////////////////////////////////////////////////

#include "MethodVar.h"
#include "VectorObjVar.h"

namespace Podd {

  class VectorObjMethodVar : public MethodVar, public VectorObjVar {

  public:
    VectorObjMethodVar( THaVar* pvar, const void* addr, VarType type,
                        Int_t elem_size, MethodPtr_t method );

    VarPtr_t     clone( THaVar* pvar ) const override;

    const void*  GetDataPointer( Int_t i = 0 ) const override;
    Bool_t       IsBasic() const override;
  };

}// namespace Podd

#endif
