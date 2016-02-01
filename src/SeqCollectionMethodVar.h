#ifndef Podd_SeqCollectionMethodVar
#define Podd_SeqCollectionMethodVar

//////////////////////////////////////////////////////////////////////////
//
// SeqCollectionMethodVar
//
// A "global variable" referencing a class member function call on
// an object in a TSeqCollection
//
//////////////////////////////////////////////////////////////////////////

#include "MethodVar.h"
#include "SeqCollectionVar.h"

namespace Podd {

  class SeqCollectionMethodVar : public MethodVar, public SeqCollectionVar {

  public:
    SeqCollectionMethodVar( THaVar* pvar, const void* addr, VarType type,
			    TMethodCall* method, Int_t offset );

    virtual const void*  GetDataPointer( Int_t i = 0 ) const;
    virtual Bool_t       IsBasic() const;
  };

}// namespace Podd

#endif
