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
#include "TMethodCall.h"

namespace Podd {

  class MethodVar : virtual public Variable {
  protected:
    using MethodPtr_t = std::unique_ptr<TMethodCall>;
  public:
    MethodVar( THaVar* pvar, const void* addr, VarType type,
	       MethodPtr_t method );

    virtual VarPtr_t     clone( THaVar* pvar ) const;

    virtual const void*  GetDataPointer( Int_t i = 0 ) const;
    virtual Bool_t       IsBasic() const;

  protected:
    MethodPtr_t          fMethod;   //Member function to access data in object
    // Data cache, filled in GetDataPointer()
    mutable Double_t     fData;     //Function call result (interpretation depends on fType!)

    const void*  GetDataPointer( const void* obj ) const;
  };

}// namespace Podd

#endif
