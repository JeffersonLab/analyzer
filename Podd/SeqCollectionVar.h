#ifndef Podd_SeqCollectionVar_h_
#define Podd_SeqCollectionVar_h_

//////////////////////////////////////////////////////////////////////////
//
// SeqCollectionVar
//
// A "global variable" referencing data in objects in a TSeqCollection.
// In particular, this includes data in objects in TClonesArrays.
//
//////////////////////////////////////////////////////////////////////////

#include "Variable.h"

namespace Podd {

  class SeqCollectionVar : virtual public Variable {

  public:
    SeqCollectionVar( THaVar* pvar, const void* addr, VarType type,
		      Int_t offset );

    VarPtr_t       clone( THaVar* pvar ) const override;

    Int_t          GetLen()  const override;
    Int_t          GetNdim() const override;
    const Int_t*   GetDim()  const override;
    const void*    GetDataPointer( Int_t i = 0 ) const override;
    Bool_t         HasSameSize( const Variable& rhs ) const override;
    Bool_t         IsBasic() const override;
    Bool_t         IsContiguous() const override;
    Bool_t         IsPointerArray() const override;
    Bool_t         IsVarArray() const override;

  protected:
    Int_t          fOffset;   //Offset of data w.r.t. object pointer
    mutable Int_t  fDim;      //Current array dimension
  };

}// namespace Podd

#endif
