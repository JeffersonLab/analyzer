//*-- Author :    Ole Hansen   18/2/2016

//////////////////////////////////////////////////////////////////////////
//
// SeqCollectionMethodVar
//
// A "global variable" referencing a member function call on objects
// in a TSeqCollection
//
//////////////////////////////////////////////////////////////////////////

#include "SeqCollectionMethodVar.h"

using namespace std;

namespace Podd {

//_____________________________________________________________________________
SeqCollectionMethodVar::SeqCollectionMethodVar( THaVar* pvar, const void* addr,
	VarType type, TMethodCall* method )
  : Variable(pvar,addr,type), MethodVar(pvar,addr,type,method),
    SeqCollectionVar(pvar,addr,type,0)
{
  // Constructor
}

//_____________________________________________________________________________
const void* SeqCollectionMethodVar::GetDataPointer( Int_t i ) const
{
  // Get pointer to data from method call on i-th object stored in a
  // TSeqCollection (e.g. TClonesArray)

  const void* obj = SeqCollectionVar::GetDataPointer(i);
  if( !obj )
    return 0;

  // Get data from method, using object retrieved from TSeqCollection above
  return MethodVar::GetDataPointer(obj);
}

//_____________________________________________________________________________
Bool_t SeqCollectionMethodVar::IsBasic() const
{
  // Data are basic (POD variable or array)

  return kFALSE;
}

}// namespace Podd
