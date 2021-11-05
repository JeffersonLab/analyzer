#ifndef Podd_THaNonTrackingDetector_h_
#define Podd_THaNonTrackingDetector_h_

//////////////////////////////////////////////////////////////////////////
//
// THaNonTrackingDetector.h
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometerDetector.h"

class TClonesArray;

class THaNonTrackingDetector : public THaSpectrometerDetector {

public:
  THaNonTrackingDetector(); // only for ROOT I/O

  virtual ~THaNonTrackingDetector();

  virtual void     Clear( Option_t* ="" );
  virtual Int_t    CoarseProcess( TClonesArray& tracks ) = 0;
  virtual Int_t    FineProcess( TClonesArray& tracks )  = 0;
  virtual Bool_t   IsTracking() { return false; }
  virtual Bool_t   IsPid()      { return false; }

  Int_t GetNTracks() const;  // Number of tracks crossing this detector
  const TClonesArray* GetTrackHits() const { return fTrackProj; }

protected:

  TClonesArray*  fTrackProj;  // projection of track(s) onto detector plane

  virtual Int_t  DefineVariables( EMode mode = kDefine );
  Int_t          CalcTrackProj( TClonesArray& tracks );

  //Only derived classes may construct me
  THaNonTrackingDetector( const char* name, const char* description,
                          THaApparatus* a = nullptr);

  ClassDef(THaNonTrackingDetector,2)  //ABC for a non-tracking spectrometer detector
};

#endif
