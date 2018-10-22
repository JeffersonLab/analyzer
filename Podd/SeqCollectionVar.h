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

    virtual Int_t        GetLen()  const;
    virtual Int_t        GetNdim() const;
    virtual const Int_t* GetDim()  const;
    virtual const void*  GetDataPointer( Int_t i = 0 ) const;
    virtual Bool_t       HasSameSize( const Variable& rhs ) const;
    virtual Bool_t       IsBasic() const;
    virtual Bool_t       IsContiguous() const;
    virtual Bool_t       IsPointerArray() const;
    virtual Bool_t       IsVarArray() const;

  protected:
    Int_t                fOffset;   //Offset of data w.r.t. object pointer
    mutable Int_t        fDim;      //Current array dimension
  };

}// namespace Podd

#endif
