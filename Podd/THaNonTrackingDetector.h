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
          Bool_t   IsTracking() { return kFALSE; }
  virtual Bool_t   IsPid()      { return kFALSE; }

  Int_t GetNTracks() const;  // Number of tracks crossing this detector
  const TClonesArray* GetTrackHits() const { return fTrackProj; }

protected:

  TClonesArray*  fTrackProj;  // projection of track(s) onto detector plane

  Int_t CalcTrackProj( TClonesArray& tracks );

  //Only derived classes may construct me
  THaNonTrackingDetector( const char* name, const char* description,
			  THaApparatus* a = NULL);

  ClassDef(THaNonTrackingDetector,2)  //ABC for a non-tracking spectrometer detector
};

#endif
