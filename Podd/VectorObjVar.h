#ifndef Podd_VectorObjVar_h_
#define Podd_VectorObjVar_h_

//////////////////////////////////////////////////////////////////////////
//
// VectorObjVar
//
// A "global variable" referencing data in objects held in a std::vector.
//
//////////////////////////////////////////////////////////////////////////

#include "SeqCollectionVar.h"

namespace Podd {

  class VectorObjVar : virtual public SeqCollectionVar {

  public:
    VectorObjVar( THaVar* pvar, const void* addr, VarType type,
		  Int_t elem_size, Int_t offset );

    virtual Int_t        GetLen()  const;
    virtual const void*  GetDataPointer( Int_t i = 0 ) const;
    virtual Bool_t       HasSameSize( const Variable& rhs ) const;

  protected:
    Int_t   fElemSize;  // Size of one vector element. If 0, assume pointers
			// (This is Int_t, not size_t, because that's what
			//  TClass::GetClassSize() returns)
  };

}// namespace Podd

#endif
