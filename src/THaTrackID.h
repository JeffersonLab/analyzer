#ifndef ROOT_THaTrackID
#define ROOT_THaTrackID

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaTrackID                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"

class THaTrackID : public TObject {

public:
  THaTrackID() {}
  virtual ~THaTrackID() {}

  virtual Bool_t  operator==( const THaTrackID& ) = 0;
  virtual Bool_t  operator!=( const THaTrackID& ) = 0;

protected:

  ClassDef(THaTrackID,0)      // Track ID abstract base class
};

#endif
