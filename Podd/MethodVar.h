#ifndef Podd_MethodVar_h_
#define Podd_MethodVar_h_

//////////////////////////////////////////////////////////////////////////
//
// MethodVar
//
// A "global variable" referencing a class member function call
//
//////////////////////////////////////////////////////////////////////////

#include "Variable.h"

class TMethodCall;

namespace Podd {

  class MethodVar : virtual public Variable {

  public:
    MethodVar( THaVar* pvar, const void* addr, VarType type,
	       TMethodCall* method );
    virtual ~MethodVar();

    virtual const void*  GetDataPointer( Int_t i = 0 ) const;
    virtual Bool_t       IsBasic() const;

  protected:
    TMethodCall*         fMethod;   //Member function to access data in object
    // Data cache, filled in GetDataPointer()
    mutable Double_t     fData;     //Function call result (interpretation depends on fType!)

    const void*  GetDataPointer( const void* obj ) const;
  };

}// namespace Podd

#endif
