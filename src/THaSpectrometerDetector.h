#ifndef Podd_THaSpectrometerDetector_h_
#define Podd_THaSpectrometerDetector_h_

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometerDetector
//
// Abstract base class for a generic spectrometer detector.
//
// This is a specialized detector class that supports the concept of
// "tracking" and "PID" detectors.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"

class THaTrack;
class TVector3;

class THaSpectrometerDetector : public THaDetector {

public:
  virtual ~THaSpectrometerDetector();

  virtual Bool_t   IsTracking() = 0;
  virtual Bool_t   IsPid()      = 0;

          Bool_t   CheckIntercept( THaTrack* track );
          Bool_t   CalcInterceptCoords( THaTrack* track,
					Double_t& x, Double_t& y );
          Bool_t   CalcPathLen( THaTrack* track, Double_t& t );
          Bool_t   CalcTrackIntercept( THaTrack* track, TVector3& icept,
				       Double_t& pathl );
          Bool_t   CalcTrackIntercept( THaTrack* track, Double_t& pathl,
				       Double_t& xdet, Double_t& ydet );

  THaSpectrometerDetector();       // for ROOT I/O only

protected:
  //Only derived classes may construct me
  THaSpectrometerDetector( const char* name, const char* description,
			   THaApparatus* a = NULL );

  ClassDef(THaSpectrometerDetector,1)   //ABC for a spectrometer detector
};

#endif
