#ifndef ROOT_THaVDCTrackPair
#define ROOT_THaVDCTrackPair

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCTrackPair                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "THaVDCCluster.h"   // for chi2_t

class THaVDCUVTrack;
class THaTrack;

class THaVDCTrackPair : public TObject {

public:
  THaVDCTrackPair( THaVDCUVTrack* lt, THaVDCUVTrack* ut ) :
    fLowerTrack(lt), fUpperTrack(ut), fError(1e307), fStatus(0) {}
  THaVDCTrackPair( const THaVDCTrackPair& rhs ) : TObject(rhs),
    fLowerTrack(rhs.fLowerTrack), fUpperTrack(rhs.fUpperTrack),
    fError(rhs.fError), fStatus(rhs.fStatus) {}
  THaVDCTrackPair& operator=( const THaVDCTrackPair& );

  virtual ~THaVDCTrackPair() {}

  void            Analyze( Double_t spacing );
  void            Associate( THaTrack* track );
  chi2_t          CalcChi2() const;
  virtual Int_t   Compare( const TObject* ) const;
  Double_t        GetError()   const { return fError; }
  THaVDCUVTrack*  GetLower()   const { return fLowerTrack; }
  THaVDCUVTrack*  GetUpper()   const { return fUpperTrack; }
  Int_t           GetStatus()  const { return fStatus; }
  THaTrack*       GetTrack()   const;
  virtual Bool_t  IsSortable() const { return kTRUE; }
  virtual void    Print( Option_t* opt="" ) const;
  void            Release();
  void            SetStatus( Int_t i ) { fStatus = i; }
  void            Use();

  Double_t        GetProjectedDistance( THaVDCUVTrack* here,
					THaVDCUVTrack* there,
					Double_t spacing );

protected:

  THaVDCUVTrack*  fLowerTrack;    // Lower UV track
  THaVDCUVTrack*  fUpperTrack;    // Upper UV track
  Double_t        fError;         // Measure of goodness of match of the tracks
  Int_t           fStatus;        // Status flag

private:
  THaVDCTrackPair();

  ClassDef(THaVDCTrackPair,0)     // A pair of VDC UV tracks
};

//////////////////////////////////////////////////////////////////////////////

#endif
