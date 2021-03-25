#ifndef Podd_OldVDCTrackPair_h_
#define Podd_OldVDCTrackPair_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCTrackPair                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"

class OldVDCUVTrack;
typedef OldVDCUVTrack* pUV;

class OldVDCTrackPair : public TObject {

public:
  OldVDCTrackPair() : 
    fLowerTrack(nullptr), fUpperTrack(nullptr), fError(1e38), fStatus(0) {}
  OldVDCTrackPair( pUV lt, pUV ut ) :
    fLowerTrack(lt), fUpperTrack(ut), fError(1e307), fStatus(0) {}
  OldVDCTrackPair( const OldVDCTrackPair& rhs ) = default;
  OldVDCTrackPair& operator=( const OldVDCTrackPair& );
  
  virtual ~OldVDCTrackPair() = default;

  void            Analyze( Double_t spacing );
  virtual Int_t   Compare( const TObject* ) const;
  Double_t        GetError()   const { return fError; }
  pUV             GetLower()   const { return fLowerTrack; }
  pUV             GetUpper()   const { return fUpperTrack; }
  Int_t           GetStatus()  const { return fStatus; }
  virtual Bool_t  IsSortable() const { return true; }
  void            SetStatus( Int_t i ) { fStatus = i; }
  virtual void    Print( Option_t* opt="" ) const;

  static Double_t GetProjectedDistance( pUV here, pUV there,
                                        Double_t spacing );

protected:

  pUV             fLowerTrack;     // Lower UV track
  pUV             fUpperTrack;     // Upper UV track
  Double_t        fError;          // Measure of goodness of match of the tracks
  Int_t           fStatus;         // Status flag


  ClassDef(OldVDCTrackPair,0)      // A pair of VDC UV tracks
};

//////////////////////////////////////////////////////////////////////////////

#endif
