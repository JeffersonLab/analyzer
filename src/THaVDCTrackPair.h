#ifndef ROOT_THaVDCTrackPair
#define ROOT_THaVDCTrackPair

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCTrackPair                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"

class THaVDCUVTrack;

class THaVDCTrackPair : public TObject {

public:
  THaVDCTrackPair() : 
    fLowerTrack(NULL), fUpperTrack(NULL), fError(1e307) {}
  THaVDCTrackPair( THaVDCUVTrack* lt, THaVDCUVTrack* ut );
    
  THaVDCTrackPair( const THaVDCTrackPair& );
  THaVDCTrackPair& operator=( const THaVDCTrackPair& );
  
  virtual ~THaVDCTrackPair() {}

  void            Analyze( Double_t spacing );
  virtual Int_t   Compare( const TObject* ) const;
  Double_t        GetError()   const { return fError; }
  THaVDCUVTrack*  GetLower()   const { return fLowerTrack; }
  THaVDCUVTrack*  GetUpper()   const { return fUpperTrack; }
  virtual Bool_t  IsSortable() const { return kTRUE; }
  virtual void    Print( Option_t* opt="" ) const;

protected:

  THaVDCUVTrack*  fLowerTrack;     // Lower UV track
  THaVDCUVTrack*  fUpperTrack;     // Upper UV track
  Double_t        fError;          // Measure of goodness of match of the tracks

  ClassDef(THaVDCTrackPair,0)      // A pair of VDC UV tracks
};

//////////////////////////////////////////////////////////////////////////////

#endif
