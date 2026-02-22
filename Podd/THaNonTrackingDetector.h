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

  ~THaNonTrackingDetector() override;

  void             Clear( Option_t* ="" ) override;
  virtual Int_t    CoarseProcess( TClonesArray& tracks ) = 0;
  virtual Int_t    FineProcess( TClonesArray& tracks )  = 0;
  Bool_t           IsTracking() override { return false; }
  Bool_t           IsPid() override { return false; }

  Int_t GetNTracks() const;  // Number of tracks crossing this detector
  const TClonesArray* GetTrackHits() const { return fTrackProj; }

protected:

  TClonesArray*  fTrackProj;  // projection of track(s) onto detector plane

  Int_t          DefineVariables( EMode mode = kDefine ) override;
  Int_t          CalcTrackProj( const TClonesArray& tracks );

  //Only derived classes may construct me
  THaNonTrackingDetector( const char* name, const char* description,
                          THaApparatus* a = nullptr);

  ClassDefOverride(THaNonTrackingDetector,2)  //ABC for a non-tracking spectrometer detector
};

#endif
