#ifndef PODD_EXTRADATA_H_
#define PODD_EXTRADATA_H_

//////////////////////////////////////////////////////////////////////////
//
// Podd::ExtraData
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"

namespace Podd {

//___________________________________________________________________________
class ExtraData : public TObject {
public:
  ExtraData( TObject* obj, TObject* prev );
  virtual ~ExtraData();

  virtual void Clear( Option_t* ) {};
  virtual void Print( Option_t* opt="" ) const;

  ExtraData* Find( TObject* obj ) const;

protected:
  ExtraData* fNext;
  TObject*   fOwner;
};

} // namespace Podd

#endif
