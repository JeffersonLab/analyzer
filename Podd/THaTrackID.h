#ifndef Podd_THaTrackID_h_
#define Podd_THaTrackID_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaTrackID                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"

class THaTrackID : public TObject {

public:
  THaTrackID() = default;

  virtual Bool_t  operator==( const THaTrackID& ) const = 0;
  Bool_t operator!=( const THaTrackID& rhs ) const { return !operator==(rhs); }

  TObject* Clone(const char* ="") const override = 0;

protected:

  ClassDef(THaTrackID,0)      // Track ID abstract base class
};

#endif
