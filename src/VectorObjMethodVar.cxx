//*-- Author :    Ole Hansen   15/2/2018

//////////////////////////////////////////////////////////////////////////
//
// VectorObjMethodVar
//
// A "global variable" referencing a class member function call
// on objects held in a std::vector.
//
//////////////////////////////////////////////////////////////////////////

#include "VectorObjMethodVar.h"

using namespace std;

namespace Podd {

//_____________________________________________________________________________
VectorObjMethodVar::VectorObjMethodVar( THaVar* pvar, const void* addr,
	VarType type, Int_t elem_size, TMethodCall* method )
  : Variable(pvar,addr,type),
    SeqCollectionVar(pvar,addr,type,0),
    MethodVar(pvar,addr,type,method),
    VectorObjVar(pvar,addr,type,elem_size,0)
{
  // Constructor
}

//_____________________________________________________________________________
const void* VectorObjMethodVar::GetDataPointer( Int_t i ) const
{
  // Get pointer to data from method call on i-th object stored in a
  // std::vector

  const void* obj = VectorObjVar::GetDataPointer(i);
  if( !obj )
    return 0;

  // Get data from method, using object retrieved from the std::vector above
  return MethodVar::GetDataPointer(obj);
}

//_____________________________________________________________________________
Bool_t VectorObjMethodVar::IsBasic() const
{
  // Data are basic (POD variable or array)

  return kFALSE;
}

}// namespace Podd
