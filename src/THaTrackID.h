#ifndef ROOT_THaTrackID
#define ROOT_THaTrackID

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaTrackID                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "TObject.h"

class THaVDCUVTrack;

class THaTrackID : public TObject {

public:
  THaTrackID() {}
  THaTrackID( const THaTrackID& rhs ) : TObject(rhs) {}
  THaTrackID& operator=( const THaTrackID& rhs )
  { TObject::operator=(rhs); return *this; }

  virtual ~THaTrackID() {}

  virtual Bool_t  operator==( const THaTrackID& ) = 0;
  virtual Bool_t  operator!=( const THaTrackID& ) = 0;

protected:

  ClassDef(THaTrackID,0)      // Track ID abstract base class
};

#endif
